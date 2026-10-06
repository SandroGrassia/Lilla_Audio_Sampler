"""Run the repository's host regressions, reporting every failure and a final exit status."""
from pathlib import Path
import argparse
import os
import shutil
import subprocess
import sys
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--match', default='', help='Run only script names containing this text.')
    parser.add_argument('--list', action='store_true', help='List selected scripts without running them.')
    parser.add_argument('--mp3', action='store_true', help='Also run the optional FFmpeg cases in zero_raw_regression.py.')
    args = parser.parse_args()
    folder = Path(__file__).resolve().parent
    scripts = sorted(path for path in folder.glob('*_regression.py') if args.match in path.name)
    if not scripts:
        parser.error('No regression scripts match the selection.')
    if args.list:
        print('\n'.join(path.name for path in scripts))
        return 0
    environment = os.environ.copy()
    environment['PYTHONDONTWRITEBYTECODE'] = '1'
    compiler = shutil.which('g++')
    if compiler is None and os.name == 'nt':
        candidate = Path('C:/msys64/ucrt64/bin/g++.exe')
        if candidate.is_file():
            compiler = str(candidate)
    if compiler is None:
        parser.error('g++ with C++20 support is required; see tests/README.md.')
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    failed = []
    started = time.monotonic()
    for index, script in enumerate(scripts, 1):
        print(f'[{index}/{len(scripts)}] {script.name}', flush=True)
        command = [sys.executable, '-B', str(script)]
        if args.mp3 and script.name == 'zero_raw_regression.py':
            command.append('--mp3')
        result = subprocess.run(command, cwd=folder.parent, env=environment, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace')
        if result.returncode:
            failed.append(script.name)
            print(result.stdout, end='' if result.stdout.endswith('\n') else '\n', flush=True)
            print(f'FAIL: {script.name} (exit {result.returncode})', flush=True)
        else:
            print('PASS', flush=True)
    print(f'\n{len(scripts) - len(failed)} passed, {len(failed)} failed ({time.monotonic() - started:.1f}s).', flush=True)
    if failed:
        print('Failed scripts:\n' + '\n'.join(failed))
    return 1 if failed else 0


if __name__ == '__main__':
    raise SystemExit(main())
