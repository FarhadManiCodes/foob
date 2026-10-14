"""Linux browser-process identity, PSS and cleanup helpers; no external deps."""
import hashlib
from pathlib import Path
import re
import subprocess
import time


def source_manifest(root='src'):
    """SHA256 of every application source file, keyed by path, plus one combined digest."""
    files = {str(path): hashlib.sha256(path.read_bytes()).hexdigest()
             for path in sorted(Path(root).rglob('*')) if path.suffix in ('.c', '.h')}
    assert files, f'no .c/.h files under {root!r}; run from the repository root'
    listing = ''.join(f'{digest}  {name}\n' for name, digest in files.items())
    return hashlib.sha256(listing.encode()).hexdigest(), files


def provenance(binary):
    """What a record must say about what it measured: revision, dirty tree, source and binary hashes."""
    digest, files = source_manifest()
    return {
        'revision': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
        'source_dirty': bool(subprocess.check_output(['git', 'status', '--porcelain'], text=True)),
        'source_sha256': digest, 'source_files': files,
        'binary_sha256': hashlib.sha256(Path(binary).read_bytes()).hexdigest(),
    }


def processes():
    result = {}
    for entry in Path('/proc').iterdir():
        if not entry.name.isdecimal():
            continue
        try:
            stat = (entry / 'stat').read_text()
            fields = stat[stat.rindex(')') + 2:].split()
            result[int(entry.name)] = {
                'name': stat[stat.index('(') + 1:stat.rindex(')')],
                'state': fields[0], 'ppid': int(fields[1]), 'session': int(fields[3]),
                'start': int(fields[19]),
            }
        except (OSError, ValueError, IndexError):
            continue
    return result


def browser_tree(root, table):
    # Session membership retains detached/reparented children of start_new_session.
    members = {pid for pid, item in table.items() if item['session'] == root}
    members.add(root)
    while True:
        expanded = members | {pid for pid, item in table.items() if item['ppid'] in members}
        if expanded == members:
            return {pid: table[pid] for pid in members if pid in table}
        members = expanded


def sample(root):
    tree = browser_tree(root, processes())
    rows = []
    for pid, identity in sorted(tree.items()):
        content = Path(f'/proc/{pid}/smaps_rollup').read_text()
        pss = re.search(r'^Pss:\s+(\d+) kB$', content, re.MULTILINE)
        if pss is None:
            raise RuntimeError(f'PSS unavailable for {pid}')
        rows.append({'pid': pid, **identity, 'pss_kib': int(pss[1])})
    return {'processes': rows, 'total_pss_kib': sum(row['pss_kib'] for row in rows)}


def survivors(tracked):
    table = processes()
    return {pid: table[pid] for pid, identity in tracked.items()
            if pid in table and table[pid]['start'] == identity['start']}


FAILURE_MARKERS = ('load-failed ', 'status=failed', 'web-process-terminated')


def stop(process):
    """Terminates a browser that is still running. None: it had already exited; else whether it was killed."""
    if process.poll() is not None:
        return None
    process.terminate()
    try:
        process.wait(timeout=10)
        return False
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()
        return True


def survivors_after_wait(tracked):
    """The tracked processes still alive after up to 10 s."""
    deadline = time.monotonic() + 10
    while survivors(tracked) and time.monotonic() < deadline:
        time.sleep(0.1)
    return survivors(tracked)
