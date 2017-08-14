/*
* ====================================================================
*  apselection.cpp
*  
* Copyright (c) 2006 - 2008 Nokia Corporation
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



// ***************************************************************
// NOTE: this file has been added because there is a conflict between
// types defined in <apsettingshandlerui.h> and <OBEX.h>.
// ***************************************************************



// Accesspoint selection
#include <e32base.h>
#include <apsettingshandlerui.h>
#include "Python.h"
#include "symbian_python_ext_util.h"
#include "apselection.h"
#include <aputils.h>


extern "C" PyObject *
ap_select_ap()
{
    TInt error=KErrNone;
    TInt ret = 0;
    CCommsDatabase* db = NULL;
    CApSettingsHandler* settingsHandler = NULL;
    CApUtils *aputil = NULL;
    
    TRAP(error, {
      db=CCommsDatabase::NewL(EDatabaseTypeIAP);
      settingsHandler = CApSettingsHandler::NewLC(
          ETrue, 
          EApSettingsSelListIsPopUp, 
          EApSettingsSelMenuSelectOnly,
          KEApIspTypeAll, 
          EApBearerTypeAll, 
          KEApSortNameAscending
          );
      aputil = CApUtils::NewLC(*db);
      CleanupStack::Pop(2);    
    });
    if (error != KErrNone) {
      delete aputil;
      delete settingsHandler;
      delete db;
      return SPyErr_SetFromSymbianOSErr(error);
    }
    TUint32 originallyFocused(0);
    TUint32 aSelectedIap(0);

    // Show the dialog
    TRAP(error, ret = settingsHandler->RunSettingsL(originallyFocused, aSelectedIap));
    delete settingsHandler;
    if (error != KErrNone) {
      delete aputil;
      delete db;
      return SPyErr_SetFromSymbianOSErr(error);
    }
     
    if(ret & KApUiEventSelected){
      // user has made a selection.
      
      TInt iapid = aputil->IapIdFromWapIdL(aSelectedIap);
      delete aputil;
      delete db; 
      return Py_BuildValue("i", iapid);      
    }
    delete aputil;
    delete db; 
    Py_INCREF(Py_None);
    return Py_None;
}
