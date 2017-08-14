/*
 * ====================================================================
 *  locationacqmodule.h
 *
 *  Python API to Series 60 Location Acquisition API.
 *
 * Copyright (c) 2007 Nokia Corporation
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

#ifndef __LOCATIONACQMODULE
#define __LOCATIONACQMODULE

#include "Python.h"

#include "symbian_python_ext_util.h"

#include <Lbs.h> 
#include <LbsSatellite.h> 

//////////////CONSTANTS//////////////

#define COURSE_INFO   0x01
#define SATELLITE_INFO   0x02

// values of requestor dictionary
_LIT8(KServiceRequestor, "service");
_LIT8(KContactRequestor, "contact");
_LIT8(KApplicationFormat, "application");
_LIT8(KTelephoneFormat, "telephone");
_LIT8(KUrlFormat, "url");
_LIT8(KMailFormat, "mail");

//////////////CLASSES////////////////

class TPyPosCallBack
  {
  public:
    TInt PositionUpdated(TPositionInfo* aPositionInfo, 
                         TInt aFlags);
  public:
    PyObject* iCb; // Not owned.
  };

#ifndef EKA2
class CPosDataFetcher:public CActive
#else
NONSHARABLE_CLASS(CPosDataFetcher):public CActive
#endif
{
public:
	static CPosDataFetcher* NewL(RPositioner& aPositioner);
	~CPosDataFetcher();
public:
	void DoCancel();
	void RunL();
	TInt CreatePositionType(TInt);
	void FetchData(TRequestStatus& aStatus, 
	               TTimeIntervalMicroSeconds aInterval,
	               TBool aPartial);
  void SetCallBack(TPyPosCallBack &aCb);
private:
  RPositioner& iPositioner;
  TPositionInfo* iPositionInfo;
  TInt iFlags;
  TBool iCallBackSet;
  TBool iCancelled;
  TPyPosCallBack iCallMe;
  void ConstructL();
  CPosDataFetcher(RPositioner& aPositioner);
};

//////////////TYPE DEFINITION//////////////

/*
 * PositionServer (RPositionServer).
 */
#define PositionServer_type ((PyTypeObject*)SPyGetGlobalString("PositionServerType"))
struct PositionServer_object {
  PyObject_VAR_HEAD
  RPositionServer* posServ; 
};

/*
 * Positioner (RPositioner).
 */
#define Positioner_type ((PyTypeObject*)SPyGetGlobalString("PositionerType"))
struct Positioner_object {
  PyObject_VAR_HEAD
  PositionServer_object* positionServer;
  RPositioner* positioner;
  RRequestorStack* requestorStack;
  CPosDataFetcher* dataFetcher;
  TPyPosCallBack myCallBack;
  TBool callBackSet;
  TUid  moduleid;
  TBool isSubSessionOpen;
};

//////////////MACRO DEFINITION///////////////

//////////////METHODS///////////////

/*
 * Create new Positioner object.
 */
extern "C" PyObject *
new_Positioner_object(PositionServer_object* posServ, TPositionModuleId moduleId);

void deleteRequestorStack(Positioner_object *positioner);

TInt opensubsession(Positioner_object *self);
#endif
