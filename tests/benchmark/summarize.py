"""Retain compact reviewed evidence in Git; full generated runs stay local.

Run from repository root after validation: python3 tests/benchmark/summarize.py
This summarizes local artifacts and never launches a browser or modifies them.
"""
from collections import Counter, defaultdict
from datetime import datetime, timezone
import json
from pathlib import Path
import sys

# Optional argument: a run directory (for example a suite-<time> folder); its summary is
# written next to the runs. Without it the curated docs/validation/results.json is rewritten.
directory = Path(sys.argv[1]) if len(sys.argv) > 1 else Path('docs/validation/runs')
result = {'schema_version': 1, 'generated_utc': datetime.now(timezone.utc).isoformat(),
          'raw_artifacts': 'docs/validation/runs/ is ignored; preserved locally',
          'measurements': {}, 'lifecycle_and_controls': {}, 'geometry_probes': {}}

for path in sorted(directory.glob('*.json')):
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

target = directory / 'results.json' if len(sys.argv) > 1 else Path('docs/validation/results.json')
target.write_text(json.dumps(result, indent=2) + '\n')
print(f'Saved compact evidence: {target}')
