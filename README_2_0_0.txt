===============================================================
Python for S60 2.0.0 on S60 3rd and higher editions, 11.02.2010
===============================================================


Contents:

1.  Introduction
2.  Overview of PyS60 on S60 3rd and higher editions
3.  Standard development lifecycle in 3rd and higher editions
3.1 Signing and distribution
3.2 Module level details
4.  File locations
5.  Capabilities
7.  PyS60 SDK package
8.  Summary
9.  Glossary


1. Introduction
---------------

This document describes the changes to Python for S60 (hereafter PyS60) needed
in order to support S60 3rd edition (hereafter also S60 3rdEd) or higher editions.
Starting from PyS60 2.0, the supported editions are only S60 3rd and higher
editions.

The new platform security (hereafter platsec) features in Symbian OS 9.x/EKA2,
and S60 3rdEd onwards require several changes to the whole PyS60 framework.
Without these modifications S60 3rd and later editions would not be supported
by PyS60. The implementation alternative selected in order to support S60 3rd
and higher editions is tightly aligned with the common EKA2 platform security
framework in order to minimize the work for a PyS60 developer. At the same time
this limits the possible security threats posed by PyS60.

The solution for PyS60 in EKA2 is based on two use cases:

  1. Stand-alone installation - in essence this makes Python applications no
  different from native Symbian applications, a user cannot tell whether this is
  a Python or C++ application. The application is visible in the device main
  menu

  2. Plain script running and the application to enable this, aka the script
  shell – the Python application seen in PyS60 1.0 onwards

In this document we provide information how the new platform security features
affect PyS60, what will be the development options and offer advices for native
extending.

For all questions and feedback, related to PyS60 and platsec, please use the
Forum Nokia discussion board:

http://discussion.forum.nokia.com/forum/forumdisplay.php?f=102

Tip: use the advanced search option with keyword "platsec" and select the Python
     forum for the search.

2. Overview of PyS60 on S60 3rd or higher edtions
---------------------------------------------------

In 3rdEd and higher edition devices, platsec is enforced. This means that all the
installed SISX files need to be signed (NB. there might be an option to install
unsignedpackages in some devices). Unsigned packages cannot be installed to a device
and neither the old style SIS packages nor the binaries packaged from 2ndEd or 1stEd
are compatible with the 3rdEd.

The software installer (hereafter SWInstall) will check if the application in
the SISX package is signed. For more information about signing, see the Section
"Signing".

A fundamental concept in platsec is 'capability' which is the term used for what
the running process can do in the device - process is the basic insulation
granularity in platsec and capabilities are forced during runtime. A capability
must be held by the executable binary if the process needs to access some
restricted resource.

Since a standalone PyS60 application is no different from a native C++
application and runs in a separate process it needs to be signed if it uses
controlled APIs or it is distributed via a SISX package.

What a Python standalone application can do will be limited by the capabilities
assigned to the interpreter DLL - these capabilities are listed in Section 5.
"Capabilities". In other words, this is the upper bound for any Python
application which uses the Nokia signed PyS60 distribution. There is of course
the possibility to sign the Python interpreter DLL for special purposes with
larger capabilities if needed but this discussion is left out from this
document.

As the Python application seen from the device main menu, aka the script shell,
is also a Python application which is available with different capability set. 
The script shell should not enable the running of scripts with large capabilities 
and thus it is not signed by Nokia with the same capabilities as the interpreter DLL.
This should not cause problems for development - a developer can sign the script shell
application with developer certificate (hereafter devcert). Due to separate
signing needs for the interpreter DLL and the script shell application, there is
a need for separate packages ('X' indicates version number):

  * Python_X.X.X.SIS - Python rutime which contains the interpreter DLL, all the Nokia
  provided native Python extensions, frequently used standard Python modules and other needed files

  * PythonScriptShell_X.X.X_3_0.sis - contains the script shell application which
  does not work without the python runtime package. This application is Nokia signed with capability set 
  ReadUserData, WriteUserData, NetworkServices, LocalServices and UserEnvironment.

  * PythonScriptShell_X.X.X_3_2.sis - contains the script shell application which
  does not work without the python runtime package. This application is Nokia signed with capability set 
  ReadUserData, WriteUserData, NetworkServices, LocalServices, UserEnvironment and Location.


  * PythonScriptShell_X.X.X_high_capas.sis - contains the script shell application which 
  does not work without the python runtime package. This application is Nokia signed with capability set 
  LocalServices, NetworkServices, ReadUserData, WriteUserData, UserEnvironment, Location, SwEvent, 
  WriteDeviceData and ReadDeviceData

  * PythonScriptShell_X.X.X_unsigned_devcert.sis - contains the script shell application,
  does not work without the python runtime package. This is an unsigned application with the capability set
  LocalServices, NetworkServices, ReadUserData, WriteUserData, UserEnvironment, Location, PowerMgmt, ProtServ, SwEvent,
  SurroundingsDD, ReadDeviceData, WriteDeviceData and TrustedUI.

A developer should keep in mind that the script shell is just a Python script,
similar to the one you package with the ensymble tool and subject to the
same security preconditions as described earlier in this document. The
interpreter DLL is the one used by all the standalone Python applications and
the entity that needs to be signed with a large set of capabilities to ensure
that individual Python applications can access the controlled resources as
freely as possible. Notice that the script shell Python application visible in
the device main menu has nothing to do with other standalone Python applications
(ie. there are no logical or conceptual dependencies).

For clarification, here is an outline of a standalone Python application in
3rdEd devices:

      default.py
           |      (wrapped together with 'foobar.exe' e.g. with ensymble py2sis)
           |
  foobar_0x01234567.exe
           |      (a simple launchpad application for interpreter creation etc.
           |       Another example is the Python_launcher.exe)
      python25.dll
           |      (shared between standalone Python applications)
           |
     location.pyd
                  (all the other native extensions are at this level also)

In the above diagram, the 'python25.dll' is signed with Nokia certificate,
and as stated previously, it provides the upper bound for what the 'foobar.exe'
can access (in the platsec sense) in the device. For the 'foobar.exe' the developer
has chosen a suitable set of capabilities limited by the developer's certificate
and/or the Python APIs utilized. The capabilities needed for the APIs are outlined in
Section 3.2.

For more information, please see the platsec material provided by Symbian and
Nokia, e.g. here is an overview of the Symbian signed process and platsec:

https://www.symbiansigned.com/How_has_Symbian_Signed_evolved_with_Symbian_OS_v9.pdf

Symbian signed
--------------

In principle, there is no problem in getting a standalone Python application to
be Symbian signed - a standalone Python application is no different from a
native C++ application.

Symbian signed Python applications are not tested yet with PyS60 2.0.

For more information, please refer to: https://www.symbiansigned.com/


3. Standard development lifecycle
---------------------------------

The S60 emulator allows unrestricted access to the platform(set PlatSecEnforcement
to OFF in epoc32\data\epoc.ini & restart the emulator) and therefore the PyS60
developer is advised to first use the emulator for overall testing of a Python script.
Useful information especially about the platform warnings can be found from the
emulator log file, located usually under directory:

c:\Documents and Settings\<USERID>\Local Settings\temp\EPOCWIND.OUT

For example, the following warning message would be emitted to the log file if a
script tries to delete a file ("traceback.pyc") under \resource:

  64.990	*PlatSec* WARNING - Capability check would have failed - A Message
  (function number=0x00000013) from Thread Python[10201510]0001::PYTHON, sent to
  Server !FileServer, was checked by Thread EFile.exe[100039e3]0001::Main and
  was found to be missing the capabilities: TCB .  Additional diagnostic
  message: \resource\traceback.pyc Used to call: Delete

This error is due to 'data caging' and the protection of folder \resource for
modifications.

The emulator can be configured also to simulate the platsec constraints, see
the SDK documentation for more information (search the SDK with "Platform
Security Tab").

In a device the following would be received if the Python script tries to write
to a restricted/not restricted location (example via Bluetooth console in Nokia
N73):

  [GCC 3.4.3 (release) (CodeSourcery ARM Q1C 2005)] on symbian_s60
  Type "copyright", "credits" or "license" for more information.
  Type "commands" to see the commands available in this simple line editor.
  >>> f=open('c:\\sys\\bin\\test.log', 'w')
  Traceback (most recent call last):
    File "<console>", line 1, in ?
  IOError: [Errno -46] : 'c:\\sys\\bin\\test.log'
  >>> f=open('c:\\Python\\test.log', 'w')
  >>> f.write('foobar')
  >>> f.close()
  >>>

The first file open fails since location c:\sys\bin is restricted in 3rdEd
devices. The second file open succeeds as this location is not controlled by
platsec - this location is also the folder for scripts seen in the script shell
application. Notice also that the platform security constraints can be handled
at Python level with e.g. try-except constructs.


3.1 Signing and distribution
----------------------------

For executing the scripts in an actual S60 3rdEd or later device there exists
numerous alternatives for signing and distribution e.g.:

  1) Using a devcert for SISX signing

  2) Self-signing the SISX

  3) Signing the Python script shell application with the above 1) 
  and installing the individual scripts with separate packages
  (which need to be signed as well)

  4) Packaging the scripts with ensymble - py2sis (and signing the SISX packages).

By following the first alternative, a developer can sign applications with
devcerts prior the official Symbian signing and test the application in
production devices with almost full capabilities. Again, applications you are
planning to distribute for 3rdEd handsets need to be signed since the platform
security restrictions are taken into use in the target handsets. For obtaining
devcerts, see:

  https://www.symbiansigned.com/app/page/devcertgeneral

For the second alternative, self-signing, please see the 3rdEd SDK documentation
for more information:

  Introduction to S60 3rd Edition >> How to Sign .sis Files

  (or search with keyword "self-sign")

In the third alternative, the Python script shell SIS package is signed with a
devcert and the individual script can be packaged to a SISX file
and e.g. Bluetooth beamed to the device. Here is an example ".pkg" file used for
generating a SISX file (for processing this file, please see the above document
about signing):

  ;
  ;Languages
  &EN
  ;
  ; The packages UID from test range
  ;
  #{"MyTestPackage"},(0xE000000F),1,0,0,TYPE=SISAPP
  %{"Vendor-EN"}
  (0x101F7961), 0, 0, 0, {"Series60ProductID"}
  ;
  ; Files to install, this file needs to be found by 'makesis.exe'.
  ; The file location on the right side is the directory seen by the script shell
  ; application in the device, you can install your scripts there for easy
  ; invocation
  ;
  "c:\src\mytest.py"       -"c:\data\Python\mytest.py"

The above example uses UID from the range 0xE0000000 - 0xEFFFFFFF, this range is
reserved for testing. For more information about UIDs, please see:

  https://www.symbiansigned.com/app/page/uidfaq

In the fourth alternative, a developer can use the ensymble program to package
individual scripts to installable SISX packages. The packages generated by
ensymble require the Python runtime sis to be installed in the device.
For details about ensymble usage, refer 'tools\py2sis\ensymble\README.txt'


3.2 Module level details
------------------------

The Python functions or modules affected by platform security are outlined in
the following table:

.--------------------------------------------------------------------------.
| Function or module       | Capabilities needed  | Devcert | Self-signing |
|--------------------------+----------------------+---------+--------------|
| location.gsm_location()* | ReadUserData,        |         |              |
|                          | ReadDeviceData,      |    X    |              |
|                          | Location             |         |      X^      |
|--------------------------+----------------------+---------+--------------|
| contacts                 | ReadUserData,        |         |      X       |
|                          | WriteUserData        |         |              |
|--------------------------+----------------------+---------+--------------|
| sysinfo.imei()           | ReadDeviceData+      |         |      X       |
|--------------------------+----------------------+---------+--------------|
| telephone                | NetworkServices      |         |      X       |
|--------------------------+----------------------+---------+--------------|
| messaging                | NetworkServices      |         |      X       |
|--------------------------+----------------------+---------+--------------|
| e32.set_home_time()      | WriteDeviceData      |    X    |              |
.--------------------------------------------------------------------------.

* = Gives false data if the executable is not signed with the specific
capabilities.

+ = Claimed by the S60 SDK but in practise self-signing is sufficient.

^ = On 3rd Ed FP2 and higher devices.

No capabilities are needed e.g. by the following extensions or self-signing is
sufficient:

  * camera
  * e32db
  * inbox
  * audio
  * socket
  * graphics


4. File locations
-----------------

In EKA2 the file locations have changed, as previously mentioned, the concept
'data caging' refers to the changed and controlled locations. The file locations are
as follows:

 c:\sys\bin

  Contains all the native extensions including all the binary launchpads for
  Python applications.

 !:\resource\python25

  Contains the Python standard library files bundled with the Nokia PyS60 SISX
  package.

 !:\private\<UID>

  Contains the "default.py" script which is the script interpreted first by the
  launchpad binaries. The <UID> is the unique identifier assigned to a Python
  application.

These locations have special constraints, see Symbian and Nokia platsec
documentation for more information. In summary, \sys\bin is the only place where
executable binaries (including DLLs) can exist, \resource can only be read, not
written to (except by TCB programs) and \private\<UID>\ is only accessible by
the process in question (and TCB programs).

Most notably this is seen in the "import" path of the interpreter. The following
is the output from the script shell application using Bluetooth console:

  >>> import sys
  >>> sys.path
  ['c:\resource\Python25\<apps UID ex e35e00df>', 'c:\resource\Python25\python25.zip',
   'c:\resource\Python25', 'c:\resource\Python25\site-packages']

Notice that the directory under !:\resource\Python25, named after the process UID
(in the above example c:\resource\Python25\e35e00df) is a new search location for
the interpreter. In this example it is the one assigned for the Python script shell
application. If you package your application with ensymble, the "import" path will
automatically contain the correct search path similar to the path above but with the UID
assigned to your application. This new search path has implications for native
extensions, see Section 6.2 for more information.


There is a new function for obtaining the process UID in PyS60:

appuifw.app.uid()

  Returns the UID, in Unicode, of the native application in whose context the
  current Python interpreter session runs.


5. Capabilities
---------------

The capabilities assigned by Nokia to PyS60 'devcert build' are as follows:

User Capabilities:

  * NetworkServices
  * LocalServices
  * ReadUserData
  * WriteUserData
  * Location
  * UserEnvironment

System Capabilities:

  * PowerMgmt
  * ReadDeviceData
  * WriteDeviceData
  * TrustedUI
  * ProtServ
  * SwEvent
  * SurroundingsDD

A Python application using the Nokia signed Python SISX package cannot have more
capabilities than in the above list, less is of course possible. If more
capabilities are needed, the Python DLL capabilities need to be changed like
stated before and the SISX package signed with a certificate with enough signing
metacapabilities. For more information, please see the 'README.txt' in the
source distribution.

The 'self-signed build' of PyS60 consists of the following capabilities:

  * LocalServices
  * NetworkServices
  * ReadUserData
  * UserEnvironment
  * WriteUserData


6. PyS60 SDK package
--------------------

The PyS60 SDK package contains both GCCE and ARMV5 link libraries. Developers have to use
either ARMV5 (RVCT 2.2) or GCCE target for compiling their native extensions
for devices.


9. Glossary
-----------

platsec
  Platform Security

EKA2
  EPOC32 Kernel Architecture 2

capability
  A capability, when hold by a running process, gives permission to access
  system resources

data caging
 The concept of dividing a file system hierarchy to generally accessible and
 restricted locations

RVCT
  RealView Compiler Tools (ARM ltd.)

pyd
  Binary Python extension (written in C/C++)

PyS60
  Python for S60. In this document, this term might refer also to the
  interpreter DLL and the native extensions (i.e. pyds)

SISX
  The new format used for SIS files by SWInstall (Software Installer). Notice
  that the file ending is still .sis

Script shell
  In this document, an S60 application visible in the device menu which enables
  a user to run individual Python scripts. Part of the Python for S60
  distribution

SWInstall
  A program running in the target device handling SISX installer packages

TCB
  From the 3rdEd SDK: "TCB stands for "Trusted Computing Base." The trusted
  computing base consists of a number of architectural elements that cannot be
  subverted and that guarantee the integrity of the device. This trusted core
  runs with "Tcb" system capability. The components with Tcb capability have
  full access to file systems including reading/writing to \sys\bin. TCB is not
  granted to third-party applications."

ensymble
  The tool to package Python scripts to SIS packages which can be installed by
  SWInstall to a Symbian OS device. The created SIS packages require that there
  is PyS60 already installed. Part of the Python for S60 distribution

appmgr
  Writes Python scripts from device inbox to the location where these can be
  interpreted by the script shell. Part of Python for S60 distribution

devcert
  Developer certificate. More APIs can be accessed when signing SISX packages
  with this certificate, for more information about this please search your SDK
  with the terms 'developer certificate' and see
  https://www.symbiansigned.com/app/page/devcertgeneral


Copyright (c) 2007-2009  Nokia Corporation. Nokia and Nokia Connecting People
are registered trademarks of Nokia Corporation.
