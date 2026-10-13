"""Read-only attribution for the disposable browser's memory and DRM clients.

Rollup accounting and mapping-name grouping are different views of the same
memory, not additive totals. No heap objects, input or page contents are read.
"""
from collections import defaultdict
import os
from pathlib import Path
import re


def category(name):
    if name.startswith('/dev/dri/') or 'dmabuf' in name.lower():
        return 'graphics_device_or_buffer_mapping'
    if not name or name.startswith(('[heap]', '[stack]', '[anon:', '[anon_shmem:')):
        return 'anonymous_heap_stack_named_allocations'
    if name.startswith('['):
        return 'kernel_special_mapping'
    if any(word in name for word in ['libWPEWebKit', 'libJavaScriptCore', 'libjavascriptcore']):
        return 'webkit_library_mappings'
    if any(word in name for word in ['libgallium', 'libLLVM', 'libEGL', 'libGL',
                                    'libgbm', 'libdrm', 'libvulkan', 'radeonsi']):
        return 'mesa_graphics_compiler_library_mappings'
    if any(word in name for word in ['libicu', 'libfontconfig', 'libfreetype',
                                    'libharfbuzz', '/fonts/', '/fontconfig/']):
        return 'text_font_library_and_resource_mappings'
    if any(word in name for word in ['libgst', '/gstreamer-', 'libavcodec',
                                    'libavformat', 'libpipewire']):
        return 'media_library_mappings'
    return 'other_file_mappings'


def details(pid):
    directory = Path(f'/proc/{pid}')
    rollup = {key: int(value) for key, value in re.findall(
        r'^([A-Za-z_]+):\s+(\d+) kB$', (directory / 'smaps_rollup').read_text(), re.MULTILINE)}
    mappings = []
    for line in (directory / 'smaps').read_text().splitlines():
        header = re.match(r'^[0-9a-f]+-[0-9a-f]+\s+([rwxps-]{4})\s+\S+\s+\S+\s+\d+\s*(.*)$', line)
        if header:
            mappings.append({'name': header[2] or '(anonymous)', 'permissions': header[1]})
        else:
            value = re.match(r'^([A-Za-z_]+):\s+(\d+) kB$', line)
            if value and mappings:
                mappings[-1][value[1]] = int(value[2])
    groups = defaultdict(int)
    names = defaultdict(int)
    executable_anon = 0
    for mapping in mappings:
        name = mapping['name']
        group = category('' if name == '(anonymous)' else name)
        pss = mapping.get('Pss', 0)
        groups[group] += pss
        names[name] += pss
        if group == 'anonymous_heap_stack_named_allocations' and 'x' in mapping['permissions']:
            executable_anon += pss
    clients = []
    seen = set()
    for descriptor in (directory / 'fd').iterdir():
        try:
            device = os.readlink(descriptor)
            if not device.startswith('/dev/dri/'):
                continue
            info = {key: value.strip() for key, value in re.findall(
                r'^(drm-[^:]+):\s*(.*)$', (directory / 'fdinfo' / descriptor.name).read_text(), re.MULTILINE)}
            identity = (info.get('drm-driver'), info.get('drm-pdev'), info.get('drm-client-id'))
            # fdinfo stats can repeat for duplicated handles to one DRM client.
            if identity in seen and info.get('drm-client-id') is not None:
                continue
            seen.add(identity)
            clients.append({'device': device, 'fd': descriptor.name, 'info': info})
        except OSError as error:
            clients.append({'fd': descriptor.name, 'inspection_error': type(error).__name__})
    return {
        'pid': pid,
        'rollup_kib': rollup,
        'thread_count': len(list((directory / 'task').iterdir())),
        'mapping_groups_pss_kib': dict(groups),
        'mapping_pss_sum_kib': sum(groups.values()),
        'anonymous_executable_mapping_pss_kib': executable_anon,
        'largest_mapping_names_by_pss_kib': [
            {'name': name, 'pss_kib': value} for name, value in
            sorted(names.items(), key=lambda item: item[1], reverse=True)[:15]],
        'drm_clients': clients,
    }
