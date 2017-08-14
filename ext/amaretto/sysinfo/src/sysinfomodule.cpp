/* Copyright (c) 2005-2008 Nokia Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * ====================================================================
 *  sysinfomodule.cpp
 *
 *  Python API to system information, partly modified from SysInfo example 
 *  available from Forum Nokia and 
 *  http://www.newlc.com/article.php3?id_article=155
 *
 *  Implements currently following Python functions:
 * 
 *  tuple<int, int, int> os_version()
 *    major, minor, build
 * 
 *  unicode_string sw_version()
 *    hardcoded string "emulator" returned if in emulator
 *
 *  unicode_string imei()
 *    hardcoded string "000000000000000" returned if in emulator
 * 
 *  int battery()
 *    current battery level (0-7), returns always 0 if in emulator
 *
 *  int signal_bars()
 *    current signal strength (0-7), returns always 0 if in emulator
 *    
 *  int signal_dbm()
 *    current signal strength in dBm, returns always 0 if in emulator
 *   
 *  int total_ram()
 *
 *  int total_rom()
 *
 *  int max_ramdrive_size()
 *  
 *  tuple<int_x, int_y> display_twips()
 *
 *  tuple<int_x, int_y> display_pixels()
 *
 *  int free_ram()
 *
 *  int ring_type()
 *    current ringing type. Possible values in release 2.0: 0 (normal) 
 *    1 (ascending) 2 (ring once) 3 (beep) 4 (silent) 
 *
 *  int active_profile()
 *    returns the active profile. Possible values on 3rd. ed:
 *    0 (General) 1 (Silent) 2 (Meeting) 3 (Outdoor) 4 (Pager)
 *    5 (Off-line) 6 (Drive) 30-49 (User created profiles)
 *
 *  dict<unicode_string:int> free_drivespace()
 *    keys in dictionary are the drive letters followed by ':', values
 *    are the amount of free space left on the drive in bytes e.g. 
 *    {u'C:' 100} 
 *      
 * TODO
 *    - Only works from one process at time.
 *
 * ====================================================================
 */

#include "Python.h"
#include <pythread.h>
#include "symbian_python_ext_util.h"

#include <sysutil.h>    // OS, SW info
#include <hal.h>        // HAL info
#if SERIES60_VERSION>26
#include <Etel3rdParty.h>
#include "asynccallhandler.h"
#endif
#ifndef EKA2
#include <plpvariant.h> // IMEI
#include <saclient.h>   // Battery, network, see also sacls.h
#if SERIES60_VERSION>12
#include <settinginfo.h> // Ringing volume, SDK 2.0 onwards
#endif
#else /*EKA2*/
#include <centralrepository.h>
#include <ProfileEngineSDKCRKeys.h>
#endif /*EKA2*/
#include <f32file.h> 

const TInt KPhoneSwVersionLineFeed = '\n';



#ifdef __WINS__
_LIT(KEmulatorIMEI, "000000000000000");
_LIT(KEmulator, "emulator");
#endif

#ifndef EKA2
// UID for network signal strength
const TInt KUidNetworkBarsValue = 0x100052D4;
const TUid KUidNetworkBars = {KUidNetworkBarsValue};

// UID for battery level
const TInt KUidBatteryBarsValue = 0x100052D3;
const TUid KUidBatteryBars = {KUidBatteryBarsValue};
#endif /*EKA2*/

/*
 * Returns the operating system version.
 */
extern "C" PyObject *
sysinfo_osversion(PyObject* /*self*/)
{
  TVersion version;
  version = User::Version();
  return Py_BuildValue("(iii)", version.iMajor,
            version.iMinor, version.iBuild);
}


extern "C"
void sysinfo_mod_cleanup();

#if SERIES60_VERSION>26
class TStaticData {
public:
  CTelephony *telephony;
};

TStaticData* GetTelephony()
{
  if (Dll::Tls())
  {
      return static_cast<TStaticData*>(Dll::Tls());
  }
  else 
  {
      TInt error = KErrNone;
      TStaticData* sd = NULL;
      TRAP(error, {
        sd = new (ELeave) TStaticData;
      });
      if(error!=KErrNone){
        return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }
      TRAP(error, sd->telephony = CTelephony::NewL());
      
      if (error != KErrNone){
        delete sd;
        return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }
      error = Dll::SetTls(sd);
      if(error!=KErrNone){
        delete sd->telephony;
        delete sd;
        return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }    
         
      PyThread_AtExit(sysinfo_mod_cleanup);
      return static_cast<TStaticData*>(Dll::Tls()); 
  }   
}
#else
class TStaticData {
public:
  RSystemAgent systemAgent;
};

TStaticData* GetSystemAgent()
{
  if (Dll::Tls())
  {
      return static_cast<TStaticData*>(Dll::Tls());
  }
  else 
  {
      TInt error = KErrNone;
      TStaticData* sd = NULL;
      TRAP(error, {
        sd = new (ELeave) TStaticData;
      });
      if(error!=KErrNone){
        return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }
      error = sd->systemAgent.Connect();
      if (error != KErrNone) {
          delete sd;
          return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }
      error = Dll::SetTls(sd);
      if(error!=KErrNone){
        delete sd;
        return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }
         
      PyThread_AtExit(sysinfo_mod_cleanup);
      return static_cast<TStaticData*>(Dll::Tls()); 
  }   
}
#endif


extern "C" {
  void sysinfo_mod_cleanup()
  {
#if SERIES60_VERSION>26  
    TStaticData* sd = GetTelephony();

    if(sd!=NULL){
      delete sd->telephony;
      delete sd;
      Dll::SetTls(NULL);
    }
#else
    TStaticData* sd = GetSystemAgent();
    if(sd!=NULL){
      sd->systemAgent.Close();
      delete sd;
      Dll::SetTls(NULL);      
    }
#endif
  }
}

/*
 * Returns the software version of the device.
 */
extern "C" PyObject *
sysinfo_swversion(PyObject* /*self*/)
{
#ifdef __WINS__
  return Py_BuildValue("u#", ((TDesC&)KEmulator).Ptr(), ((TDesC&)KEmulator).Length());
#else
	TBufC<KSysUtilVersionTextLength> version;
  TPtr ptr(version.Des());
  TInt error = KErrNone;

	error = SysUtil::GetSWVersion(ptr);
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }  

	TInt index = 0;

	for (; index < ptr.Length(); index++)
		{
		if (ptr[index] == KPhoneSwVersionLineFeed)
			{
			ptr[index] = ' ';
			}
		}

  return Py_BuildValue("u#", version.Ptr(), version.Length());
#endif
}

/*
 * Returns the imei code.
 */
extern "C" PyObject *
sysinfo_imei(PyObject* /*self*/)
{
#ifdef __WINS__
  // Return a fake IMEI when on emulator
  return Py_BuildValue("u#", ((TDesC&)KEmulatorIMEI).Ptr(), ((TDesC&)KEmulatorIMEI).Length());
#else
#ifndef EKA2
  // This only works on target machine
  TPlpVariantMachineId imei;
  TRAPD(error,(PlpVariant::GetMachineIdL(imei)));
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }

  return Py_BuildValue("u#", imei.Ptr(), imei.Length());
#else 
  TInt error = KErrNone;
  TRequestStatus status;
  TStaticData* sd = NULL;
  sd = GetTelephony();
  if(sd==NULL){
    return NULL;
  }
  CTelephony::TPhoneIdV1 phoneId;    
  CTelephony::TPhoneIdV1Pckg phoneIdPkg(phoneId);

  Py_BEGIN_ALLOW_THREADS
  CAsyncCallHandler* asyncCallHandler = NULL;

  TRAP(error,{
    asyncCallHandler = CAsyncCallHandler::NewL(*sd->telephony);
    asyncCallHandler->GetPhoneId(phoneIdPkg,status);
  });
  
  delete asyncCallHandler;

  Py_END_ALLOW_THREADS

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return Py_BuildValue("u#",phoneId.iSerialNumber.Ptr(),phoneId.iSerialNumber.Length());
#endif /*EKA2*/
#endif /*__WINS__*/
}

/*
 * Returns the battery level left in the device.
 */
extern "C" PyObject *
sysinfo_battery(PyObject* /*self*/)
{
#ifdef __WINS__
  return Py_BuildValue("i", 0);
#else
#if SERIES60_VERSION<=26

  TStaticData* sd = NULL;
  sd = GetSystemAgent();
  if(sd==NULL){
    return NULL;
  }

	// Get network value:
	TInt batteryValue = sd->systemAgent.GetState(KUidBatteryBars); 
  return Py_BuildValue("i", batteryValue);
  
#else
  TInt error = KErrNone;
  TRequestStatus status;
  
  TStaticData* sd = NULL;
  sd = GetTelephony();
  if(sd==NULL){
    return NULL;
  }

  CTelephony::TBatteryInfoV1 batteryInfo;
  CTelephony::TBatteryInfoV1Pckg batteryInfoPkg(batteryInfo);
  
  Py_BEGIN_ALLOW_THREADS
  CAsyncCallHandler* asyncCallHandler = NULL;

  TRAP(error,{
    asyncCallHandler = CAsyncCallHandler::NewL(*sd->telephony);
    asyncCallHandler->GetBatteryInfo(batteryInfoPkg,status);
  });
  
  delete asyncCallHandler;

  Py_END_ALLOW_THREADS

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  } 
  return Py_BuildValue("i", batteryInfo.iChargeLevel);
    
#endif /*SERIES60_VERSION<=26*/
#endif /*__WINS__*/
}


/*
 * Returns the signal strength in in scale 0-7 currently.
 */
extern "C" PyObject *
sysinfo_signal_bars(PyObject* /*self*/)
{
#ifdef __WINS__
  return Py_BuildValue("i", 0);
#else
#if SERIES60_VERSION<=26

  TStaticData* sd = NULL;
  sd = GetSystemAgent();

  if(sd==NULL){
    return NULL;
  }

  // Get network value:
  TInt networkValue = sd->systemAgent.GetState(KUidNetworkBars);
  return Py_BuildValue("i", networkValue);
#else

  TInt error = KErrNone;
  TRequestStatus status = KRequestPending;;
  
  TStaticData* sd = NULL;
  sd = GetTelephony();
  if(sd==NULL){
    return NULL;
  }

  CTelephony::TSignalStrengthV1 signalStrength;    
  CTelephony::TSignalStrengthV1Pckg signalStrengthPkg(signalStrength);

  Py_BEGIN_ALLOW_THREADS
  CAsyncCallHandler* asyncCallHandler = NULL;

  TRAP(error,{
      asyncCallHandler = CAsyncCallHandler::NewL(*sd->telephony);
      asyncCallHandler->GetSignalStrength(signalStrengthPkg,status);
  });
  
  delete asyncCallHandler;

  Py_END_ALLOW_THREADS

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if (status == KErrNone) {
    return Py_BuildValue("i", signalStrength.iBar);
  }
  else {
    return NULL;
  }

#endif /*SERIES60_VERSION<=26*/
#endif /*__WINS__*/
}
/*
 * Returns the signal strength currently in dBm.
 */
#if SERIES60_VERSION>=28
extern "C" PyObject *
sysinfo_signal_dbm(PyObject* /*self*/)
{
#ifdef __WINS__
  return Py_BuildValue("i", 0);
#else
 
  TInt error = KErrNone;
  TRequestStatus status;
  TStaticData* sd = NULL;
  sd = GetTelephony();
  if(sd==NULL){
    return NULL;
  }

  CTelephony::TSignalStrengthV1 signalStrength;    
  CTelephony::TSignalStrengthV1Pckg signalStrengthPkg(signalStrength);

  Py_BEGIN_ALLOW_THREADS
  CAsyncCallHandler* asyncCallHandler = NULL;

  TRAP(error,{
    asyncCallHandler = CAsyncCallHandler::NewL(*sd->telephony);
    asyncCallHandler->GetSignalStrength(signalStrengthPkg,status);
  });
  
  delete asyncCallHandler;

  Py_END_ALLOW_THREADS

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }    
  if (status == KErrNone) {
    return Py_BuildValue("i", signalStrength.iSignalStrength);
  }
  else {
    return NULL;
  }
    
#endif /*__WINS__*/
}
#endif /* SERIES60_VERSION>=28 */
/*
 * Returns the total amount of RAM in the device.
 */
extern "C" PyObject *
sysinfo_memoryram(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt eValue;
  
  if ((error = HAL::Get(HALData::EMemoryRAM, eValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", eValue);
}

/*
 * Returns the total amount of ROM in the device.
 */
extern "C" PyObject *
sysinfo_memoryrom(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt eValue;
  
  if ((error = HAL::Get(HALData::EMemoryROM, eValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", eValue);
}

/*
 * Returns the size of RAM drive in the device (normally D:-drive).
 */
extern "C" PyObject *
sysinfo_maxramdrivesize(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt eValue;
  
  if ((error = HAL::Get(HALData::EMaxRAMDriveSize, eValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", eValue);
}

/*
 * Returns the twips of the device's display.
 */
extern "C" PyObject *
sysinfo_displaytwips(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt xValue;
  TInt yValue;
  
  if ((error = HAL::Get(HALData::EDisplayXTwips, xValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  if ((error = HAL::Get(HALData::EDisplayYTwips, yValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("(ii)", xValue, yValue);
}

/*
 * Returns the pixels in the device's display.
 */
extern "C" PyObject *
sysinfo_displaypixels(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt xValue;
  TInt yValue;
  
  if ((error = HAL::Get(HALData::EDisplayXPixels, xValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  if ((error = HAL::Get(HALData::EDisplayYPixels, yValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("(ii)", xValue, yValue);
}

/*
 * Returns the free ram available.
 */
extern "C" PyObject *
sysinfo_memoryramfree(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TInt eValue;
  
  if ((error = HAL::Get(HALData::EMemoryRAMFree, eValue)) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  return Py_BuildValue("i", eValue);
}

/*
 * Returns the ringing type set.
 */
#if SERIES60_VERSION>12
extern "C" PyObject *
sysinfo_ringtype(PyObject* /*self*/)
{
#ifndef EKA2
  TInt error = KErrNone;
  TInt ret;
  
  CSettingInfo* settings = NULL;

  // NULL given since notifications are not needed
  TRAP(error,(settings = CSettingInfo::NewL(NULL)));
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }

  if((error = settings->Get(SettingInfo::ERingingType, ret)) != KErrNone) {
    delete settings;
    settings = NULL;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  delete settings;
  settings = NULL;

  return Py_BuildValue("i", ret);
#else 
  TInt error = KErrNone;
  TInt ret;
  TRAP(error,{
    CRepository* rep = CRepository::NewLC(KCRUidProfileEngine);
    User::LeaveIfError(rep->Get(KProEngActiveRingingType, ret));
    CleanupStack::PopAndDestroy(rep);
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return Py_BuildValue("i", ret);
  
#endif /*EKA2*/
}
#endif


/*
 * Returns the currently active profile.
 */
#if SERIES60_VERSION>12
extern "C" PyObject *
sysinfo_activeprofile(PyObject* /*self*/)
{
#ifndef EKA2
  TInt error = KErrNone;
  TInt ret;
  
  CSettingInfo* settings = NULL;

  // NULL given since notifications are not needed
  TRAP(error,(settings = CSettingInfo::NewL(NULL)));
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }

  if((error = settings->Get(SettingInfo::EActiveProfile, ret)) != KErrNone) {
    delete settings;
    settings = NULL;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  delete settings;
  settings = NULL;

  return Py_BuildValue("i", ret);
#else 
  TInt error = KErrNone;
  TInt ret;
  TRAP(error,{
    CRepository* rep = CRepository::NewLC(KCRUidProfileEngine);
    User::LeaveIfError(rep->Get(KProEngActiveProfile, ret));
    CleanupStack::PopAndDestroy(rep);
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return Py_BuildValue("i", ret);
  
#endif /*EKA2*/
}
#endif

/*
 * Returns the drives and the amount of free space in the device.
 */
extern "C" PyObject *
sysinfo_drive_free(PyObject* /*self*/)
{
  TInt error = KErrNone;
  RFs fsSession;
  if ((error = fsSession.Connect()) != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  PyObject* ret = PyDict_New();
  if (ret == NULL)
    return PyErr_NoMemory();

  TVolumeInfo volumeInfo; 
  TInt driveNumber;

  for (driveNumber = EDriveA; driveNumber <= EDriveZ; driveNumber++) {
    TInt err = fsSession.Volume(volumeInfo,driveNumber); 
    if (err != KErrNone)
      continue; 

    char d[2];
    d[0] = 'A'+driveNumber; d[1] = ':';

    TInt freeSpace;
#ifndef EKA2
    freeSpace = (volumeInfo.iFree).GetTInt();
#else
    freeSpace = (volumeInfo.iFree);
#endif /*EKA2*/

    PyObject* f = Py_BuildValue("i", freeSpace);
    PyObject* v = PyUnicode_Decode(d, 2, NULL, NULL);
    if (v == NULL || f == NULL) {
      Py_XDECREF(v);
      Py_XDECREF(f);
      Py_DECREF(ret);
      ret = NULL;
      break;     
    }

    if ((PyDict_SetItem(ret, (v), f) != 0)) {
      Py_DECREF(v);
      Py_DECREF(f);
      Py_DECREF(ret);
      ret = NULL;
      break; 
    }

    Py_DECREF(v);
    Py_DECREF(f);
  }

  fsSession.Close();

  return ret;
}



extern "C" {

  static const PyMethodDef sysinfo_methods[] = {
    {"os_version", (PyCFunction)sysinfo_osversion, METH_NOARGS, NULL},
    {"sw_version", (PyCFunction)sysinfo_swversion, METH_NOARGS, NULL},
    {"imei", (PyCFunction)sysinfo_imei, METH_NOARGS, NULL},
    {"battery", (PyCFunction)sysinfo_battery, METH_NOARGS, NULL},
    {"signal_bars", (PyCFunction)sysinfo_signal_bars, METH_NOARGS, NULL},
#if SERIES60_VERSION>=28
    {"signal_dbm", (PyCFunction)sysinfo_signal_dbm, METH_NOARGS, NULL},
#endif
    {"total_ram", (PyCFunction)sysinfo_memoryram, METH_NOARGS, NULL},
    {"total_rom", (PyCFunction)sysinfo_memoryrom, METH_NOARGS, NULL},
    {"max_ramdrive_size", (PyCFunction)sysinfo_maxramdrivesize, METH_NOARGS, NULL},
    {"display_twips", (PyCFunction)sysinfo_displaytwips, METH_NOARGS, NULL},
    {"display_pixels", (PyCFunction)sysinfo_displaypixels, METH_NOARGS, NULL},
    {"free_ram", (PyCFunction)sysinfo_memoryramfree, METH_NOARGS, NULL},
#if SERIES60_VERSION>12
    {"ring_type", (PyCFunction)sysinfo_ringtype, METH_NOARGS, NULL},
    {"active_profile", (PyCFunction)sysinfo_activeprofile, METH_NOARGS, NULL},
#endif
    {"free_drivespace", (PyCFunction)sysinfo_drive_free, METH_NOARGS, NULL},

	
    {NULL,NULL}           /* sentinel */
  };

  DL_EXPORT(void) initsysinfo(void)
  {
    
    
    Py_InitModule("_sysinfo", (PyMethodDef*)sysinfo_methods);
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif /*EKA2*/
