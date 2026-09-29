#!/usr/bin/env python
import subprocess
import sys

output = subprocess.check_output([sys.executable, sys.argv[1], sys.argv[2]])
sys.stdout.write(output.decode('utf-8').replace('Z_TYPE_BAR_', 'ZBAR_TYPE_'))
