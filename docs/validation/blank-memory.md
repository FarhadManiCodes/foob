# Blank-page memory attribution and GPU evidence

2026-10-13. Read-only diagnosis of the stock M1 browser; no application/engine
feature, font, security or desktop configuration changed. Gate A remains ready
for review and not approved. No M2/custom-engine implementation.

## Why blank exceeds 100 MiB

The 100 MiB goal was an unverified **whole-browser-process-tree PSS stretch
target**, not a configured limit or a promise. Even `about:blank` initializes
WebKit's application/network/web processes, allocators and graphics runtime.
The application process alone is about 33 MiB; the complete browser is larger.

Three fresh attribution launches, each with three samples at approximately
10/11/12 seconds after load completion, all 950×1188 logical pixels/scale 1:

| Launch | Sample 1 / 2 / 3, MiB |
|---|---|
| blank-attribution-1 | 171.085 / 171.092 / 171.092 |
| blank-attribution-2 | 171.030 / 171.038 / 171.038 |
| blank-attribution-3 | 168.304 / 168.304 / 168.304 |

Mean 170.143 MiB; original M1 baseline mean remains 174.318 MiB. These later
diagnostic runs do not replace the original benchmark and are not an optimization
comparison. Source/binary/engine unchanged; OS caches and sharing are uncontrolled.
All launches exit 0 with no tracked survivors or load/process failures.

The web process is the largest contributor. Representative middle sample from
launch 1:

| Process role | PSS, MiB | Anonymous PSS | File-backed PSS | Threads |
|---|---:|---:|---:|---:|
| Application (`foob`) | 32.698 | 20.086 | 12.603 | 10 |
| Network | 27.420 | 18.734 | 8.686 | 8 |
| Web/rendering | 109.478 | 44.574 | 64.903 | 38 |
| Four bubblewrap helpers + D-Bus proxy | 1.496 | — | — | 8 combined |
| Total | 171.092 | 84.254 | 86.828 | 64 combined |

An additional 0.010 MiB is shared-memory PSS. This is resident, proportionally
accounted memory—not the much larger reserved virtual address space or the
engine's installed file size. PSS divides shared resident pages among their
mappers; [Linux proc documentation](https://www.kernel.org/doc/html/latest/filesystems/proc.html)
defines the accounting.

Grouped by mapping names across the whole tree (launch 1): approximately
64.2 MiB in `libWPEWebKit` mappings, 63.4 MiB in anonymous/heap/stack/named
allocator mappings, 20.9 MiB in Mesa/graphics/LLVM mappings, 5.6 MiB in
text/font-related mappings, 2.1 MiB in direct media-library mappings, and
14.3 MiB in other file mappings. Web process alone has about 43.4 MiB in
WebKit mappings and 20.5 MiB in Mesa/graphics/LLVM mappings.

These are **two views of the same memory**, not additive analyses. A library-named
mapping can include private copy-on-write anonymous data. The mapping-name view
therefore differs from the kernel's anon/file view. Per-VMA PSS rounding and
non-atomic reads also mean grouped values differ slightly from rollup totals.
Named `WKFastMalloc`/`JSJITCode` allocations are present even on blank; map names
do not establish exact object/feature ownership. No heap allocation profiling
was performed. The direct media mapping total does not bound media-owned heap
allocations or predict feature-removal savings.

This identifies the dominant resident components: WebKit initialization and its
graphics dependencies. Sandbox helpers account for about 1.5 MiB. There is no
evidence here that the 200-line application shell holds a large document/cache,
or that removing the helpers would address the approximately 70 MiB gap.
Security remains enabled. A supported custom-engine A/B experiment remains M5;
this snapshot cannot establish that such a build would meet 100 MiB.

## What acceleration means, and what is now confirmed

Hardware acceleration means the GPU participates in rendering/composing page
frames; JavaScript/layout and other work still use the CPU. Native Wayland alone
does not prove that path: WPE can present GPU buffers or shared-memory fallback
buffers. See [WPE architecture](https://wpewebkit.org/about/architecture.html).

The three blank launches now show actual hardware work for each WPE web process's
`/dev/dri/renderD128` client on `amdgpu`:

| Launch | Cumulative graphics engine time | Cumulative compute engine time |
|---|---:|---:|
| 1 | 112676 ns | 630777 ns |
| 2 | 150058 ns | 572812 ns |
| 3 | 135570 ns | 528563 ns |

These are nonzero cumulative counters from the browser's own DRM client,
not compositor counters or an idle GPU utilization guess. The
[kernel DRM usage specification](https://www.kernel.org/doc/html/latest/gpu/drm-usage-stats.html)
defines `drm-engine-*` as time executing that client's workloads.
**Hardware GPU participation is confirmed for this blank-page scenario.**
Exact GL renderer string, per-feature acceleration and every-frame rendering
behavior remain unmeasured; this is not a graphics stress/performance test.

The render client also reports 48508 KiB resident VRAM and 10260 KiB resident
GTT, with some shared buffers. These driver-owned allocations are recorded
separately; do **not** simply add them to PSS (overlap/shared ownership and
integrated-GPU accounting are not resolved). LLVM/Mesa library PSS above is
CPU-mapped memory, a different quantity.

## Evidence and reproduction

Committed compact evidence: [results.json](results.json).
Full generated `runs/blank-attribution-{1,2,3}.json` and `.log` files are ignored
and retained locally. The optional collector reads only its disposable browser's
kernel mapping metadata/DRM counters, not input contents or heap objects.

```sh
python3 tests/benchmark/run.py --label blank-attribution --uri about:blank \
    --memory-details --output docs/validation/runs/blank-attribution.json
python3 tests/benchmark/summarize.py
```

Stock WPE `2.54.1-2`, Mesa `1:26.2.4-1`, GCC 16/C23 `debugoptimized`.
App source SHA256: `6bf147f1390eef2881a8180f418236e5056373c336e081f71f20cb880995bce2`;
binary: `9a9225d877defb9f6c574ea1929653fdbef5355ad8db5ab5dc99e4ee10283229`.
The maintainer confirms no second monitor; multi-monitor checks are unavailable
for the current setup. Fractional scaling remains untested at the normal scale 1.
