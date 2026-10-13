"""Retain compact reviewed evidence in Git; full generated runs stay local.

Run from repository root after validation:

    python3 tests/benchmark/summarize.py                      # runs/*.json -> docs/validation/results.json
    python3 tests/benchmark/summarize.py docs/validation/runs/suite-<time> --output NEW.json
    python3 tests/benchmark/summarize.py docs/validation/runs/suite-<time> --output docs/validation/results.json --replace

The input is ONE directory of run records (default: the top level of docs/validation/runs,
which does not include the suite-<time> folders that suite.py creates). Without --output
the summary goes to docs/validation/results.json for the default directory and to
<directory>/results.json otherwise. An existing output file is only replaced with --replace
(the default curated file is regenerated as before), and an input without run records is
rejected before anything is written. This never launches a browser or modifies the runs.
"""
import argparse
from collections import Counter, defaultdict
from datetime import datetime, timezone
import json
from pathlib import Path

default_directory = Path('docs/validation/runs')
curated = Path('docs/validation/results.json')
parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('directory', nargs='?', type=Path, default=default_directory)
parser.add_argument('--output', type=Path)
parser.add_argument('--replace', action='store_true', help='overwrite an existing --output file')
args = parser.parse_args()
directory = args.directory
target = args.output or (curated if directory == default_directory else directory / 'results.json')
if target.exists() and not args.replace and target != curated:
    raise SystemExit(f'{target} exists; choose a new --output or pass --replace')
if target == curated and args.output and not args.replace:
    raise SystemExit(f'{target} is the curated file; pass --replace to overwrite it')
result = {'schema_version': 1, 'generated_utc': datetime.now(timezone.utc).isoformat(),
          'raw_artifacts': 'docs/validation/runs/ is ignored; preserved locally',
          'measurements': {}, 'lifecycle_and_controls': {}, 'geometry_probes': {}}

records = [path for path in sorted(directory.glob('*.json')) if path.name != 'results.json']
if not records:
    raise SystemExit(f'no run records (*.json) in {directory}; nothing written')
for path in records:
    raw = json.loads(path.read_text())
    samples = raw.get('samples', [])
    common = {key: raw[key] for key in ['date_utc', 'revision', 'source_sha256', 'binary_sha256',
                                       'condition', 'result', 'lifecycle_result', 'exit_code', 'error'] if key in raw}
    common['tracked_identity_count'] = len(raw.get('tracked_processes', {}))
    common['survivor_count'] = len(raw.get('survivors_after_10s', {}))
    if samples:
        common['samples'] = [{
            'pss_kib': sample['total_pss_kib'],
            'seconds_after_completion': sample['seconds_after_observed_completion'],
            'viewport': sample['viewport'],
            'process_roles': dict(Counter(row['name'] for row in sample['processes'])),
        } for sample in samples]
        common['navigation_elapsed_ms'] = raw.get('navigation_elapsed_ms', [])
        roles = defaultdict(list)
        for sample in samples:
            totals = defaultdict(int)
            for row in sample['processes']:
                totals[row['name']] += row['pss_kib']
            for role, value in totals.items():
                roles[role].append(value)
        common['mean_role_pss_kib'] = {role: round(sum(values) / len(values), 3)
                                       for role, values in roles.items()}
        middle = samples[min(1, len(samples) - 1)]
        if 'memory_details' in middle:
            common['middle_sample_attribution'] = [{
                'role': row['name'], 'pss_kib': row['pss_kib'],
                'rollup_kib': {key: detail['rollup_kib'][key]
                               for key in ['Pss', 'Pss_Anon', 'Pss_File', 'Pss_Shmem']},
                'threads': detail['thread_count'],
                'mapping_group_pss_kib': detail['mapping_groups_pss_kib'],
                'largest_mapping_names': detail['largest_mapping_names_by_pss_kib'][:6],
                'drm_clients': detail['drm_clients'],
            } for row, detail in zip(middle['processes'], middle['memory_details'])
                if row['name'] in ['foob', 'WPENetworkProce', 'WPEWebProcess']]
        result['measurements'][path.stem] = common
    elif 'geometry' in raw:
        common['geometry_reports'] = raw['geometry']
        result['geometry_probes'][path.stem] = common
    else:
        result['lifecycle_and_controls'][path.stem] = common

target.write_text(json.dumps(result, indent=2) + '\n')
print(f'Saved compact evidence: {target}')
