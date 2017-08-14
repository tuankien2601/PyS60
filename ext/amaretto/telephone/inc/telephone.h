/**
 * ====================================================================
 * telephone.h
 *
 * Copyright (c) 2005 - 2007 Nokia Corporation
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
#include <e32std.h>

#ifndef EKA2 
#include <etel.h>
_LIT(KTsyName,"phonetsy.tsy");
#else
#include <Etel3rdParty.h>
#endif /* EKA2 */

#define MaxTelephoneNumberLength 30

#ifdef EKA2
class TPyPhoneCallBack
  {
  public:
    TInt IncomingCall(TInt aStatus, TDesC& aNumber);
  public:
    PyObject* iCb; // Not owned.
  };
#endif

//Safe to use also from emulator.
#ifndef EKA2
class CPhoneCall : public CActive
#else
NONSHARABLE_CLASS (CPhoneCall) : public CActive
#endif
  {
public:
  static CPhoneCall* NewL();
  static CPhoneCall* NewLC();
  ~CPhoneCall();
public: 
  CPhoneCall();
  void ConstructL();
  
  TInt Initialise();
  TInt UnInitialise();
  TInt Dial();
  void SetNumber(TDes& aNumber);
  TInt HangUp();
#ifdef EKA2
  TInt Answer();
  void SetIncomingCallBack(TPyPhoneCallBack &aCb);
#endif
  /*
  ENotCalling The initial (un-initialised) state
  EInitialised After Initialise() call, ready to Dial()
  ECalling A dial request has been issued
  ECallInProgress A call is ongoing
  */
	enum TCallState 
	  {
 #ifdef EKA2
    EHangingUp,
 #endif /* EKA2 */
		ENotCalling,
		EInitialised,
    ECalling,
    ECallInProgress
	  };  
	//These are returned to avoid ETel server panics:
	enum TPhoneCallErrors
	  {
		ENumberNotSet = 0x7D0,
		ENotInitialised,
		ENotCallInProgress,
		EInitialiseCalledAlready,
		EAlreadyCalling,
	  };  
public: //from CActive:
  void RunL();
  void DoCancel();
private:
#ifndef EKA2
  RTelServer iServer;
  RTelServer::TPhoneInfo iInfo;
  RPhone iPhone;
  RPhone::TLineInfo iLineInfo;
  RLine iLine;
  TBuf<100> iNewCallName;
  RCall iCall;
#else
  CTelephony* iTelephony;
  CTelephony::TCallId iCallId;
  CTelephony::TCallStatusV1Pckg iCallStatusV1Pckg;
  CTelephony::TCallStatusV1 iCallStatusV1;

  TPyPhoneCallBack iCallMe;
  TBool iCallBackSet;  
  TBool iWaitingForCall;
  TBool iAnswering;
#endif /* EKA2 */
  TCallState iCallState;
  TBuf<MaxTelephoneNumberLength> iNumber;
  TBool iNumberSet;
  };

//////////////TYPE DEFINITION

#define PHO_type ((PyTypeObject*)SPyGetGlobalString("PHOType"))

struct PHO_object {
  PyObject_VAR_HEAD
  CPhoneCall* phone;
#ifdef EKA2
  TPyPhoneCallBack myCallBack;
  TBool callBackSet;
#endif
};
