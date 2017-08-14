# Copyright (c) 2005-2009 Nokia Corporation
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
import base64
if sys.version_info < (2, 4):
    print "Python 2.4 or later required."
    sys.exit(2)
import os
from subprocess import *
import thread
from threading import Thread
import re
import imp
import compileall
import traceback
from zipfile import ZipFile
import time
import glob
import itertools
import string
from optparse import OptionParser

topdir = os.getcwd()
testdata_dir = '.\\build\\test'
testdir = os.path.abspath(os.path.join(topdir,
                                       '..\\test\\automatic\\standard'))
sys.path.append(os.path.join(topdir, 'tools'))
sys.path.append(os.path.join(topdir, 'newcore\\Symbian\\src'))

import fileutil
import template_engine
import module_config_parser
from shellutil import *

internal_proj = False

# Specify the magic number of the Python interpreter used in PyS60.
pys60_magic_number = '\xb3\xf2\r\n'

projects = [{'name': 'python25',
             'path': 'newcore\\symbian\\group',
             'internal': False},
            {'name': 'testapp',
             'path': 'ext\\test\\testapp\\group',
             'internal': False},
            {'name': 'run_testapp',
             'path': 'ext\\test\\run_testapp\\group',
             'internal': True},
            {'name': 'run-interpretertimer',
             'path': 'ext\\test\\run-interpretertimer\\group',
             'internal': True},
            {'name': 'interpreter-startup',
             'path': 'ext\\test\\interpreter-startup\\group',
             'internal': True}]



buildconfig_defaults={'PYS60_VERSION_MAJOR': 2,
                      'PYS60_VERSION_MINOR': 0,
                      'PYS60_VERSION_MICRO': 0,
                      # The default tag for a build is based on the
                      # time the configuration script was run. To make
                      # a build that claims to be a "release" build
                      # you need to specify a PYS60_VERSION_TAG
                      # explicitly.
                      'PYS60_VERSION_TAG': time.strftime(
                                           "development_build_%Y%m%d_%H%M"),
                      'PYS60_RELEASE_DATE': time.strftime("%d %b %Y"),
                      'PY_CORE_VERSION': '2.5.4',
                      'PY_ON_BUILD_SERVER': '2.5.1',
                      'PYS60_VERSION_SERIAL': 0,
                      'EMU_BUILD': 'udeb',
                      'DEVICE_PLATFORM': 'armv5',
                      'DEVICE_BUILD': 'urel',
                      'SRC_DIR': topdir,
                      'TEST_DIR': testdir,
                      'PROJECTS': projects,
                      'INTERNAL_PROJ': internal_proj,
                      'COMPILER_FLAGS': '',
                      'TEST_FLAG': 'OFF',
                      'EXTRA_SYSTEMINCLUDE_DIRS': [],
                      'WITH_MESSAGING_MODULE': 1,
                      'WITH_LOCATION_MODULE': 1,
                      'WITH_SENSOR_MODULE': 1,
                      'PREFIX': 'kf_',
                      'INCLUDE_ARMV5_PYDS': False,
                      'CTC_COVERAGE': False,
                      # UIDs 10201510 to 10201519 inclusive, allocated for
                      # PyS60 1.4.x
                      'PYS60_UID_CORE': '0x20022EE8',
                      'PYS60_UID_S60': '0x10201510',
                      'PYS60_UID_LAUNCHER': '0x20022EF0',
                      'PYS60_UID_TESTAPP': '0xF0201517',
                      'PYS60_UID_RUNTESTAPP': '0xF0201518',
                      'PYS60_UID_PYTHONUI': '0xF020151B',
                      'PYS60_UID_PYREPL': '0x10201519',
                      'OMAP2420': 0}

buildconfig_sdks = {
    '30armv5': {'S60_VERSION': 30,
           'DEVICE_PLATFORM': 'armv5',
           'EMU_PLATFORM': 'winscw',
           'SDK_NAME': 'S60 3rd Ed. w/ RVCT compiler',
           'SDK_MARKETING_VERSION_SHORT': '3rdEd',
           'PYS60_UID_SCRIPTSHELL': '0x20022EED',
           'S60_REQUIRED_PLATFORM_UID': '0x101F7961'},
    '30gcce': {'S60_VERSION': 30,
           'DEVICE_PLATFORM': 'gcce',
           'EMU_PLATFORM': 'winscw',
           'PYS60_UID_SCRIPTSHELL': '0x20022EED',
           'SDK_NAME': 'S60 3rd Ed. w/ GCCE compiler',
           'SDK_MARKETING_VERSION_SHORT': '3rdEd',
           'S60_REQUIRED_PLATFORM_UID': '0x101F7961'},
    '32': {'S60_VERSION': 32,
           'DEVICE_PLATFORM': 'armv5',
           'EMU_PLATFORM': 'winscw',
           'PYS60_UID_SCRIPTSHELL': '0x20022EEC',
           'SDK_NAME': 'S60 3rd EdFP2  w/ RVCT compiler',
           'SDK_MARKETING_VERSION_SHORT': '3rdEdFP2',
           'S60_REQUIRED_PLATFORM_UID': '0x102752AE'},
    '32gcce': {'S60_VERSION': 32,
           'DEVICE_PLATFORM': 'gcce',
           'EMU_PLATFORM': 'winscw',
           'PYS60_UID_SCRIPTSHELL': '0x20022EEC',
           'SDK_NAME': 'S60 3rd EdFP2  w/ GCCE compiler',
           'SDK_MARKETING_VERSION_SHORT': '3rdEdFP2',
           'S60_REQUIRED_PLATFORM_UID': '0x102752AE'},
    '50armv5': {'S60_VERSION': 50,
           'DEVICE_PLATFORM': 'armv5',
           'EMU_PLATFORM': 'winscw',
           'PYS60_UID_SCRIPTSHELL': '0x20022EEC',
           'SDK_NAME': 'S60 5th Ed  w/ RVCT compiler',
           'SDK_MARKETING_VERSION_SHORT': '5thEd',
           'S60_REQUIRED_PLATFORM_UID': '0x1028315F'}}

# 0 if missing files in SDK zip packaging are not allowed
allow_missing = 0

BUILDCONFIG_FILE = os.path.join(topdir, 'build.cfg')


def buildconfig_exists():
    return os.path.exists(BUILDCONFIG_FILE)


def buildconfig_load():
    global BUILDCONFIG
    if buildconfig_exists():
        BUILDCONFIG = eval(open(BUILDCONFIG_FILE, 'rt').read())
    else:
        raise BuildFailedException("Source not configured")


def buildconfig_save():
    open(BUILDCONFIG_FILE, 'wt').write(repr(BUILDCONFIG))


def buildconfig_clean():
    if os.path.exists(BUILDCONFIG_FILE):
        delete_file(BUILDCONFIG_FILE)


def get_project_details(params):
    buildconfig_load()
    requested_detail = []
    for project in projects:
        if not project['internal'] or BUILDCONFIG['INTERNAL_PROJ']:
            requested_detail.append(project[params])

    return requested_detail


def scanlog(log):
    analysis = run_shell_command('perl \\epoc32\\tools\\scanlog.pl', log)
    m = re.search(r'^Total\s+[0-9:]+\s+([0-9]+)\s+([0-9]+)',
                  analysis['stdout'], re.M)
    if m:
        return {'analysis': analysis,
                'errors': int(m.group(1)),
                'warnings': int(m.group(2))}
    else:
        raise Exception('scanlog.pl failed')


def run_command_and_check_log(cmd, verbose=1, ignore_errors=0):
    print 'Running "%s"' % cmd
    try:
        out = run_shell_command(cmd, mixed_stderr=1, verbose=verbose)
        n_errors = 0
        scanlog_result = scanlog(out['stdout'])
        n_errors = scanlog_result['errors']
    except:
        if ignore_errors:
            print 'Ignoring exception "%s" raised by command "%s"' \
            %(traceback.format_exception_only(sys.exc_info()[0], \
            sys.exc_info()[1]), cmd)
            return
        raise
    if n_errors > 0:
        if ignore_errors:
            print 'Ignoring errors of command "%s"' % cmd
        else:
            raise BuildFailedException('Command "%s" failed:\n%s' \
                                        % (cmd, out['stdout']))


def enter(dir):
    absdir = os.path.join(topdir, dir)
    print 'Entering "%s"' % absdir
    os.chdir(absdir)


def run_in(relative_dir, cmd, verbose=1, ignore_errors=0):
    curr_dir = os.getcwd()
    enter(relative_dir)
    run_command_and_check_log(cmd, verbose=verbose,
                              ignore_errors=ignore_errors)
    os.chdir(curr_dir)


def parse_assignments(params):
    d = {}
    for item in params:
        (name, value) = item.split('=')
        try:
            value = int(value)
        except ValueError:
            pass # not an integer
        d[name] = value
    return d


def cmd_configure(params):
    global BUILDCONFIG
    BUILDCONFIG = {}
    BUILDCONFIG.update(buildconfig_defaults)
    parser = OptionParser()
    parser.add_option("-p", "--profile_log", dest="profile",
            action="store_true", default=False,
            help="Profile log generator for python [default: %default]")
    parser.add_option("-v", "--version", dest="version", default='2.0.0',
            help="Python release version [default: %default]")
    parser.add_option("--compiler-flags", dest="compiler_flags",
            default='',
            help="Compiler flags to be used while building the python" +
                 " interpreter core [default: %default]")
    parser.add_option("--internal-projects", dest="internal_proj",
            action="store_true", default=False,
            help="Builds internal projects like 'interpreter-startup, " +
                 "run_testapp etc...  [default: %default]")
    parser.add_option("--version-tag", dest="version_tag", default='final',
            help="Specify release tag [default: %default]")
    parser.add_option("-s", "--sdk", dest="sdk", default='50armv5',
            help="Specify the version of sdk to use, choose any version " +
                 "from %s"%",".join(buildconfig_sdks.keys()) +
                 "[default: %default]")
    parser.add_option("-c", "--caps", dest="dll_caps",
            default='LocalServices NetworkServices ReadUserData ' +
                    'WriteUserData UserEnvironment Location',
            help="Specify DLL capability within quotes [default: %default]")
    parser.add_option("--debug-device", dest="debug_device",
            action='store_true',
            help="Build device builds in debug mode.")
    parser.add_option("--include-internal-src", action='store_true',
            dest="include_internal_src", default=False,
            help="Include the source under ..\internal-src for build." +
                 " [default: %default]")
    parser.add_option("--exe-caps", dest="exe_caps",
            help="Specify EXEs capability within quotes. " +
                 "If nothing is specified this will be same as DLL " +
                 "capabilities")
    parser.add_option("--compression-type",
            default='', dest="compression_type",
            help="Modify the compression type of all the " +
                 "E32Image files generated, using 'elftran " +
                 "-compressionmethod'. Refer elftran help for valid " +
                 "compression types. If the type is given as '', " +
                 "elftran is not invoked for modifying the compression type." +
                 " [default: '']")
    parser.add_option("-k", "--key", dest="key",
            help="Specify key name," +
                 " [default:  NONE] - packages are left unsigned")
    parser.add_option("--keydir", dest="keydir", default='..\\keys',
            help="Specify key path [default: %default])")
    parser.add_option("--do-not-compile-pyfiles",
            dest="do_not_compile_pyfiles", action="store_true", default=False,
            help="Do not compile the .py files under src. You can disable the "
                 "compilation if the Python version on the host system is not "
                 "bytecode compatible with the Python used in PyS60 "
                 "[default: %default]")
    parser.add_option("--build-profile",
            dest="build_profile", default='integration',
            help="Use this option to set the build_config variable, "
                 "BUILD_PROFILE to 'integration' or 'release'. BUILD_PROFILE "
                 "can then be used in the template processing. "
                 "[default: %default]")

    (options, args) = parser.parse_args()

    if not options.do_not_compile_pyfiles and \
                                      pys60_magic_number != imp.get_magic():
        raise SystemError("Not compiling the .py files: " + \
              "Python version(%s) " % sys.version.split()[0] + \
              "not bytecode compatible with the Python version " + \
              "used in PyS60(%s)." % BUILDCONFIG['PY_ON_BUILD_SERVER'])

    if options.sdk not in buildconfig_sdks:
        print 'Unsupported SDK configuration "%s"' % options.sdk
        sys.exit(2)

    # Find out if coverage called configure. There are chances that the cfg
    # file is missing or CTC_COVERAGE key is not present. We just ignore it.
    try:
        BUILDCONFIG_temp = eval(open(BUILDCONFIG_FILE, 'rt').read())
        if BUILDCONFIG_temp['CTC_COVERAGE']:
            BUILDCONFIG['CTC_COVERAGE'] = True
    except:
        pass
    BUILDCONFIG['PYS60_VERSION_NUM'] = options.version
    BUILDCONFIG['PYS60_VERSION'] = '%s %s' %(options.version,
                                             options.version_tag)
    [major, minor, micro] = options.version.split('.')
    BUILDCONFIG['PYS60_VERSION_MAJOR'] = major
    BUILDCONFIG['PYS60_VERSION_MINOR'] = minor
    BUILDCONFIG['PYS60_VERSION_MICRO'] = micro
    BUILDCONFIG['BUILD_PROFILE'] = options.build_profile
    if options.compiler_flags:
        BUILDCONFIG['COMPILER_FLAGS'] = "OPTION " \
        + options.compiler_flags
        log_file = topdir + "\\build\\regrtest_emulator_x86.log"
        if os.path.exists(log_file):
            open(log_file, "a+").write("\n" +
                "Build Info -- Name : <Compiler Flags>, Value : <" +
                options.compiler_flags + ">")
            # Running it again to get the compiler tags into the XML. In the
            # future when new build info metrics will be added, this can be
            # run at the end after appending all the values to emu log.
            run_cmd('python tools\\regrtest_log_to_cruisecontrol.py ' +
                    ' regrtest_emulator_x86.log')
    BUILDCONFIG['PYS60_VERSION_TAG'] = options.version_tag
    BUILDCONFIG['PROFILE_LOG'] = options.profile
    buildconfig_save()
    if options.key:
        BUILDCONFIG['SIGN_KEY'] = options.keydir + '\\' + \
        '%s' % options.key + '.key'
        BUILDCONFIG['SIGN_CERT'] = options.keydir + '\\' + \
        '%s' % options.key + '.crt'
        BUILDCONFIG['SIGN_PASS'] = ''
    BUILDCONFIG['DLL_CAPABILITIES'] = options.dll_caps
    if not options.exe_caps:
        BUILDCONFIG['EXE_CAPABILITIES'] = options.dll_caps
    else:
        BUILDCONFIG['EXE_CAPABILITIES'] = options.exe_caps
    BUILDCONFIG['COMPRESSION_TYPE'] = options.compression_type
    BUILDCONFIG.update(buildconfig_sdks[options.sdk])
    if options.debug_device:
        print "Configuring the device build as debug(UDEB)"
        BUILDCONFIG['DEVICE_BUILD'] = 'udeb'
    if options.internal_proj:
        global INTERNAL_PROJ
        BUILDCONFIG['INTERNAL_PROJ'] = True

    BUILDCONFIG['INCLUDE_INTERNAL_SRC'] = options.include_internal_src
    BUILDCONFIG['MOD_REPO'] = False

    print "Configuring for %s" % options.sdk
    buildconfig_save()
    # Check if a directory for build dependencies was given
    if 'BUILD_DEPS' in BUILDCONFIG:
        # Form the build dependencies include directory path. The
        # drive letter and colon are stripped from the path since the
        # Symbian build system doesn't understand drive letters in
        # paths.
        builddep_includes = os.path.abspath(os.path.join(
                            BUILDCONFIG['BUILD_DEPS'], BUILDCONFIG['SDK_TAG'],
                            'include'))[2:]
        if os.path.exists(builddep_includes):
            print "Adding extra include directory %s" % builddep_includes
            BUILDCONFIG['EXTRA_SYSTEMINCLUDE_DIRS'].append(builddep_includes)
    buildconfig_save()
    print "Build configuration:"
    for name, value in sorted(BUILDCONFIG.items()):
        print "  %s=%s" % (name, repr(value))
    BUILDCONFIG['ConfigureError'] = ConfigureError

    build_dirs = ['']
    if options.include_internal_src:
        build_dirs.append('..\\internal-src')
        build_dirs.append('..\\extraprojs')
    if not options.do_not_compile_pyfiles:
        compileall.compile_dir('newcore\\Lib', rx=re.compile('/[.]svn'))
    prev_dir = os.getcwd()
    os.chdir('newcore\\Symbian\\src')
    execfile('module_config_parser.py', BUILDCONFIG)
    os.chdir(prev_dir)

    # Go through all directories that have template files that need processing.
    for dir_ in build_dirs:
        for f in fileutil.all_files(dir_, '*.in'):
            # Omit newcore from template processing for now.
            if (f.startswith('newcore\\') or f.startswith('build\\')) and \
            not f.startswith('newcore\\Symbian\\') and \
            not f.startswith('newcore\\Doc\\s60'):
                print "Ignoring", f
                continue
            print "Processing template", f
            template_engine.process_file(f, BUILDCONFIG)

    for x in get_project_details('path'):
        run_in(x, 'bldmake clean')
        run_in(x, 'bldmake bldfiles')


def cmd_generate_ensymble(params):
    print "Building for ensymble started"
    buildconfig_load()
    mod_depcfg_dir = 'newcore\\Symbian\\src\\'
    ensymble_dir = 'tools\\py2sis\\ensymble\\'
    stub_dirs = ['tools\\py2sis\\python_console\\group',
                 'ext\\amaretto\\python_ui\\group']

    # It is a prerequisite for ensymble to have python built for armv5
    if not os.path.exists("\\epoc32\\release\\armv5\\urel\\python25.dll"):
        raise RuntimeError('Python not built for ARMV5')
    for stub_dir in stub_dirs:
        run_in(stub_dir, 'abld -keepgoing reallyclean armv5', ignore_errors=1)
        run_in(stub_dir, 'bldmake clean')
        run_in(stub_dir, 'bldmake bldfiles')
        run_in(stub_dir, 'abld build \
                     %(EMU_PLATFORM)s %(EMU_BUILD)s ' % BUILDCONFIG)
        run_in(stub_dir, 'abld build \
                     %(DEVICE_PLATFORM)s %(DEVICE_BUILD)s ' % BUILDCONFIG)
    run_in(ensymble_dir, 'python genensymble.py')
    os.chdir('newcore\\Symbian\\src')
    BUILDCONFIG['MOD_REPO'] = True
    execfile('module_config_parser.py', BUILDCONFIG)
    os.chdir(topdir)
    py_files = []
    os.chdir(mod_depcfg_dir)
    if os.path.exists('module_dependency.cfg'):
        shutil.move('module_dependency.cfg',
            os.path.join(topdir, ensymble_dir,
            'module-repo', 'standard-modules'))
    os.chdir(topdir)
    BUILDCONFIG['MOD_REPO'] = False


def cmd_build(params):
    global build_emulator
    global build_devices
    parser = OptionParser()
    parser.add_option("--emu", action="store_true",
            dest="build_emulator", default=False,
            help="Build for Emulator")
    parser.add_option("--device", action="store_true",
            dest="build_devices", default=False,
            help="Build for device")
    (options, args) = parser.parse_args()
    build_emulator = options.build_emulator
    build_devices = options.build_devices
    if build_emulator:
        # This check is done because when option(--emu or --device), is given
        # with the subsystem(ext\amaretto\<module>\group), the subsytem is
        # passed as the params
        if params.__len__() >= 2:
            build_emu(params[1:2])
        else:
            params = []
            build_emu(params)
    elif build_devices:
        if params.__len__() >= 2:
            build_device(params[1:2])
        else:
            params = []
            build_device(params)
    # if build option is specified without arguments then build for emulator
    # and device
    else:
        build_emu(params)
        build_device(params)


def build_module(params, build):
    if build == "emu":
        platform_type = '%(EMU_PLATFORM)s' % BUILDCONFIG
        build_type = '%(EMU_BUILD)s' % BUILDCONFIG
    elif build == "device":
        platform_type = '%(DEVICE_PLATFORM)s' % BUILDCONFIG
        build_type = '%(DEVICE_BUILD)s' % BUILDCONFIG

    if os.path.exists(params[0]):
        module_dir = params[0]
    else:
        module_dir = 'ext\\%s\\group' % params[0]
    run_in(module_dir, 'abld -keepgoing reallyclean %s' % platform_type,
           ignore_errors=1)
    run_in(module_dir, 'bldmake clean')
    run_in(module_dir, 'bldmake bldfiles')
    run_in(module_dir, 'abld build %s %s' %(platform_type, build_type))


def build_emu(params):
    buildconfig_load()
    if params:
        build_module(params, "emu")
    else:
        deltree_if_exists('\\epoc32\\winscw\\c\\data\\python\\test')
        os.makedirs('\\epoc32\\winscw\\c\\data\\python\\test')
        for x in get_project_details('path'):
            run_in(x, 'abld build %(EMU_PLATFORM)s %(EMU_BUILD)s '
                   % BUILDCONFIG)


def build_device(params):
    buildconfig_load()
    if params:
        build_module(params, "device")
    else:
        for x in get_project_details('path'):
            run_in(x, 'abld build %(DEVICE_PLATFORM)s %(DEVICE_BUILD)s'
                   % BUILDCONFIG)


def cmd_test_device_local(params):
    sys.path.append(os.path.abspath(os.path.join(topdir, '..\\test')))
    import test_device
    test_device.test_device_local()


def cmd_test_device_remote(params):
    sys.path.append(os.path.abspath(os.path.join(topdir, '..\\test')))
    import test_device
    buildconfig_load()
    test_device.test_device_remote()


def cmd_coverage(params):
    buildconfig_load()
    BUILDCONFIG['CTC_COVERAGE'] = True
    buildconfig_save()
    print " \n---- Code coverage module called in setup.py ---- \n "
    release_dir = os.path.abspath('..\\')
    run_cmd("SET CTC_LOCK_MAX_WAIT=0")
    parser = OptionParser()
    parser.add_option("--work-area", dest = "work_area", default = 'Build',
                        help = "Path for coverage results [default: %default]")
    (options, args) = parser.parse_args()
    s = os.path.splitdrive(options.work_area)[1]
    if (s != '' and s[:1] in '/\\'):
        testres_dir = options.work_area
    else:
        testres_dir = release_dir + '\\src\\%s' % options.work_area
    # As coverage is test related content it will be moved to test folder
    # under the work_area directory
    testres_dir += '\\test'
    config = {}


    # Read SDK and COVERAGE_DIRS from the cfg file in tools folder
    ctc_file = open(r'.\tools\ctc.cfg', 'r')
    for line in ctc_file:
        line = line.rstrip('\n').strip()
        if line and line[0] != '#':
            temp_list = line.split('=')
            config[temp_list[0]] = temp_list[1:]
    ctc_file.close()
    run_cmd('python setup.py configure -s ' + str(config['SDK'][0]))
    run_cmd('python setup.py build --emu')
    x = str(config['COVERAGE_DIRS'][0]).split('|')

    # Instrument the files using the ctcwrap command
    for group_dir in x:
        os.chdir(topdir + '\\' + group_dir)
        run_cmd('abld reallyclean')
        run_cmd('bldmake clean')
        run_cmd('bldmake bldfiles')
        run_cmd('ctcwrap -i m -v abld build winscw udeb', exception_on_error=0)

    # Verify instrumentation did not fail by looking at the log
    os.chdir(testres_dir)
    file_list = os.listdir(os.getcwd())
    for x in file_list:
        if x.startswith('pys60_build_log') and x.endswith('.log'):
            log_file = x
    compiled_reg = re.compile('CTC\+\+ Error')
    if compiled_reg.search(file(log_file).read()):
        print "Instrumentation failed"
    else:
        os.chdir(release_dir + '\\src')
        # Run testapp in textshell mode
        run_testapp()
        # Collect log, covert to html and move to build directory
        os.chdir(release_dir + '\\src\\newcore\\Symbian\\group')
        run_cmd('ctcpost MON.sym MON.dat -p profile.txt', exception_on_error=0)
        run_cmd('ctc2html -i profile.txt', exception_on_error=0)
        if os.path.exists('CTCHTML'):
            shutil.copytree('CTCHTML', testres_dir + '\\code_coverage')


def do_build(in_dir, device=False):
    cwd = os.getcwd()
    os.chdir(in_dir)
    build_cmd = 'abld build winscw udeb'
    if device:
        build_cmd = 'abld build %(DEVICE_PLATFORM)s %(DEVICE_BUILD)s' \
                    % BUILDCONFIG
    run_cmd('bldmake clean')
    run_cmd('bldmake bldfiles')
    run_cmd('abld reallyclean')
    run_cmd(build_cmd)
    os.chdir(cwd)


class KillTestappIfFrozen(Thread):

    def __init__(self, timeout):
        Thread.__init__(self)
        self.timeout = timeout
        self.counter = 0

    def run(self):
        while True:
            time.sleep(60)
            if os.system("pslist > temp.txt"):
                print "Add sysinternalsuite's install " + \
                "path to PATH env variable"
                break
            if open("temp.txt").read().find("testapp") == -1:
                break
            self.counter += 1
            if self.counter > self.timeout:
                run_cmd('pskill testapp.exe')
                break


def run_testapp():
    if not os.path.exists('\\epoc32\\data\\epoc.ini.bak'):
        os.rename('\\epoc32\\data\\epoc.ini', '\\epoc32\\Data\\epoc.ini.bak')
    f = open('\\epoc32\\data\\epoc.ini', 'wa')
    f1 = open('\\epoc32\\data\\epoc.ini.bak', 'r')
    f.write('textshell\n')
    for line in f1:
        f.write(line)
    f.close()
    f1.close()
    try:
        # Kill testapp(emulator) if it doesn't close in 40 minutes
        KillTestappIfFrozen(40).start()
        result = run_shell_command(
                '\\epoc32\\release\\winscw\\udeb\\testapp.exe',
                mixed_stderr = 1, verbose = 1, exception_on_error = 0)
        if result['return_code'] != 0:
            print "*** testapp crash - code %d ***" % result['return_code']
        else:
            print 'Finished executing all test cases'
    finally:
        delete_file('\\epoc32\\data\\epoc.ini')
        os.rename('\\epoc32\\data\\epoc.ini.bak', '\\epoc32\\data\\epoc.ini')


def cmd_test(params):
    buildconfig_load()
    if len(sys.argv) <= 2:
        cmd_help(())
        sys.exit(2)
    parser = OptionParser()
    parser.add_option("--testset-size", dest="test_size",
            action="store_true", default=False,
            help="Run all the tests under c:\data\python\test in sets of N")
    parser.add_option("--sdk-version", dest="sdk_version", default="_x86",
            help="Specify the sdk version")
    parser.add_option("--use-testsets-cfg", dest="testsets_cfg",
            action="store_true", default=False,
            help="Specify to run the tests listed in testcase.cfg")
    (options, args) = parser.parse_args()

    sdk_version = options.sdk_version
    if not options.testsets_cfg:
        f = open('options.txt', 'w')
        f.write("\n".join(sys.argv[2:]))
        f.close()
    shutil.move('options.txt', '\\epoc32\\winscw\\c\\data\\python\\test')
    run_testapp()
    if not os.path.exists(testdata_dir):
        os.makedirs(testdata_dir)
    regrtest_log = '\\epoc32\\winscw\\c\\data\\python\\test\\regrtest_emu.log'
    regrlog_version = 'regrtest_emulator%s.log' % sdk_version
    if os.path.exists(regrtest_log):
        shutil.move(regrtest_log,
               os.path.join(topdir, testdata_dir, regrlog_version))
        if sdk_version == '_3_2' or sdk_version == "_x86":
            run_cmd('python tools\\regrtest_log_to_cruisecontrol.py ' +
                regrlog_version)
        results = \
                open(testdata_dir + '\\' + regrlog_version, 'r').read()
        print "printing regrtest.py output"
        print results
        print "Finished printing"
    else:
        print "regrtest log was not created by testapp"


def install_sdk_files_to(directory):
    print "Installing SDK files to directory %s" % directory
    # Copy files to sdk_files directory
    execfile('tools/sdk_files.py', BUILDCONFIG)
    missing = []
    n_copied = 0
    for fromfile, tofile in BUILDCONFIG['SDK_FILES']:
        if not os.path.exists(fromfile):
            missing.append(fromfile)
            continue
        abs_tofile = os.path.normpath(os.path.join(directory, tofile))
        copy_file(fromfile, abs_tofile)
        n_copied += 1
    print "Installed SDK files to directory %s" % directory
    if missing:
        if not allow_missing:
            raise BuildFailedException('Files not found:\n  ' + \
                                       '\n  '.join(missing))
        else:
            print "** Warning: Following %d files were not found:\n" \
            % len(missing) + "\n  ".join(missing)
    return missing


def cmd_bdist_sdk(params):
    parser = OptionParser()

    parser.add_option("--create-temp-sdk-zip", action='store_true',
            dest="create_temp_sdk_zip", default=False,
            help="Include all armv5 built binaries in the sdk zip" +
                 " [default: %default]")

    (options, args) = parser.parse_args()

    buildconfig_load()

    if options.create_temp_sdk_zip:
        BUILDCONFIG['INCLUDE_ARMV5_PYDS'] = True

    sdk_files_dir = os.path.normpath(topdir + '/install/sdk_files')
    deltree_if_exists(sdk_files_dir)
    sdk_full_name = 'Python_SDK_%(SDK_MARKETING_VERSION_SHORT)s'% BUILDCONFIG
    install_sdk_files_to(sdk_files_dir)
    # Generate uninstaller
    f = open(sdk_files_dir + '/uninstall_%s.cmd' % sdk_full_name, 'wt')
    print >> f, '''@echo Uninstalling %s''' % sdk_full_name
    for fromfile, tofile in BUILDCONFIG['SDK_FILES']:
        print >> f, 'del ' + os.path.normpath(tofile)
    f.close()

    zipname = topdir + '/%s.zip' % sdk_full_name
    create_archive_from_directory(zipname, sdk_files_dir)


def cmd_bdist_sis(params):
    buildconfig_load()
    parser = OptionParser()
    parser.add_option("--keydir", dest = "keydir", default = '..\\keys',
            help = "specify key path [default: %default]")
    parser.add_option("-k", "--key", dest = "key",
            help = "specify key name, [default: None]" +
            " - packages are left unsigned")
    (options, args) = parser.parse_args()
    if options.key:
        BUILDCONFIG['SIGN_KEY'] = options.keydir + '\\' + \
        '%s' % options.key + '.key'
        BUILDCONFIG['SIGN_CERT'] = options.keydir + '\\' + \
        '%s' % options.key + '.crt'
        BUILDCONFIG['SIGN_PASS'] = ''
        buildconfig_save()
    s60version = BUILDCONFIG['S60_VERSION']
    # packaging for S60 3.0 and above
    y = 0
    group_dir = get_project_details('path')
    names = get_project_details('name')
    for x in group_dir:
        if options.key == 'pythonteam' and names[y] == 'python25':
            group_dir.remove(x)
            names.remove(names[y])

    for x in group_dir:
        run_in(x, 'makesis ' + names[y] + '.pkg')
        y += 1
    # If certificate and key files and passphrase has been provided,
    # sign the generated SIS packages.
    y = 0
    if not ('SIGN_CERT' in BUILDCONFIG and
            'SIGN_KEY' in BUILDCONFIG and
            'SIGN_PASS' in BUILDCONFIG):
        print"Warning! SIGN_CERT, SIGN_KEY or SIGN_PASS is not defined." + \
        "SIS packages will not be signed"
    else:
        cert = os.path.join(topdir, BUILDCONFIG['SIGN_CERT'])
        key = os.path.join(topdir, BUILDCONFIG['SIGN_KEY'])
        passphrase = BUILDCONFIG['SIGN_PASS']
        print "Signing packages with certificate %s and key %s" % (cert, key)
        for x in group_dir:
            run_in(x, 'signsis ' + names[y] +
            '.sis ' + names[y] + '.sis %s %s %s' % (cert, key, passphrase))
            y += 1
    y = 0
    for x in group_dir:
        os.chdir(topdir + '\\' + x)
        rename_file(names[y] + '.sis', names[y] +
        '_%(SDK_MARKETING_VERSION_SHORT)s.sis' % BUILDCONFIG)
        y += 1


def cmd_obb(params):
    cmd_configure(params)
    cmd_build(())
    cmd_generate_ensymble(())
    cmd_bdist_sdk(())
    cmd_bdist_sis(())


def cmd_clean(params):
    buildconfig_clean()
    global BUILDCONFIG
    build_dirs = ['', '..\\internal-src']
    BUILDCONFIG = {}
    BUILDCONFIG.update(buildconfig_defaults)
    buildconfig_save()
    for x in get_project_details('path'):
        run_in(x, 'abld -keepgoing reallyclean winscw', ignore_errors=1)
        run_in(x, 'abld -keepgoing reallyclean armv5', ignore_errors=1)
        run_in(x, 'bldmake clean', ignore_errors=1)
    for f in template_engine.templatefiles_in_tree(topdir):
        outfile = template_engine.outfilename_from_infilename(f)
        if os.path.exists(outfile):
            delete_file(outfile)
    # Delete the files created by module_config_parser
    prev_dir = os.getcwd()
    os.chdir('newcore\\Symbian\\src')
    module_config_parser.clean()
    os.chdir(prev_dir)
    for directory in build_dirs:
        for files in fileutil.all_files(directory, '*.pyc'):
            delete_file(files)

    deltree_if_exists('install')
    srcdir_base = topdir[2:]
    deltree_if_exists('\\epoc32\\build' + srcdir_base)


    buildconfig_clean()


def cmd_setcaps(params):
    srcdir_base = topdir[2:]
    parser = OptionParser()
    parser.add_option("-c", "--caps", dest = "dll_caps",
        default = 'LocalServices NetworkServices ReadUserData ' +
                  'WriteUserData UserEnvironment',
        help = "Specify DLL capability within quotes [default: %default]")
    parser.add_option("--exe-caps", dest = "exe_caps",
           help = "Specify EXEs capability within quotes. If nothing " +
           "is specified this will be same as DLL capabilities")
    (options, args) = parser.parse_args()
    if not options.dll_caps:
        print '''*** Error: Incorrect arguments
        Usage: setcaps [SETTING=value ...]
        prerequisite: binaries for the device must be built'''
        return
    else:
        buildconfig_load()
    BUILDCONFIG['DLL_CAPABILITIES'] = options.dll_caps
    if not options.exe_caps:
        BUILDCONFIG['EXE_CAPABILITIES'] = options.dll_caps
    else:
        BUILDCONFIG['EXE_CAPABILITIES'] = options.exe_caps
    buildconfig_save()
    input_dir = '\\epoc32\\build' + srcdir_base
    input_platform_build = '%(DEVICE_PLATFORM)s.%(DEVICE_BUILD)s' % BUILDCONFIG
    output_dir = '\\Epoc32\\release\\%(DEVICE_PLATFORM)s\\%(DEVICE_BUILD)s\\' \
                 % BUILDCONFIG

    def set_capas_of_matching_files(regex, capas):
        input_files = files_matching_regex(input_dir, regex)
        if not input_files:
            print '*** Error: binaries for the device must be built'
            return
        for x in input_files:
            setcapas(capas = capas,
                     compression_type = BUILDCONFIG['COMPRESSION_TYPE'],
                     output = os.path.join(output_dir, os.path.basename(x)),
                     verbose = 1)
    set_capas_of_matching_files('.*' + input_platform_build + '.*(pyd|dll)$',
                                BUILDCONFIG['DLL_CAPABILITIES'])
    set_capas_of_matching_files('.*' + input_platform_build + '.*exe$',
                                BUILDCONFIG['EXE_CAPABILITIES'])


def cmd_generate_docs(params):
    """Generate html documentation."""
    # Copy newcore source for Doc generation.
    vmware_share_dir = "c:\\python_share"
    if os.path.exists(vmware_share_dir + "\\newcore"):
        shutil.rmtree(vmware_share_dir + "\\newcore")
    newcore_dir = os.path.abspath(os.path.join(topdir, 'newcore'))
    run_cmd('python setup.py configure --do-not-compile-pyfiles')
    shutil.copytree(newcore_dir, vmware_share_dir + "\\newcore")

    print "Generating Python docs"
    html_dir = vmware_share_dir + "\\html"
    log_file = vmware_share_dir + "\\build.log"
    newcore_src = vmware_share_dir + "\\newcore"

    parser = OptionParser()
    parser.add_option("--work-area", dest = "work_area", default = 'Build',
                help = "Path where generated docs will be placed" +
                       "(relative to the src directory) [default: %default]")
    (options, args) = parser.parse_args()
    work_area = os.path.join(topdir, options.work_area)

    # Clean dirs
    if os.path.exists(html_dir):
        shutil.rmtree(html_dir)

    print "Invoking vmware image to build Python docs"
    os.system("call C:\\Ubuntu-8.10\\Ubuntu.vmx")

    if os.path.exists(html_dir):
        shutil.copytree(html_dir, work_area + "\\doc")
        shutil.rmtree(html_dir)
    else:
        print "Python doc not generated"

    if os.path.exists(log_file):
        log_text = file(log_file).read()
        print log_text
        os.remove(log_file)


def cmd_help(params):
    print '''Usage: %s <command> [<options>]
Commands:
    configure -s <SDK> [options]
        Configure the source for the given SDK. This must be done before build.
        For more options try python setup.py configure --help
    build [<subsystem>] [<additional abld parameters> ...]
        Build for device and emulator, with dependency checking. If subsystem
        is given, compile just that module. If the directory structure for the
        subsystem is given then it should be relative to src directory.
        For eg : python setup.py build ext\\amaretto\\calendar\\group
        If just the subsystem name is specified then the default lookup path
        would be ext\\<subsystem>. The bld.inf should be present under the
        group directory of the <subsystem>.
        Use --device [<subsystem>] [<additional abld parameters> ...] or
        --emu [<subsystem>] [<additional abld parameters> ...] option to
        build just for the device or the emulator. <subsystem> lookup path is
        the same as mentioned in the build command.
    generate_ensymble
        Generates the ensymble file
    generate_docs [--work-area=work_area_dir]
        Generate html documentation and place it in directory specified
        using --work-area option. A zip file of the doc is also generated.
    bdist_sdk
        Build a binary SDK ZIP package.
    bdist_sis
        Build a SIS package of the compiled files.
    obb <SDK configuration> [SETTING=value ...]
        "One-button build": do configure, build, bdisk_sdk and bdist_sis.

    clean
        Cleans everything configure and build did.

    setcaps [SETTING=value ...]
        Applies the set of capabilities to deliverables without
        recompiling them.
        Prerequisite: binaries for the device must be built
    test [options]
        Runs test cases automatically on emulator.
        This calls regrtest.py internally.
        Pre-requisite: source code must be configured and built for emulator.

    Group of test cases in a single interpreter instance:
        Use "--use-testsets-cfg" option to run a group of
        test cases in a single
    interpreter instance. The test case names should to be added in
    testapp\\src\\testsets.cfg file using "<<<<TestCase<<<<"
    and ">>>>TestCase>>>>" to seperate each set of test cases.
    Example: testsets.cfg
    <<<<TestCase<<<<
    test_cpickle
    test_cmath
    test_math
    >>>>TestCase>>>>
    <<<<TestCase<<<<
    test_StringIO
    >>>>TestCase>>>>

        setup.py test --use-testsets-cfg testsets.cfg

    Use "--testset-size" option to look through the Lib/test directory,
    find all test cases and automatically run N test cases at a time.
    Ex:
           setup.py test --testset-size 10

    Pass options to regrtest.py:
    Supports all the options of regrtest.py.
    For more information execute: setup.py test -h

    Use "--sdk-version"  option to specify the sdk version
    Ex:
           setup.py test --sdk-version _3_2

    test_device_local
        Run test cases automatically on the device connected to the local \
machine. Refer AUTOMATION_SETUP.doc under test\\ats_automation
directory for details.

    test_device_remote
        Run test cases automatically on the device connected to the CATS server

    coverage
        Get function coverage for the test cases run by regrtest. \
Modify tools\ctc.cfg for selective instrumentation. \
Results are saved at the location \src\Build\code_coverage.

Examples:

    setup.py configure -s 26
        Configure for S60 version 2.6.
    setup.py obb 28cw
        Configure, build and package for S60 SDK 2.8.
    setup.py test -v
        Run tests in verbose mode with output to stdout
''' % sys.argv[0]


if __name__ == '__main__':
    if len(sys.argv)<2:
        cmd_help(())
        sys.exit(2)

    #resolve the projects needed for release and integration builds
    cmd = sys.argv[1]
    funcname = 'cmd_' + cmd
    if hasattr(sys.modules['__main__'], funcname):
        try:
            getattr(sys.modules['__main__'], funcname)(sys.argv[2:])
    #it is coming only for setup.py configure --help
        except SystemExit:
            raise
        except:
            traceback.print_exc()
            print "*** BUILD FAILED ***"
            sys.exit(1)
    else:
        print "Unknown command %s" % cmd
        cmd_help(())
        sys.exit(2)
