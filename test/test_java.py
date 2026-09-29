#!/usr/bin/env python3
"""Compile and run the existing JNI tests outside the source tree."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

javac, java, archive, library, junit, *sources = sys.argv[1:]
classpath = os.pathsep.join([archive, junit, '/usr/share/java/hamcrest-core.jar'])
with tempfile.TemporaryDirectory() as classes:
    subprocess.run([javac, '-classpath', classpath, '-d', classes,
                    *sources], check=True)
    subprocess.run([java, '-Xcheck:jni',
                    '-Djava.library.path=' + str(Path(library).parent),
                    '-classpath', classpath + os.pathsep + classes,
                    'org.junit.runner.JUnitCore',
                    'TestImage', 'TestImageScanner', 'TestScanImage'], check=True)
