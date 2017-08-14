/**
 * ====================================================================
 *  location.cpp
 *
 *  Python API to location information.
 *
 * Copyright (c) 2005 - 2008 Nokia Corporation
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
 * ====================================================================
 */
 
#include "Python.h"
#include "symbian_python_ext_util.h"
#include <pythread.h>

#include <eikenv.h>
#include <e32std.h>

#ifndef EKA2
#include <etelbgsm.h>
#else
#include <etel.h>
#include <etel3rdparty.h>
#include <e32cmn.h>
#include "asynccallhandler.h"
#endif



#ifndef EKA2
_LIT (KTsyName, "phonetsy.tsy");

extern "C" PyObject *
get_location(PyObject* /*self*/)
{
  TInt error = KErrNone;

  TInt enumphone = 1;
  RTelServer	 server;
  RBasicGsmPhone phone;
  RTelServer::TPhoneInfo info;
  MBasicGsmPhoneNetwork::TCurrentNetworkInfo NetworkInfo;

  error = server.Connect();
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = server.LoadPhoneModule(KTsyName);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = server.EnumeratePhones(enumphone);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  if (enumphone < 1)
    return SPyErr_SetFromSymbianOSErr(KErrNotFound);

  error = server.GetPhoneInfo(0, info);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = phone.Open(server, info.iName);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = phone.GetCurrentNetworkInfo(NetworkInfo);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
		       
  phone.Close();
  server.Close();

  return Py_BuildValue("(iiii)", NetworkInfo.iNetworkInfo.iId.iMCC,
		       NetworkInfo.iNetworkInfo.iId.iMNC,
		       NetworkInfo.iLocationAreaCode,
		       NetworkInfo.iCellId);
}


#else

extern "C"
void location_mod_cleanup();

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
         
      PyThread_AtExit(location_mod_cleanup);
      return static_cast<TStaticData*>(Dll::Tls()); 
  }   
}

extern "C" {
  void location_mod_cleanup()
  {
    TStaticData* sd = NULL;
    sd = GetTelephony();
    
    if(sd!=NULL){
      delete sd->telephony;
      delete static_cast<TStaticData *>(Dll::Tls());
      Dll::SetTls(NULL);
    }
  }
}

extern "C" PyObject *
get_location(PyObject* /*self*/)
{
  TInt error = KErrNone;
  
  TStaticData* sd = NULL;
  sd = GetTelephony();
  if(sd==NULL){
    return NULL;
  }
  
  TRequestStatus status;
  CTelephony::TNetworkInfoV1 networkInfo;
  
  CTelephony::TNetworkInfoV1Pckg networkInfoPkg(networkInfo);
  
  Py_BEGIN_ALLOW_THREADS
  CAsyncCallHandler* asyncCallHandler = NULL;

  TRAP(error,{
    asyncCallHandler = CAsyncCallHandler::NewL(*sd->telephony);
    asyncCallHandler->MakeAsyncCall(networkInfoPkg,status);
  });
  
  delete asyncCallHandler;
  Py_END_ALLOW_THREADS
    
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  return Py_BuildValue("(u#u#iii)", 
         networkInfo.iCountryCode.Ptr(),networkInfo.iCountryCode.Length(), // string
		     networkInfo.iNetworkId.Ptr(),networkInfo.iNetworkId.Length(), // string    
		     networkInfo.iLocationAreaCode,
		     networkInfo.iCellId,
		     networkInfo.iAreaKnown);
}
#endif /* EKA2 */



extern "C" {

  static const PyMethodDef location_methods[] = {
    {"gsm_location", (PyCFunction)get_location, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initlocation(void)
  {
    Py_InitModule("_location", (PyMethodDef*)location_methods);
    return;
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif
