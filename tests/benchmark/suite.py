"""Run the approved M1 baseline serially; no concurrent browser workloads.

Every run writes into a new directory docs/validation/runs/suite-<UTC time>/ so earlier
evidence is never overwritten; summarize it with `python3 tests/benchmark/summarize.py <dir>`.
"""
from datetime import datetime, timezone
from pathlib import Path
import json
import subprocess
import sys

fixture = Path('tests/fixtures/viewport.html').resolve().as_uri()
runs = Path('docs/validation/runs') / datetime.now(timezone.utc).strftime('suite-%Y%m%dT%H%M%SZ')
runs.mkdir(parents=True)  # fails if it exists
print(f'writing to {runs}', flush=True)
runner = [sys.executable, 'tests/benchmark/run.py']
viewports = []
for name, uri in [('blank', 'about:blank'), ('fixture', fixture),
                  ('wikipedia', 'https://en.wikipedia.org/wiki/Mathematics')]:
    for repetition in range(1, 4):
        subprocess.run([*runner, '--label', f'{name}-{repetition}', '--uri', uri,
                        '--output', str(runs / f'{name}-{repetition}.json')], check=True)
        record = json.loads((runs / f'{name}-{repetition}.json').read_text())
        viewports.extend(item['viewport'] for item in record['samples'])
        if any(item != viewports[0] for item in viewports):
            raise RuntimeError('workload viewports/scales differ; comparison invalid')
subprocess.run([*runner, '--label', 'fixture-warm-reload', '--uri', fixture + '?warm=1',
                '--loads', '2', '--output', str(runs / 'fixture-warm-reload.json')], check=True)
warm = json.loads((runs / 'fixture-warm-reload.json').read_text())
if any(item['viewport'] != viewports[0] for item in warm['samples']):
    raise RuntimeError('warm-reload viewport differs')
