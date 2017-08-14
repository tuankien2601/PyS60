PYTHON FOR S60
==============

Python(r) for S60 is a scripting language environment for the S60 smartphone
platform, based on Python 2.5.4.

Note: This README instructs how you can build Python for S60. If you are only
developing Python scripts for a Nokia device you can use the existing binary
builds for development - in this case building PyS60 from source code is not
needed.


TRADEMARKS
==========

Python and the Python logo are registered trademarks of the Python
Software Foundation.


LICENSING
=========

Copyright (c) 2005-2009 Nokia Corporation. This is Python for S60 created by
Nokia Corporation. The original software, including modifications of Nokia
Corporation therein, is licensed under the applicable license(s) for
Python 2.5.4, unless specifically indicated otherwise in the relevant source
code file.

See http://www.apache.org/licenses/LICENSE-2.0
and http://www.python.org/2.5.4/license.html


CHANGES TO PYTHON 2.5.4
=======================

See the file changes.txt for a list of changes to Python 2.5.4.


REQUIREMENTS
============

To build PyS60, you need:

- Python 2.5.4
- Open C/C++ Plug-ins version 1.6 available at
  http://www.forum.nokia.com/info/sw.nokia.com/id/91d89929-fb8c-4d66-bea0-227e42df9053/Open_C_SDK_Plug-In.html   
- S60 C++ SDK. Currently supported versions for building are:
  - S60 SDK 3.0, S60 SDK 3.2 and S60 SDK 5.0
   - To patch the SDK's headers you need the GNU patch utility.
   
Adding support for other SDK versions is probably not very
difficult. See the beginning of setup.py for the configuration
parameters required.


Requirements for extensions
============================

sensor
------

The Sensor API plug-in from Forum Nokia is needed for building sensor module on
all S60 3rd Edition SDKs. On S60 5th Edition Sdk, the module can be built
without any plug-in

"Sensor Plug-in for S60 3rd Edition SDK for Symbian OS, for C++, MR, for Nokia
5500 Sport"

Available from:
http://www.forum.nokia.com/info/sw.nokia.com/id/4284ae69-d37a-4319-bdf0-d4acdab39700/Sensor_plugin_S60_3rd_ed.exe.html

Sensor API Plug-in for S60 3rd Edition SDK for Symbian OS, Feature Pack 2

Available from:
http://www.forum.nokia.com/info/sw.nokia.com/id/8059e8ae-8c22-4684-be6b-d40d443d7efc/Sensor_API_Plug_in_S60_3rd_FP2.html

The sensor extension has been tested in Nokia N95 and 5800 XpressMusic. The 
sensor module cannot be loaded on the emulator because of the limitations in the
sensor plug-in.

There are 2 sensor modules available
a. src\ext\amaretto\sensor - for S60 3rd Edition SDK and S60 3rd Edition SDK FP1
b. src\ext\sensorfw - for S60 3rd Edition SDK FP2 and S60 5th Edition SDK

You can compile the code without the sensor module by removing the respective
mmp file entry from \src\newcore\Symbian\group\bld.inf.in.template.

Extension
---------

The C++ extensions plug-ins for S60 3rd Edition SDKs extend the features of an
S60 platform SDK, offering improved device emulation and support for additional
functionality included in shipped devices or specific to a certain device.
As a result, the plug-ins enable a greater range of applications to be built and
tested using the S60 device emulator supplied in the SDK. The Extension plug-in
is available at this URL:
http://www.forum.nokia.com/info/sw.nokia.com/id/48a93bd5-028a-4b3e-a0b1-148ff203b2b3/Extensions_plugin_S60_3rd_ed.html


FIXING THE S60 3rd edition HEADERS
==================================

There are some bugs in the shipped SDK headers. To fix these, you must
apply the patch pys60-fix-3rded-sdk.diff. To do this, you will need
the GNU patch utility.

You can patch the headers by going to the \epoc32\include directory on
your SDK drive and giving the command:

  patch -p1 < (path to the diff file)\src\misc\pys60-fix-3rded-sdk.diff

You need to do this only once.


FIXING THE x86 compiler build issues
====================================
Python source code doesn't get compiled using the latest x86 compiler 
(build > 471). Work around would be is to download x86 compiler of build 471 
from the link "http://tools.ext.nokia.com/download/beta_build.php" and extract 
the same to epoc32 folder.


COMPILING
=========

- The build system assumes that it is being run from a subst'ed drive
pointed at the root of your SDK and that EPOCROOT is \. For example if
you are using S60 SDK 3.0, you can create a substed drive T: with the
command:

  subst t: C:\Symbian\9.1\S60_3rd_MR

- Make sure that Python 2.5.4 is in your PATH.


COMPILING USING setup.py
===========================

Configuring
------------

usage:python setup.py configure [options]
For more information about options check the command line help
ex: python setup.py configure --help

If any command line option is absent default values will be used for that.
- To configure the source for a particular SDK, run

  setup.py configure -s <your SDK> [options]

Capabilities
------------

The capabilities assigned to different packages are given as build
parameters. The default set of capabilities in the setup.py is the 3.2 device 
capability set which is available using a self-signed certificate. This set is:

  LocalServices NetworkServices ReadUserData WriteUserData UserEnvironment Location  

If you are using a self-signed certificate, the maximum set of
capabilities is:
3.0 devices:
  LocalServices NetworkServices ReadUserData WriteUserData UserEnvironment
3.2 and 5.0 devices:
  LocalServices NetworkServices ReadUserData WriteUserData UserEnvironment Location

If you have a devcert with higher capabilities or if you want to build
a self-signed package with only user-grantable capabilities, then you
can redefine this with the following build parameters:

DLL_CAPABILITIES: the capabilities assigned to the DLL's contained in the Python runtime package.
Examples:

  setup.py configure -s 30armv5 -k selfsigned --caps "LocalServices NetworkServices ReadUserData WriteUserData UserEnvironment"

This could be also done using "setcaps" option even without recompiling the source
Example:

  setup.py setcaps --caps "LocalServices NetworkServices ReadUserData WriteUserData UserEnvironment"
  
Compiling
----------

To compile for the device and the emulator, run:

  setup.py build

build_device and build_emu builds just for the device or the emulator.

Note: Unless you specify a version tag, setup.py will automatically assign a
final tag to it. You can specify the version tag using the configure command.

  setup.py configure --version-tag <version_tag>

For general instructions on porting existing extension modules to PyS60 1.9.x and
S60 editions, refer the doc http://pys60.garage.maemo.org/doc/s60/extendandembed.html 

Signing
-------

All SIS packages installed to a S60 device must be signed, and so
the setup.py command bdist_sis will automatically sign the created SIS packages
with the given key.

You need to pass the following build parameters to configure for bdist_sis:

usage:python setup.py bdist_sis [options]

options:
  -h, --help               Show this help message and exit
  --keydir=KEYDIR          specify key path. Default is '..\keys'
  -k KEY, --key=KEY        specify key name to use,
                           Default: None - packages are left unsigned

All built SIS files are signed with the same key.


INSTALLING TO THE EMULATOR
==========================

Building the code for the emulator is sufficient and no further installation
is needed.

INSTALLING TO THE DEVICE
========================

To package the software into a SIS package for installation on your device, run

  setup.py bdist_sis

This will create a SIS package of the compiled code. The packages are signed
with the key and certificate given in the configure phase. Alternatively, you
may specify the key and key directory parameters on the bdist_sis command line.

NOTE: This command packages the code as it exists on disk at the time of its
execution. It does not recompile anything automatically. Use PyS60 Application
packager tool for packaging PyS60 applications


ONE BUTTON BUILD
================

To invoke the steps configure, build, bdisk_sdk and bdist_sis in one step, run:

  setup.py obb <your SDK> [<other configure parameters>]


ONE COMMAND TESTING
===================

TO run test cases automatically on emulator with one command, run:

   python setup.py test [options]

Runs test cases automatically on emulator. This calls regrtest.py internally.
Pre-requisite: source code must be configured and built for emulator.

Group of test cases in a single interpreter instance:
   Use "--use-testsets-cfg" option to run a group of test cases in a single
interpreter instance. The test case names needs to be added in
testapp\src\testsets.cfg file using "<<<<TestCase<<<<" and ">>>>TestCase>>>>"
to separate each set of test cases.

   Example: testsets.cfg
   <<<<TestCase<<<<
   test_cpickle
   test_cmath
   test_math
   >>>>TestCase>>>>
   <<<<TestCase<<<<
   test_StringIO
   >>>>TestCase>>>>

   python setup.py test --use-testsets-cfg testsets.cfg

Execute N tests at a time:
   Use "--testset-size" option to look through the Lib/test directory, find all
test cases and automatically run N test cases at a time.

    Example:
    setup.py test --testset-size 10

Pass options to regrtest.py:
    Supports all the options of regrtest.py. For more information execute
    setup.py test -h

If any command line option is absent default values will be used for that.

Note:
-->This command builds the test code
-->It starts up emulator and executes all the test cases that are in newcore\Lib\test
-->Puts output results in data\python\test\regrtest_emu.log.

NOTE: PyS60 source code must be built for emulator before running this command


ADDING NEW MODULES
==================

To add a new module to the distribution, you need to do the following:

- create a directory for it under ext

- take a look at one of the existing extensions to see the required
files. The inbox module is a good example. The elemlist module under extras
directory also is a good starting point.

- Use one of the following command for building just this module.
    - setup.py build <ext-module>         : Build for both emu and device
    - setup.py build_emu <ext-module>     : Build just for emu
    - setup.py build_device <ext-module>  : Build just for device

For S60 3.0 extension porting, please refer the doc http://pys60.garage.maemo.org/doc/s60/extendandembed.html


DOCUMENTATION
=============

For building the documentation and the needed prerequisites, refer to README in
src\newcore\Doc.
