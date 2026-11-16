"""Hardware-independent CLI validation. No test enables RF."""
from pathlib import Path
import subprocess
import sys

exe = str(Path(sys.argv[1]).resolve())
cases = [
    (['--help'], 0, None),
    ([], 2, None),
    (['--transmit'], 2, None),
    (['--probe', '--transmit', '-f', '474000'], 2, None),
    (['--muxrate', '-b', '8000', '-r', '3/4', '-G', '1/32'], 0, 27144384),
    (['--muxrate', '-G', '1/34'], 2, None),
    (['--muxrate', '-b', '6500'], 2, None),
    (['--muxrate', '-t', '4k'], 2, None),
    (['--muxrate', '-C', 'qam256'], 2, None),
    (['--muxrate', '-r', '9/10'], 2, None),
    (['--muxrate', '-i', '65536'], 2, None),
    (['--muxrate', '-f', '474000junk'], 2, None),
    (['--muxrate', '-d', '-1'], 2, None),
    (['--muxrate', '-g', '999999999999999999'], 2, None),
    (['--muxrate', 'unexpected.ts'], 2, None),
]
for args, expected, rate in cases:
    result = subprocess.run([exe, *args], capture_output=True, text=True, timeout=5)
    assert result.returncode == expected, (args, result.returncode, result.stderr)
    if rate is not None:
        assert abs(int(result.stdout) - rate) <= 1, result.stdout
print(f'{len(cases)} CLI checks passed (RF never enabled)')
