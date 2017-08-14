# Copyright (c) 2008-2009 Nokia Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import os

os.chdir('c:\\data\\python\\test')
saveout = sys.stdout
saveerr = sys.stderr
fsock = open('regrtest_emu.log', 'w')
sys.stderr = sys.stdout = fsock

options = []
try:
    f = open('options.txt', 'r')
    str = f.read()
    options = str.split('\n')
except:
    print 'Executing all test cases'
try:
    sys.path.append('c:\\data\\python')
    import test.regrtest

    if str:
        sys.argv = ['python.exe'] + [x.replace('\r', '') for x in options]
    else:
        sys.argv = ['python.exe']
    test.regrtest.main()
    print 'all ok'
except:
    import traceback
    traceback.print_exc()

print 'finished'
sys.stdout = saveout
sys.stderr = saveerr
fsock.close()
