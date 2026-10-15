# Memory and GPU baseline (stock engine, M1)

2026-10-13, on the pre-audit binary (source `6bf147f1…`, binary `9a9225d8…`), stock WPE
`2.54.1-2`, Mesa `1:26.2.4-1`. The baseline for the custom-engine comparison (M5). The
levers already tried are in [audit-m1.md](audit-m1.md#memory-levers-tried); the current
totals in [measurements-m1-audit.md](measurements-m1-audit.md).

## Why blank exceeds 100 MiB

The 100 MiB goal is a stretch target for the whole process tree's PSS, not a limit. Even
`about:blank` starts WebKit's UI, network and web processes, their allocators and the
graphics stack. Three launches (950×1188, scale 1, samples 10-12 s after load) read
168.3-171.1 MiB, mean 170.1.

By process (launch 1, middle sample):

| Process | PSS, MiB | Anonymous | File-backed | Threads |
|---|---:|---:|---:|---:|
| UI (`foob`) | 32.698 | 20.086 | 12.603 | 10 |
| Network | 27.420 | 18.734 | 8.686 | 8 |
| Web (rendering) | 109.478 | 44.574 | 64.903 | 38 |
| Four bubblewrap helpers + D-Bus proxy | 1.496 | — | — | 8 |
| Total | 171.092 | 84.254 | 86.828 | 64 |

By mapping name, across the tree: `libWPEWebKit` 64.2 MiB, anonymous, heap and allocator
63.4, Mesa, graphics and LLVM 20.9, text and fonts 5.6, media libraries 2.1, other files
14.3. The web process alone holds 43.4 MiB of WebKit and 20.5 MiB of Mesa/LLVM mappings.

The UI process's 33 MiB: ~12.7 MiB `libWPEWebKit` mappings (only ~3.6 MiB private
relocation data; the library already uses `RELR` and `BIND_NOW`), ~11 MiB WebKit allocator
heap, a 2 MiB JIT region (JavaScriptCore starts here too), ~7 MiB other libraries (113
mapped, from the engine's dependencies).

The process and mapping views overlap and cannot be added. A mapping name shows which
library owns the pages, not which feature uses them; no heap profiling was done, so the
2.1 MiB of media libraries does not bound what removing media would save.

Conclusion: WebKit's initialization and its graphics stack dominate. foob's own code is
about 9 KB and the sandbox helpers 1.5 MiB; neither explains the ~70 MiB gap, and the
sandbox stays on. Whether a reduced custom build reaches 100 MiB is for M5 to measure.

## GPU use

Native Wayland alone does not prove GPU rendering, since WPE can fall back to shared-memory
buffers ([WPE architecture](https://wpewebkit.org/about/architecture.html)). Each web
process's own DRM client (`/dev/dri/renderD128`, amdgpu) reported nonzero engine time
([DRM usage stats](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html)):

| Launch | Graphics engine time | Compute engine time |
|---|---:|---:|
| 1 | 112,676 ns | 630,777 ns |
| 2 | 150,058 ns | 572,812 ns |
| 3 | 135,570 ns | 528,563 ns |

**The GPU does work for the blank page.** Not measured: the GL renderer string,
acceleration per feature and every-frame behaviour. The client also holds 48,508 KiB of
VRAM and 10,260 KiB of GTT; these are driver allocations, partly shared, and must not be
added to PSS.

## Reproduce

```sh
python3 tests/benchmark/run.py --label blank-attribution --uri about:blank \
    --memory-details --output docs/validation/runs/blank-attribution.json
```

`--memory-details` reads only the browser's own mapping metadata and DRM counters, not
page contents. Raw records stay local (`runs/blank-attribution-{1,2,3}`); compact values
are in [results.json](results.json).
