"""Capture native Teensy audio CPU counters; requires pyserial and diagnostic firmware."""
import argparse
import json
from pathlib import Path
import statistics
import time

import serial


def fields(parts):
    return {key: float(value) for key, value in (part.split('=', 1) for part in parts)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='COM8')
    parser.add_argument('--seconds', type=float, default=30)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    reports, lines = [], []
    frame = None
    with serial.Serial(args.port, 115200, timeout=0.1, write_timeout=2) as port:
        port.reset_input_buffer()
        port.write(b'r')
        started = time.monotonic()
        next_poll = started + 0.5
        pending = bytearray()
        while time.monotonic() - started < args.seconds + 0.5:
            now = time.monotonic()
            if now >= next_poll:
                port.write(b'v')
                next_poll = now + 0.5
            pending.extend(port.read(max(1, port.in_waiting)))
            while b'\n' in pending:
                raw, _, pending = pending.partition(b'\n')
                line = raw.decode('utf-8', errors='replace').strip()
                lines.append(line)
                parts = line.split(',')
                if parts[0] == 'REVERB_COST':
                    frame = {'kind': parts[1], 'settings': fields(parts[2:]), 'streams': {}}
                elif parts[0] == 'AUDIO_COST' and frame is not None:
                    frame['streams'][parts[1]] = fields(parts[2:])
                    if parts[1] == 'total':
                        if frame['kind'] == 'report':
                            reports.append(frame)
                        frame = None
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.with_suffix('.log').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    if not reports:
        raise SystemExit('No complete diagnostic reports received; check the firmware and serial port.')
    summary = {}
    for stream in reports[0]['streams']:
        samples = [report['streams'][stream] for report in reports]
        summary[stream] = {
            'median_polled_current_us': statistics.median(sample['current_us'] for sample in samples),
            'window_peak_us': max(sample['peak_us'] for sample in samples),
            'window_peak_pct': max(sample['peak_pct'] for sample in samples),
        }
    result = {
        'port': args.port,
        'duration_seconds': args.seconds,
        'note': 'Median describes foreground polls, not all audio blocks. Peaks are native per-block maxima since reset; individual peaks need not coincide.',
        'summary': summary,
        'reports': reports,
    }
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'reports': len(reports), 'reverb': summary['reverb'], 'total': summary['total'], 'max_polled_voices': max(report['settings']['voices'] for report in reports)}, indent=2))


if __name__ == '__main__':
    main()
