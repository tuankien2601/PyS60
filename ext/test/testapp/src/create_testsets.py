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
import re

sys.path.append('c:\\resource\\python25\\python25_repo.zip')
sys.path.append('c:\\data\\python')
import shutil

os.chdir('c:\\data\\python\\test')
options = []

PYS60STDTESTS = [
    'test_pystone',
    'test_grammar',
    'test_opcodes',
    'test_operations',
    'test_builtin',
    'test_exceptions',
    'test_types',
    'test_unittest',
    'test_doctest',
    'test_doctest2',
    'test_shutil',
   ]

file_name_prefix = 'set_'
file_name_ext = '.txt'
ALPHABETS = 'abcdefghijklmnopqrstuvwxyz'
alpha = ALPHABETS


def get_filename():
    '''return filenames that are in alphabetical order'''
    global alpha
    global file_name_prefix

    if alpha == "":
        alpha = ALPHABETS
        file_name_prefix += "z"

    file_name = file_name_prefix + alpha[0] + file_name_ext
    alpha = alpha[1:]
    return file_name


def eof(fileobj):
    curloc = fileobj.tell()
    ofloc = fileobj.seek(0, 2)
    fileobj.seek(curloc, 0)
    if ofloc >= curloc:
        return True
    return False


def files_matching_regex(topdir, regex):
    files = []
    compiled_regex = re.compile(regex, re.I)
    for path, dirnames, filenames in os.walk(topdir):
        for x in filenames:
            if compiled_regex.match(x):
                files.append(x)
    return files


print 'Executing create_testsets\n'
f = open('options.txt', 'r')
options_str = f.read()
f.close()
options = options_str.split('\n')
# Make sure test cases dir is empty at start
testset_root_dir = 'c:\\data\\python\\test\\testcases'
if os.path.exists(testset_root_dir):
    for file in os.listdir(testset_root_dir):
        os.remove(os.path.join(testset_root_dir, file))
else:
    os.mkdir(testset_root_dir)
try:
    # If options.txt is empty, it means that test cases needs to be executed in
    # sets. Create files, each containing test case names that are to be
    # executed in one interpreter instance.
    if not options_str:
        START_TAG = "<<<<TestCase<<<<"
        END_TAG = ">>>>TestCase>>>>"
        if os.path.exists('testsets.cfg'):
            fp = open('testsets.cfg', 'r')
            results = fp.read()
        else:
            print 'testsets.cfg file not exist\n'
            print 'Executing regrtest.py in verbose mode\n'
            sys.exit(2)
        if not eof(fp):
            output_split = results.split(START_TAG)
            del output_split[0]
            output_split.reverse()
            for i in range(len(output_split)):
                test_set = os.path.join(testset_root_dir, 'set%s.txt'%i)
                print "create_testsets: Created %s" % test_set
                f = open(test_set, 'w')
                output = output_split.pop().split(END_TAG)
                f.write('%s\n' % output[0])
                f.close()
        fp.close()
        # Create file 'is_testcase_dir_empty.log', which is used by testapp to
        # check if there are test sets to be executed.
        fp = open('is_testcase_dir_empty.log', 'w')
        fp.write("This file is used by testapp to check if there are test " \
                  "sets to be executed.")
        fp.close()
    else:
        # If options.txt is not empty, then check for '--testset-size' option
        options = options_str.split('\n')
        import test.regrtest
        for item in [x.replace('\r', '') for x in options]:
            if item == '--testset-size':
                testset_size = options[1]
                testdir = 'c:\\data\\python\\test'
                files = test.regrtest.findtests(stdtests=PYS60STDTESTS)
                for i in range(0, len(files), int(testset_size)):
                    file = "%s\\%s" % (testset_root_dir, get_filename())
                    f = open(file, 'w')
                    for j in range(int(testset_size)):
                        if (i+j) >= len(files):
                            break
                        f.write(files[i+j])
                        f.write('\n')
                    f.close()
                fp = open('is_testcase_dir_empty.log', 'w')
                fp.write("This file is used by testapp to check if there are " \
                         "test sets to be executed.")
                fp.close()
except:
    import traceback
    traceback.print_exc()
