"""Linux browser-process identity, PSS and cleanup helpers; no external deps."""
from pathlib import Path
import re


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
