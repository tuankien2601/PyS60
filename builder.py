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
#

import os
import sys
import os.path
import shutil
import time
import re
from optparse import OptionParser
sys.path.append(os.path.join(os.getcwd(), 'tools'))
import code_size
from shellutil import *
import build_utils


topdir = os.getcwd()

# Variables needed to create the package
installshield_dir = "c:\\Program Files\\InstallShield\\2009\\System"
installer_dir = topdir + '\\tools\\installer\\'
#.ism file is the project file for InstallShield
ism_path = installer_dir + "PythonForS60.ism"
release_name = "Beta"
project_configuration = "PythonForS60"


PROJ_DIR = "..\\build_dep"

# SDK install paths used by junction tool to swap SDK epoc32 folders
epoc_3_0 = 'C:\\Symbian\\9.1\\S60_3rd_MR\\epoc32'
epoc_3_1 = 'C:\\Symbian\\9.1\\S60_3_1\\epoc32'
epoc_3_2 = 'C:\\Symbian\\9.1\\S60_3rd_FP2_SDK\\epoc32'
epoc_5_0 = 'C:\\Symbian\\9.4\\S60_5th_Edition_SDK_v1.0\\epoc32'

# Please update this dictionary with all the informations required
# to configure the source for a particular flavor
variants = {'unsigned_devcert': {
                        'capas': 'LocalServices NetworkServices ReadUserData' +
                                 ' WriteUserData UserEnvironment Location' +
                                 ' PowerMgmt ProtServ SwEvent SurroundingsDD' +
                                 ' ReadDeviceData WriteDeviceData TrustedUI'},
                'unsigned_alabs': {
                        'capas': '''ALL -TCB -DRM -AllFiles''',
                        'exe_capas': 'LocalServices NetworkServices' +
                                 ' ReadUserData WriteUserData' +
                                 ' UserEnvironment'},
                'alabs_pythonteam': {
                        'capas': '''ALL -TCB -DRM -AllFiles''',
                         'key': 'pythonteam'},
                'selfsigned': {
                        'capas': 'LocalServices NetworkServices' +
                                 ' ReadUserData WriteUserData' +
                                 ' UserEnvironment',
                        'key': "selfsigned"},
                'unsigned_3_0': {
                        'capas': 'LocalServices NetworkServices' +
                                 ' ReadUserData WriteUserData' +
                                 ' UserEnvironment',
                        'uid': '0x20022EED'},
                'unsigned_3_2': {
                        'capas': 'LocalServices NetworkServices' +
                                 ' ReadUserData WriteUserData' +
                                 ' UserEnvironment Location',
                        'uid': '0x20022EEC'},
                'white_choco': {
                        'capas': '''ALL -TCB''',
                        'key': "pythonteam"},
                'dark_choco': {
                        'capas': '''ALL -TCB''',
                        'key': "rd02"},
                'unsigned_high_capas': {
                    'capas': 'LocalServices NetworkServices ReadUserData' +
                             ' WriteUserData UserEnvironment Location' +
                             ' WriteDeviceData ReadDeviceData SwEvent',
                    'uid': '0x20022EE9'},
                'high_capas_pythonteam': {
                    'capas': 'LocalServices NetworkServices ReadUserData' +
                             ' WriteUserData UserEnvironment Location' +
                             ' WriteDeviceData ReadDeviceData SwEvent',
                    'key': 'pythonteam',
                    'uid': '0x20022EE9'}}

# Update the below dictionary to change the deliverables name
deliverables_name = {
  '30armv5': {'python_dll_sis_name': ['Python25_3rdEd.SIS',
                                      'Python_%s%s_3rdEd_%s.sis',
                                      '\\newcore\\symbian\\group'],
              'testapp_sis_name': ['testapp_3rdEd.SIS',
                                   'testapp_%s%s_3rdEd_%s.sis',
                                   '\\ext\\test\\testapp\\group', 'test'],
              'sdk_zip_name': ['Python_SDK_3rdEd.zip',
                               'Python_%s%s_SDK_3rdEd.zip'],
              'run_testapp_sis_name': ['run_testapp_3rdEd.SIS',
                                       'run_testapp_%s%s_3rdEd_%s.sis',
                                       '\\ext\\test\\run_testapp\\group',
                                       'test'],
              'run-interpretertimer_sis_name':
                                  ['run-interpretertimer_3rdEd.SIS',
                                   'run-interpretertimer_%s%s_3rdEd_%s.sis',
                                   '\\ext\\test\\run-interpretertimer\\group',
                                   'test'],
              'interpreter-startup_sis_name':
                                  ['interpreter-startup_3rdEd.SIS',
                                   'interpreter-startup_%s%s_3rdEd_%s.sis',
                                   '\\ext\\test\\interpreter-startup\\group',
                                   'test']},
  '50armv5': {'python_dll_sis_name': ['Python25_5thEd.SIS',
                                 'Python_%s%s_5thEd_%s.sis',
                                 '\\newcore\\symbian\\group'],
         'testapp_sis_name': ['testapp_5thEd.SIS',
                              'testapp_%s%s_5thEd_%s.sis',
                              '\\ext\\test\\testapp\\group',
                              'test'],
         'run_testapp_sis_name': ['run_testapp_5thEd.SIS',
                                  'run_testapp_%s%s_5thEd_%s.sis',
                                  '\\ext\\test\\run_testapp\\group',
                                  'test'],
         'run-interpretertimer_sis_name':
                                ['run-interpretertimer_5thEd.SIS',
                                 'run-interpretertimer_%s%s_5thEd_%s.sis',
                                 '\\ext\\test\\run-interpretertimer\\group',
                                 'test'],
         'interpreter-startup_sis_name':
                                ['interpreter-startup_5thEd.SIS',
                                 'interpreter-startup_%s%s_5thEd_%s.sis',
                                 '\\ext\\test\\interpreter-startup\\group',
                                 'test'],
         'sdk_zip_name': ['Python_SDK_5thEd.zip',
                          'Python_%s%s_SDK_5thEd.zip']},
  '31': {'sdk_zip_name': ['Python_SDK_3rdEd.zip',
                          'Python_%s%s_SDK_3rdEdFP1.zip']},
  '32': {'python_dll_sis_name': ['Python25_3rdEdFP2.SIS',
                                 'Python_%s%s_3rdEd_%s.sis',
                                 '\\newcore\\symbian\\group'],
         'testapp_sis_name': ['testapp_3rdEdFP2.SIS',
                              'testapp_%s%s_3rdEd_%s.sis',
                              '\\ext\\test\\testapp\\group', 'test'],
         'sdk_zip_name': ['Python_SDK_3rdEdFP2.zip',
                          'Python_%s%s_SDK_3rdEdFP2.zip'],
         'run_testapp_sis_name': ['run_testapp_3rdEdFP2.SIS',
                                  'run_testapp_%s%s_3rdEd_%s.sis',
                                  '\\ext\\test\\run_testapp\\group',
                                  'test'],
         'run-interpretertimer_sis_name':
                                  ['run-interpretertimer_3rdEdFP2.SIS',
                                   'run-interpretertimer_%s%s_3rdEd_%s.sis',
                                   '\\ext\\test\\run-interpretertimer\\group',
                                   'test'],
         'interpreter-startup_sis_name':
                                  ['interpreter-startup_3rdEdFP2.SIS',
                                   'interpreter-startup_%s%s_3rdEd_%s.sis',
                                   '\\ext\\test\\interpreter-startup\\group',
                                   'test']}}
# TO-Do
# Generic method to log the PyS60 build errors


def generic_errors(args, __func__):
    try:
        __func__(path)
    except OSError, (errno, strerror):
        print ERROR_STR % {'*** Error': strerror}


def create_readybuild_dirs():
    # Method that create a clean directory to store the deliverables
    # --work-area option can be used in command to change the deliverable path
    # Ex- builder.py --work-area c:\\temp\\readybuild
    dir_path = os.path.abspath(python_readybuild_dir)
    deltree_if_exists(dir_path)
    os.makedirs(dir_path + '\\test')


def move_deliverable_to_readybuild_dir(platform, flavor):
    # Method that renames all the deliverables to PyS60 standard and moves to
    # deliverable directory
    platform_name = deliverables_name[platform]
    for deliverable in platform_name.keys():
        if deliverable == 'sdk_zip_name':
            continue
        sis_name = platform_name[deliverable][0]
        final_sis_name = platform_name[deliverable][1] \
        %(version, version_tag, flavor)
        os.chdir(topdir + platform_name[deliverable][2])
        if os.path.exists(sis_name):
            rename_file(sis_name, final_sis_name)
        else:
            continue
        if 'test' in platform_name[deliverable]:
            output_dir = python_readybuild_dir + '\\test'
        else:
            output_dir = python_readybuild_dir
        shutil.move(final_sis_name,
                 '%s\\%s' % (output_dir, final_sis_name))
    os.chdir(topdir)


def do_clean_and_build_emu(flavor, platforms, run_regrtest=True):
    # Method that compiles source for emulator environment
    # for given list of flavors
    global setup_configure
    for platform in platforms:
        run_cmd('python setup.py clean', exception_on_error=0)
        do_setupdotpy_configure(flavor, platform)
        run_cmd('python setup.py build --emu')

        if run_regrtest:
            run_cmd('python setup.py test --testset-size 350')


def create_and_move_device_deliverables(platform, flavor):
    # Method that compiles source for device environment for
    # given list of flavors and moves the sis files to work area directory
    if not 'key' in variants[flavor]:
        run_cmd('python setup.py bdist_sis')
    else:
        run_cmd('python setup.py bdist_sis --keydir %s --key %s' \
         % (key_dir, '%(key)s' % variants[flavor]))
    if flavor == 'unsigned_alabs':
        cert = os.path.join(topdir, key_dir, 'pythonteam.crt')
        key = os.path.join(topdir, key_dir, 'pythonteam.key')
        passphrase = ''
        os.chdir(topdir +
                 deliverables_name[platform]['python_dll_sis_name'][2])
        final_name = deliverables_name[platform]['python_dll_sis_name'][1] %\
                    (version, version_tag, 'alabs_pythonteam')
        run_cmd('signsis ' +
                deliverables_name[platform]['python_dll_sis_name'][0] + ' ' +
                final_name + ' %s %s %s' %
               (cert, key, passphrase))
        shutil.move(final_name,
                 '%s\\%s' % (python_readybuild_dir, final_name))
        os.chdir(topdir)
    move_deliverable_to_readybuild_dir(platform, flavor)


def do_setupdotpy_configure(flavor, platform):
    # Method that calls setup.py configure with arguments based on arguments
    # passed to builder.py
    global setup_configure
    setup_configure = "python setup.py configure --sdk %s --version %s \
        --version-tag %s --build-profile %s" \
        % (platform, version, version_tag, build_profile)
    if include_internal_src:
        setup_configure += " --include-internal-src"
    setup_configure += " --compression-type=" + compression_type
    if profiler:
        setup_configure += " --profile_log "
    if compiler_flags:
        setup_configure += ' --compiler-flags "' + compiler_flags + '"'
    if not 'key' in variants[flavor]:
        setup_configure += ' --caps="' + variants[flavor]['capas'] + '"'
        if 'exe_capas' in variants[flavor]:
            setup_configure += " --exe-caps " + variants[flavor]['exe_capas']
    else:
        setup_configure += " --keydir " + key_dir + " --key " \
        + variants[flavor]['key']
    if internal_proj:
        # Add the internal projects to the build
        setup_configure += " --internal-projects"
    run_cmd(setup_configure)
    log('''builder.py - Building platform: %s and flavor: %s''' %
        (platform, flavor))


def create_deliverables_using_elftran(platform, flavors, create_sis):
    # Creates the different flavors of PyS60, using postlinker tool ELFTRAN
    for flavor in flavors:
        log('builder.py - Using ELFTRAN to generate flavor: %s' % flavor)
        cmd = 'python setup.py setcaps -c "%(capas)s"' % variants[flavor]
        if 'exe_capas' in variants[flavor]:
            cmd += ' --exe-caps "' + variants[flavor]['exe_capas'] + '"'
        run_cmd(cmd)
        if create_sis:
            create_and_move_device_deliverables(platform, flavor)


def build_device_and_move_sis(flavors, platforms, create_sis=True):
    # Builds the source for device(unsigned_high_capas variant) and creates
    # the flavors specified using elftran
    for platform in platforms:
        do_setupdotpy_configure('unsigned_high_capas', platform)
        run_cmd('python setup.py build --device')
        create_deliverables_using_elftran(platform, flavors, create_sis)


def create_and_move_zip(platforms):
    global create_temp_sdk_zip

    def create_sdk_zip(platforms, armv5_pyds=False):
        # Creates the sdk zip and moves it to work area directory
        pyds_opt = ""
        final_sdk_zip_name = ""
        if armv5_pyds:
            pyds_opt = " --create-temp-sdk-zip"
            final_sdk_zip_name = 'Python_SDK_3rdEd_temp.zip'
        else:
            final_sdk_zip_name = deliverables_name[platforms]['sdk_zip_name'][1]\
                                                  % (version, version_tag)

        run_cmd('python setup.py bdist_sdk' + pyds_opt)
        sdk_zip_name = deliverables_name[platforms]['sdk_zip_name'][0]
        rename_file(sdk_zip_name, final_sdk_zip_name)
        shutil.move(final_sdk_zip_name,
                     '%s\\%s' % (python_readybuild_dir, final_sdk_zip_name))

    if create_temp_sdk_zip and platforms == '30armv5':
       create_sdk_zip(platforms)
       create_sdk_zip(platforms, True)
    else:
       create_sdk_zip(platforms)

def init_args(params):
    # Parse all the arguments using optparse module
    global platforms
    global target
    global version
    global version_tag
    global platforms
    global python_readybuild_dir
    global key_dir
    global gflavors
    global generate_docs
    global grelease
    global profiler
    global compiler_flags
    global integration_bld
    global compression_type
    global internal_proj
    global without_src_zip
    global without_ensymble
    global installer
    global include_internal_src
    global build_profile
    global create_temp_sdk_zip

    # Set default build profile to 'integration'
    build_profile = 'integration'

    parser = OptionParser()
    default_version_tag = 'svn' + get_svn_revision()
    parser.add_option("-p", "--profile_log", dest="profiler",
                        action="store_true", default=False,
                        help="Profile log generator" +
                        "for python[default: %default]")
    parser.add_option("-v", "--version", dest="version", default='2.0.0',
                        help="Python release version [default: %default]")
    parser.add_option("-r", "--version-tag", dest="version_tag",
                        default=default_version_tag,
                        help="Release tag [default: %default]")
    parser.add_option("--compiler-flags", dest="compiler_flags",
                        default='',
                        help="Compiler flags to be used while building the" +
                        " python interpreter core [default: %default]")
    parser.add_option("-t", "--targets", dest="targets", default='all',
                        help="Build target emu/device [default: %default]")
    parser.add_option("-s", "--sdk", dest="platforms", default='50armv5',
                        help="Configure the source for a given SDK." +
                        "Options : %s"%', '.join(deliverables_name.keys())+
                        ' [default: %default]')
    parser.add_option("-f", "--flavors", dest="flavors",
                        help="Choose any set of flavors for build " +
                        "%s [default: all the flavors]" \
                        % ','.join(variants.keys()))
    parser.add_option("--work-area", dest="work_area", default='build',
                        help="Path for deliverables [default: %default]")
    parser.add_option("--keydir", dest="key", default='..\\keys',
                        help="Key path")
    parser.add_option("-d", "--docs", action="store_true", dest="doc",
                        default=False,
                        help="Compiles the documentation and moves the html" +
                        "files to work_area\docs [default: %default]" +
                        "Read README.txt for more information.")
    parser.add_option("--include-internal-src", action = 'store_true',
                        dest = "include_internal_src", default = False,
                        help = "Include the source under ..\internal-src for" +
                        " build. [default: %default]")
    parser.add_option("--integration-build", action="store_true",
                        dest="integ_bld", default=False,
                        help="Does an integration build. This also sets the" +
                        " --include-internal-src option. " +
                        "[default: %default]")
    parser.add_option("--internal-projects", action="store_true",
                        dest="internal_proj", default=False,
                        help="Builds internal projects like " +
                        "interpreter-startup, run_testapp " +
                        "etc... [default: %default]")
    parser.add_option("--release-build", action="store_true",
                        dest="release", default=False,
                        help="Build all the deliverables required for " +
                        "releasing 1.9.x. This also sets the" +
                        " --include-internal-src option. " +
                        "[default: %default]")
    parser.add_option("--compression-type", dest="compression_type",
                        default='',
                        help="Modify the compression type of all the " +
                        "E32Image files generated, using 'elftran " +
                        "-compressionmethod'. Refer elftran help for valid " +
                        "compression types. If the type is given as '', " +
                        "elftran is not invoked for modifying the " +
                        "compression type. [default: '']")
    parser.add_option("--without-src-zip", action="store_true",
                        dest="without_src", default=False,
                        help="Does not create a source zip when doing a " +
                        "--release-build [default: %default]")
    parser.add_option("--without-ensymble", action="store_true",
                        dest="without_ensymble", default=False,
                        help="Disables the ensymble build [default: %default]")
    parser.add_option("--installer", action="store_true",
                        dest="installer", default=False,
                        help="Builds and creates a Setup.exe for PYS60 " +
                        "[default: %default]")
    parser.add_option("--create-temp-sdk-zip", action="store_true",
                        dest="create_temp_sdk_zip", default=False,
                        help="Creates an sdkzip with all armv5 binaries" +
                        "[default: %default]")

    (options, args) = parser.parse_args()
    target = options.targets
    version = options.version
    version_tag = options.version_tag

    platforms = options.platforms.split(',')
    #python_readybuild_dir = options.work_area
    python_readybuild_dir = topdir + '\\build'
    key_dir = options.key
    generate_docs = options.doc
    grelease = options.release
    if options.flavors:
        gflavors = options.flavors.split(',')
    else:
        gflavors = variants.keys()
    profiler = options.profiler
    compiler_flags = options.compiler_flags
    without_src_zip = options.without_src
    integration_bld = options.integ_bld
    internal_proj = options.internal_proj
    without_ensymble = options.without_ensymble
    installer = options.installer
    compression_type = options.compression_type
    create_temp_sdk_zip = options.create_temp_sdk_zip

    include_internal_src = False
    if options.include_internal_src:
        include_internal_src = True

    if integration_bld:
    #platforms, gflavors, version_tag needs to be updated for this build
        set_integration_env()


class RunBuildCmd(Thread):

    def __init__(self, build_cmd):
        Thread.__init__(self)
        self.build_cmd = build_cmd

    def run(self):
        run_cmd('python setup.py ' + self.build_cmd)


class BuildInstaller(Thread):

    def __init__(self):
        Thread.__init__(self)

    def run(self):
        print "Building the Installer Package ..."

        package_files = \
                {'..\\tools\\py2sis\\ensymble\\':
                              ['ensymble.py', 'templates', 'README',
                               'module-repo'],
                 '..\\tools\\py2sis\\ensymble_ui\\images\\':
                              ['python_logo.gif'],
                 '..\\tools\\py2sis\\ensymble_ui\\':
                              ['ensymble_gui.pyw', 'ensymble_ui_help.html'],
                 topdir + '\\tools\\installer\\doc\\':
                              ['Quickguide.html', 'python_logo.PNG']}

        package_dir = topdir + "\\build\\PythonForS60"

        # Files which are present in the Windows Installer setup but which
        # should not be present in the Linux/Mac package
        files_not_in_pys60_archive = {topdir +
                               '\\build\\PythonForS60_package\\PythonForS60\\':
                               ['Quickguide.html', 'ensymble_gui.pyw',
                                'ensymble_ui_help.html',
                                'python_logo.PNG']}
        package_dependencies_dir = "C:\\Installer_Dependency\\PyS60Dependencies"
        # These files are copied to the local directory pointed
        # by `PyS60Dependencies` for packaging as dependency files.
        package_dependencies = [
            'Python_%s%s_3rdEd_alabs_pythonteam.sis' % (version, version_tag),
            'Python_%s%s_3rdEd_unsigned_alabs.sis' % (version, version_tag),
            'PythonScriptShell_%s_unsigned_3_0.sis' % version,
            'PythonScriptShell_%s_unsigned_3_2.sis' % version,
            'PythonScriptShell_%s_unsigned_high_capas.sis' % version,
            'PythonScriptShell_%s_high_capas_pythonteam.sis' % version,
            'PythonScriptShell_%s_unsigned_devcert.sis' % version]

        if not os.path.exists(package_dir):
            print "Creating the temporary PythonForS60 folder"
            os.mkdir(package_dir)

        # Copy all the files that will be part of either Installer or the zip
        # package to the PythonForS60 folder under 'src\Build'
        print "Copying the files to PythonForS60 to be picked by Installer"
        current_dir = os.getcwd()
        os.chdir(topdir + '\\build')
        for path in package_files:
            for entry in package_files[path]:
                entry_path = path + entry
                if os.path.isdir(entry_path):
                    shutil.copytree(entry_path,
                                    os.path.join(package_dir, entry))
                else:
                    shutil.copy(entry_path, package_dir)
        os.chdir(current_dir)

        # Copy the sis files to `PyS60Dependencies` folder in
        # c:\\Installer_Dependency.
        for deps in package_dependencies:
            deps_path = topdir + '\\build\\'
            shutil.copy(deps_path + deps,
                        package_dependencies_dir + '\\' + deps)

        def generate_setup():
            # Installshield setup is created by calling 'IsCmdBld.exe'and then
            # moved to build folder
            os.chdir(installshield_dir)
            print "Build the setup.exe by passing command line option"
            installer_build_cmd = 'IsCmdBld.exe -p "%s" -r "%s" -c \
                COMP -a "%s"' % (ism_path, release_name, project_configuration)
            os.system(installer_build_cmd)
            os.chdir(current_dir)
            setup_file = 'PythonForS60_%s_%s_Setup.exe' % (version, version_tag)
            setup_dir = 'PythonForS60\\%s\\Beta\\DiskImages\\DISK1' \
                        % project_configuration
            os.rename(installer_dir + '%s\\setup.exe' % setup_dir,
                    installer_dir + '%s\\%s' % (setup_dir, setup_file))
            shutil.copy(installer_dir +
                'PythonForS60\\%s\\Beta\\DiskImages\\DISK1\\%s'
                % (project_configuration, setup_file),
                os.path.join(topdir, 'build', setup_file))
            print "Installer Package Built"

        def build_pys60_package():
            # Create a Python for S60 package for Linux/Mac users
            print "Building PythonForS60 package for Linux/Mac ..."
            pys60_archive_dir = topdir + '\\build\\PythonForS60_package'
            if not os.path.exists(pys60_archive_dir):
                os.mkdir(pys60_archive_dir)

            # Make a copy of both the PythonForS60 folder and PyS60Dependencies
            # folder to create an archive
            shutil.copytree(package_dir, pys60_archive_dir + '\\' +
                                                 os.path.basename(package_dir))
            shutil.copytree(package_dependencies_dir, pys60_archive_dir +
               '\\PythonForS60\\' + os.path.basename(package_dependencies_dir))
            for path in files_not_in_pys60_archive:
                for entry in files_not_in_pys60_archive[path]:
                    entry_path = path + entry
                    os.remove(entry_path)
            create_archive_from_directory(topdir +
                '\\build\\PythonForS60_%s_%s.tar.gz' % (version, version_tag),
                pys60_archive_dir, archive_type='tar.gz')
            print "PythonForS60 package built"
            shutil.rmtree(pys60_archive_dir)

        try:
            generate_setup()
            build_pys60_package()
        finally:
            print "Cleaning the folders after Installer generation is complete"
            os.chdir(package_dependencies_dir)
            for files in package_dependencies:
                os.remove(files)

            os.chdir(current_dir)
            if os.path.exists(package_dir):
                os.system("rmdir /S/Q " + package_dir)


def call_ensymble(tmp_dir, ensymble_options, flavour):
    sign_cert = "..\\..\..\\..\\keys\\pythonteam.crt"
    sign_key = "..\\..\..\\..\\keys\\pythonteam.key"
    ensymble_cmd = "python ensymble.py py2sis %s %s" \
                    % (tmp_dir, ensymble_options)
    # The first string in 'ensymble_options' has the scriptshell sis name
    sis_name = ensymble_options.split(' ')[0]
    try:
        run_cmd(ensymble_cmd)
        merge_example_scripts(sis_name)
        if flavour == 'high_capas_pythonteam':
            run_cmd("signsis %s %s %s %s"
                    % (sis_name, sis_name, sign_cert, sign_key))
    except:
        print "Error generating PythonScriptShell sis file."
        raise
    else:
        print "PythonScriptShell sis file generated."
        shutil.move(sis_name, topdir + "\\build")


def merge_example_scripts(sis_name):
    run_cmd("dumpsis -x " + sis_name)

    prev_dir = os.getcwd()
    scriptshell_dir = os.path.join(prev_dir, sis_name.replace(".sis", ""))
    os.chdir(os.path.join(prev_dir, scriptshell_dir))

    scriptshell_pkg = sis_name.replace(".sis", ".pkg")
    temp_pkg_file = "scriptshell.pkg"

    # The pkg file generated by dumpsis command contains some non ascii
    # characters, hence copying the pkg file content to a temporary file
    # excluding those non ascii characters
    type_cmd = "type %s > %s" % (scriptshell_pkg, temp_pkg_file)
    run_cmd(type_cmd)

    f = open(temp_pkg_file, 'a')
    f.write('\n\n@"..\\..\\..\\..\\..\\internal-src\\' \
            'dependency_sis_files\\PyS60ExampleScripts.sis"' \
            ', (0x20022EEA)')
    f.close()

    delete_file(scriptshell_pkg)
    rename_file(temp_pkg_file, scriptshell_pkg)

    run_cmd("makesis " + scriptshell_pkg)
    shutil.move(sis_name, prev_dir)

    os.chdir(prev_dir)
    deltree_if_exists(scriptshell_dir)


def populate_scriptshell_scripts(tmp_dir):
    scriptshell_dir = topdir + "\\ext\\amaretto\\scriptshell\\"
    scriptshell_src = ['default.py', 'consolidated_imports.py']

    deltree_if_exists(tmp_dir)
    os.makedirs(tmp_dir)

    for f in scriptshell_src:
        print "Copying %s to %s" % (scriptshell_dir + f, tmp_dir)
        shutil.copy(scriptshell_dir + f, tmp_dir)
    # filebrowser.py imports dir_iter.py. So moving this file to scriptshell's
    # private directory.
    ex_file = topdir + "\\extras\\" + 'dir_iter.py'
    shutil.copy(ex_file, tmp_dir)


def create_sdk_zip_31():
    print "Creating sdk zip for 3.1 .."
    platforms = ['30armv5']
    build_utils.create_clean_env('\\Epoc32_3.1.zip')
    sdk_zip_name = deliverables_name['32']['sdk_zip_name'][1] % \
                                          (version, version_tag)
    build_utils.unzip_file_into_dir(topdir + '\\build\\' + sdk_zip_name, '\\')
    # Building only the socket
    do_setupdotpy_configure('unsigned_high_capas', platforms[0])
    run_cmd('python setup.py build ext\\amaretto\\socket\\group')
    create_and_move_zip('31')
    test_sdk_zip('_3_1')
    shutil.rmtree("\\epoc32")


def generate_scriptshell_sis():
    print "Building the PythonScriptShell sis files ..."
    prev_dir = os.getcwd()
    os.chdir(topdir + '\\tools\\py2sis\\ensymble')

    app_name = "PythonScriptShell"
    heap_size = "100K,16M"
    vendor = "Nokia"
    short_caption = "Python" + version
    caption = "PythonScriptShell"
    tmp_dir = "scriptshell_dir"

    if integration_bld:
        flavours = ['high_capas_pythonteam']
    else:
        flavours = ['unsigned_3_0', 'unsigned_3_2', 'unsigned_devcert',
                    'high_capas_pythonteam', 'unsigned_high_capas']

    populate_scriptshell_scripts(tmp_dir)

    for flavour in flavours:
        if flavour in ['unsigned_3_0', 'unsigned_3_2',
                       'high_capas_pythonteam', 'unsigned_high_capas']:
            uid = variants[flavour]['uid']
        else:
            uid = None

        common_opt = "-v --appname=%s --version=%s --heapsize=%s " + \
                     "--vendor=%s --shortcaption=%s --caption=%s "

        common_opt = common_opt % (app_name, version, heap_size, vendor,
                           short_caption, caption)
        if uid is not None:
            common_opt += ' --uid=' + uid
        caps = variants[flavour]['capas'].replace(' ', '+')
        if integration_bld:
            sis_name = "PythonScriptShell_%s%s_%s.sis" % \
                                                (version, version_tag, flavour)
        else:
            sis_name = "PythonScriptShell_%s_%s.sis" % (version, flavour)
        options_for_ensymble = "%s --caps=%s %s" % (sis_name, caps, common_opt)
        if integration_bld:
            options_for_ensymble += " --ignore-missing-deps"
        try:
            call_ensymble(tmp_dir, options_for_ensymble, flavour)
        except:
            raise

    deltree_if_exists(tmp_dir)
    os.chdir(prev_dir)


def generate_ensymble_zip():
    package_files = {'tools\\py2sis\\ensymble\\':
                     ['ensymble.py', 'templates', 'README', 'module-repo']}
    package_dir = topdir + '\\build\\ensymble_zip'
    prev_dir = os.getcwd()
    os.mkdir(package_dir)
    for path in package_files:
            for entry in package_files[path]:
                entry_path = path + entry
                if os.path.isdir(entry_path):
                    shutil.copytree(entry_path,
                                    os.path.join(package_dir, entry))
                else:
                    shutil.copy(entry_path, package_dir)
    create_archive_from_directory(topdir + '\\build\\ensymble_%s_%s.zip' %
                                  (version, version_tag), package_dir)
    deltree_if_exists(package_dir)
    os.chdir(prev_dir)


def test_sdk_zip(sdk_version):
    sign_cert = "..\\..\..\\..\\keys\\pythonteam.crt"
    sign_key = "..\\..\..\\..\\keys\\pythonteam.key"
    caps_opt = ""

    if sdk_version == '_3_0':
        elemlist_sis_name = "elemlist_%s%s_3rdEd_alabs_pythonteam.sis" \
                                                      % (version, version_tag)
        build_utils.create_clean_env('\\Epoc32_3.0.zip')
        sdk_zip_name = 'Python_%s%s_SDK_3rdEdFP2.zip' % (version, version_tag)
        caps_opt = ' --caps "%s"' % variants['alabs_pythonteam']['capas']
    elif sdk_version == '_3_1':
        elemlist_sis_name = "elemlist_%s%s_3rdEdFP1_alabs_pythonteam.sis" \
                                                      % (version, version_tag)
        build_utils.create_clean_env('\\Epoc32_3.1.zip')
        sdk_zip_name = 'Python_%s%s_SDK_3rdEdFP1.zip' % (version, version_tag)
        caps_opt = ' --caps "%s"' % variants['alabs_pythonteam']['capas']
    elif sdk_version == '_3_2':
        elemlist_sis_name = "elemlist_%s%s_3rdEdFP2_alabs_pythonteam.sis" % \
                                                        (version, version_tag)
        build_utils.create_clean_env('\\Epoc32_3.2.zip')
        caps_opt = ' --caps "%s"' % variants['alabs_pythonteam']['capas']
        sdk_zip_name = 'Python_%s%s_SDK_3rdEdFP2.zip' % (version, version_tag)

    build_utils.unzip_file_into_dir(topdir + '\\build\\' + sdk_zip_name, '\\')
    cmd_test = 'python setup.py test --testset-size 350 --sdk-version %s' % \
                                                            sdk_version
    run_cmd(cmd_test, exception_on_error=0)
    run_cmd('python setup.py configure --sdk 30armv5')
    run_cmd('python setup.py build extras\\elemlist\\group')
    run_cmd('python setup.py configure --sdk 30gcce' + caps_opt)
    run_cmd('python setup.py build extras\\elemlist\\group')

    os.chdir('extras\\elemlist\\group')
    try:
        run_cmd('makesis elemlist.pkg')
        run_cmd('signsis elemlist.sis %s %s %s' % (elemlist_sis_name,
                                                   sign_cert, sign_key))
        shutil.copy(elemlist_sis_name, '%s\\test\\%s' %
                                   (python_readybuild_dir, elemlist_sis_name))
    finally:
        os.chdir(topdir)


def release_build():
    global platforms
    global gflavors
    global without_src_zip
    global installer
    global variants
    global internal_proj
    global build_profile

    build_profile = 'release'
    #Build for emu first and then build for device
    gflavors = ['unsigned_alabs', 'alabs_pythonteam']
    platforms = ['30armv5']
    do_clean_and_build_emu('white_choco', platforms, run_regrtest=False)

    # When run-interpretertimer is built using GCCE on 3.0SDK the compilation
    # fails as it complains that the application is nested too deeply in the
    # drive. The same error is not seen on 3.2SDK. To workaround this 3.0SDK
    # quirk we don't build internal projects instead of the arduous task of
    # either renaming the exe or moving the project somewhere else.
    orig_internal_proj_value = internal_proj
    internal_proj = False
    build_device_and_move_sis(gflavors, ['30gcce'], create_sis=False)
    internal_proj = orig_internal_proj_value

    build_device_and_move_sis(gflavors, platforms, False)

    run_cmd('python setup.py generate_ensymble')

    create_and_move_zip('30armv5')

    # Building for 3.2 sdk
    platforms = ['32']
    build_utils.create_clean_env('\\Epoc32_3.2.zip')
    sdk_zip_name = deliverables_name['30armv5']['sdk_zip_name'][1] % \
                                          (version, version_tag)
    final_sdk_zip_name = deliverables_name['32']['sdk_zip_name'][1] % \
                                          (version, version_tag)
    os.rename(topdir + '\\build\\' + sdk_zip_name,
              topdir + '\\build\\' + final_sdk_zip_name)
    build_utils.unzip_file_into_dir(topdir + '\\build' +
                                    '\\Python_SDK_3rdEd_temp.zip',
                                    '\\')

    # Building only scriptext, sensorfw and iad_client on 3.2
    proj = ['..\\internal-src\\scriptext\\group', 'ext\\sensorfw\\group',
            '..\\internal-src\\iad_client\\group']
    # Configure for 3.2 sdk
    do_setupdotpy_configure('unsigned_alabs', platforms[0])
    for x in proj:
        run_cmd('python setup.py build ' + x)

    create_deliverables_using_elftran(platforms[0], gflavors, True)
    code_size.updatelog("3.0", python_readybuild_dir + "\\" +
            deliverables_name['30armv5']['python_dll_sis_name'][1] %
            (version, version_tag, gflavors[0]),
            "C:\\Program Files\\CruiseControl\\logs\\codesize_metrics.log")
    delete_file(topdir + '\\build\\Python_SDK_3rdEd_temp.zip')

    pyd_path = '\\epoc32\\release\\winscw\\udeb\\'
    zip_object = zipfile.ZipFile(topdir + '\\build\\' + final_sdk_zip_name,
                                 'a')
    zip_object.write(pyd_path + 'kf_scriptext.pyd',
                     pyd_path + 'kf_scriptext.pyd')
    zip_object.close()

    run_cmd('python setup.py generate_ensymble')
    generate_scriptshell_sis()
    test_sdk_zip('_3_2')
    thread_pool = []
    cmd_remote_buid = 'test_device_remote --work-area ' + \
                       python_readybuild_dir
    build_commands = ['generate_docs', 'coverage', cmd_remote_buid]

    for x, command in enumerate(build_commands):
        thread_pool.append(RunBuildCmd(command))
        thread_pool[x].setName(command)
    for threads in thread_pool:
        threads.start()
    for threads in thread_pool:
        threads.join()
        if threads.getName() == 'generate_docs':
            t = BuildInstaller()
            t.start()
            t.join()
    create_sdk_zip_31()
    build_utils.create_clean_env('\\Epoc32_3.0.zip')
    if without_src_zip == False:
        # Create src zip
        zipname = '%s\\' % (python_readybuild_dir) + \
                  'pys60-%s%s_src.zip' % (version, version_tag)
        create_archive_from_directory(zipname, os.getcwd(), 'src')


def set_integration_env():
    global gflavors
    global version_tag
    global variants
    # 3.2 devices have 'Location' capability as user-grantable using selfsigned
    # certificate
    if '30armv5' not in platforms:
        variants['selfsigned']['capas'] += ' Location'
    gflavors = ['alabs_pythonteam', 'unsigned_alabs']
    svn_revision = get_svn_revision()
    version_tag = 'svn' + svn_revision + '-integration'


def get_svn_revision():
    out_put = run_shell_command('svnversion ' + PROJ_DIR)
    [version, n] = out_put['stdout'].rsplit("\r")
    return version


def del_mod_repo(topdir):
    deltree_if_exists(os.path.abspath('tools\\py2sis\\ensymble\\module-repo'))


def integration_build():
    global build_profile
    del_mod_repo(topdir)

    build_profile = 'integration'

    do_clean_and_build_emu('white_choco', platforms)
    build_device_and_move_sis(gflavors, platforms)
    thread_pool = []
    build_commands = []
    if generate_docs:
        build_commands.append('generate_docs')
    if not without_ensymble:
        build_commands.append('generate_ensymble')
    for command in build_commands:
        thread_pool.append(RunBuildCmd(command))
    for threads in thread_pool:
            threads.start()
    for threads in thread_pool:
            threads.join()
    generate_ensymble_zip()
    generate_scriptshell_sis()
    for platform in platforms:
        create_and_move_zip(platform)
    if installer:
        t = BuildInstaller()
        t.start()
        t.join()


def main():
    if grelease:
        release_build()
    elif integration_bld:
        integration_build()
    else:
        if target == 'emu':
            do_clean_and_build_emu(gflavors, platforms)
        elif target == 'device':
            build_device_and_move_sis(gflavors, platforms)
        else:
            do_clean_and_build_emu(gflavors, platforms)
            build_device_and_move_sis(gflavors, platforms)

    log('builder.py - Please find the deliverables in directory: %s'
        % python_readybuild_dir)


def show_configuration():
    log('builder.py : The following values are considered for this build.\n'+
         'Target -----------------> %s \n' % target +
         'Version ----------------> %s \n' % version +
         'Version tag ------------> %s \n' % version_tag +
         'Build profile ----------> %s \n' % build_profile +
         'Sdk --------------------> %s \n' % platforms +
         'Work area --------------> %s \n' % python_readybuild_dir +
         'Key dir ----------------> %s \n' % key_dir +
         'Flavors ----------------> %s \n' % gflavors +
         'Compile Docs flag ------> %s \n' % generate_docs +
         'Integration build flag--> %s \n' % integration_bld +
         'Release flag -----------> %s \n' % grelease +
         'Compression type--------> %s \n' % compression_type +
         'Profile log-------------> %s \n' % profiler +
         'Include ..\internal_src-> %s \n' % include_internal_src +
         'Without src zip---------> %s \n' % without_src_zip +
         'Internal Projects-------> %s \n' % internal_proj +
         'Without Ensymble--------> %s \n' % without_ensymble +
         'Create Installer--------> %s \n' % installer)


if __name__=="__main__":
    global target
    global version
    global version_tag
    global platforms
    global python_readybuild_dir
    global key_dir
    global gflavors
    global generate_docs
    global grelease
    global integration_bld

    working_dir = os.getcwd()
    init_args(sys.argv)
    create_readybuild_dirs()
    temp = time.strftime(python_readybuild_dir +
                         "\\test\\pys60_build_log_%Y%m%d_%H%M.log")
    log_file = tee(temp, 'w')
    print time.strftime("Start Time: %a, %d %b %Y %H:%M:%S")
    show_configuration()
    main()
    print time.strftime("End Time: %a, %d %b %Y %H:%M:%S")
    log_file.close()
    del tee
