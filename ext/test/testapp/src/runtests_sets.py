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
import socket
import e32
from zipfile import ZipFile

os.chdir('c:\\data\\python\\test')
saveout = sys.stdout
saveerr = sys.stderr
log_file = os.path.join('c:\\data\\python\\test', 'regrtest_emu.log')
fsock = open(log_file, 'a')
if not fsock:
    print "Error creating log file"
sys.stderr = sys.stdout = fsock
sis_version = "version.py"
global testcase

testcase_dir = 'c:\\data\\python\\test\\testcases'
try:
    sys.path.append('c:\\data\\python')
    import warnings
    import test.regrtest
    if os.listdir(testcase_dir) == []:
        # testcase_dir is empty, delete file 'is_testcase_dir_empty.log'
        os.remove('C:\\data\\python\\test\\is_testcase_dir_empty.log')
    else:
        # testcase dir is not empty, run regrtest.py with -f option
        global testcase
        try:
            # If Testapp is executed by run_testapp, then test_set contains
            # the test set file name.
            testcase = test_set
        except:
            # Testapp is executed directly, get the first
            # file from testcase_dir
            listTestSets = os.listdir(testcase_dir)
            listTestSets.reverse()
            testcase = listTestSets[0]
        cmd = 'c:\\data\\python\\test\\regrtest.py'
        opt = '-f ' + testcase
        #Executes version.py before executing any of the test cases
        if testcase == "set_a.txt":
            execfile(sis_version)
        print "runtests_sets: " + testcase + "-> Started"
        if not e32.in_emulator():
            socket.set_default_access_point(u'Mobile Office')
            sys.argv = ['python.exe'] + ['-v'] + ['-u'] + \
                       ['network,urlfetch'] + ['-f'] + \
                       ['%s\\%s'%(testcase_dir,testcase)]
        else:
            sys.argv = ['python.exe'] + ['-v'] + ['-f'] + \
                       ['%s\\%s' % (testcase_dir, testcase)]
        test.regrtest.main()
except SystemExit:
    # Ignore the sys.exit at the end of regrtest's main()
    pass
except:
    import traceback
    traceback.print_exc()
finally:
    #delete the test_set file, so that its not used in next iteration.
    if os.listdir(testcase_dir) != []:
        os.remove(os.path.join(testcase_dir, testcase))

    if os.listdir(testcase_dir) == []: 
        # Finished executing all the test sets. Create a ZIP file from the
        # contents in testcasedata directory
        testcasedata_dir = 'c:\\data\\python\\test\\testcasedata\\'
        if not os.path.exists(testcasedata_dir):
            os.mkdir(testcasedata_dir)
        testcasedata_zip = ZipFile('c:\\data\\python\\test\\' +
                                   'testcasedata.zip', 'w')
        for testcasedata_file in os.listdir(testcasedata_dir):
            testcasedata_zip.write(testcasedata_dir + testcasedata_file)
        testcasedata_zip.close()

sys.stdout = saveout
sys.stderr = saveerr
fsock.close()
