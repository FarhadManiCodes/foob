"""Run the approved M1 baseline serially; no concurrent browser workloads."""
from pathlib import Path
import json
import subprocess
import sys

fixture = Path('tests/fixtures/viewport.html').resolve().as_uri()
runner = [sys.executable, 'tests/benchmark/run.py']
viewports = []
for name, uri in [('blank', 'about:blank'), ('fixture', fixture),
                  ('wikipedia', 'https://en.wikipedia.org/wiki/Mathematics')]:
    for repetition in range(1, 4):
        subprocess.run([*runner, '--label', f'{name}-{repetition}', '--uri', uri,
                        '--output', f'docs/validation/runs/{name}-{repetition}.json'], check=True)
        record = json.loads(Path(f'docs/validation/runs/{name}-{repetition}.json').read_text())
        viewports.extend(item['viewport'] for item in record['samples'])
        if any(item != viewports[0] for item in viewports):
            raise RuntimeError('workload viewports/scales differ; comparison invalid')
subprocess.run([*runner, '--label', 'fixture-warm-reload', '--uri', fixture + '?warm=1',
                '--loads', '2', '--output', 'docs/validation/runs/fixture-warm-reload.json'], check=True)
warm = json.loads(Path('docs/validation/runs/fixture-warm-reload.json').read_text())
if any(item['viewport'] != viewports[0] for item in warm['samples']):
    raise RuntimeError('warm-reload viewport differs')
