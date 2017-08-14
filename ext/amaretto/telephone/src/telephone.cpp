/**
 * ====================================================================
 * telephone.cpp
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

#include "telephone.h"

#ifdef EKA2
//A helper function for the implementation of callbacks
//from C/C++ code to Python callables (modified from appuifwmodule.cpp)
TInt TPyPhoneCallBack::IncomingCall(TInt aStatus, TDesC& aNumber)
  {
  PyGILState_STATE gstate = PyGILState_Ensure();
 
  TInt error = KErrNone;
  
  PyObject* arg = Py_BuildValue("((iu#))", aStatus, aNumber.Ptr(), aNumber.Length());
  PyObject* rval = PyEval_CallObject(iCb, arg);
  
  Py_DECREF(arg);
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      if (PyInt_Check(value))
        error = PyInt_AS_LONG(value);
    }
  }
  else
    Py_DECREF(rval);

  PyGILState_Release(gstate);
  return error;
  }
#endif

//////////////

CPhoneCall* CPhoneCall::NewL()
  {
  CPhoneCall* self = NewLC();
  CleanupStack::Pop(self); 
  return self;
  }

CPhoneCall* CPhoneCall::NewLC()
  {
  CPhoneCall* self = new (ELeave) CPhoneCall();
  CleanupStack::PushL(self);
  self->ConstructL();
  return self;
  }

void CPhoneCall::SetNumber(TDes& aNumber) 
  {
  iNumber = aNumber;
  iNumberSet = ETrue;
  }

#ifndef EKA2
CPhoneCall::CPhoneCall()
  : CActive(CActive::EPriorityStandard)
  {
  CActiveScheduler::Add(this);
  }

void CPhoneCall::ConstructL()
  {
  iCallState = ENotCalling;
  iNumberSet = EFalse;
  }

CPhoneCall::~CPhoneCall()
  {
  Cancel();

  //This is the state where we should be. If not, then we need to close the
  //servers
  if (iCallState != ENotCalling) 
    {
#ifndef __WINS__
	  //Close the phone, line and call connections
	  //NOTE: This does not hang up the call
    iPhone.Close();
    iLine.Close();
    iCall.Close();

    //XXX error code
    //Unload the phone device driver
    /*error =*/ iServer.UnloadPhoneModule(KTsyName);
    /*if (error != KErrNone)
      return error;*/

	  //Close the connection to the tel server
    iServer.Close();
#endif /* __WINS__ */
    }
  }

void CPhoneCall::RunL()
  {
  switch (iCallState) 
    {
    case ECalling:
      iCallState = ECallInProgress;
      break;
    default:
      break;
    }
  }
   
void CPhoneCall::DoCancel()
  {
  iCall.DialCancel();
  iCallState = EInitialised;
  }
  
TInt CPhoneCall::Initialise()
  {
  TInt error = KErrNone;
  if (iCallState != ENotCalling)
    return EInitialiseCalledAlready;
#ifndef __WINS__

	//Create a connection to the tel server
  error = iServer.Connect();
  if (error != KErrNone)
    return error;

	//Load in the phone device driver
  error = iServer.LoadPhoneModule(KTsyName);
  if (error != KErrNone)
    return error;
  
	//Find the number of phones available from the tel server
	TInt numberPhones;
  error = iServer.EnumeratePhones(numberPhones);
  if (error != KErrNone)
    return error;

	//Check there are available phones
	if (numberPhones < 1)
		{
		return KErrNotFound;
    }

	//Get info about the first available phone
  error = iServer.GetPhoneInfo(0, iInfo);
  if (error != KErrNone)
    return error;

	//Use this info to open a connection to the phone, the phone is identified by its name
  error = iPhone.Open(iServer, iInfo.iName);
  if (error != KErrNone)
    return error;
    
  //"The phone hardware is usually automatically initialised before 
  //the first command is sent" (from SDK), no need for Initialise()

	//Get info about the first line from the phone
  error = iPhone.GetLineInfo(0, iLineInfo);
  if (error != KErrNone)
    return error;

	//Use this to open a line
  error = iLine.Open(iPhone, iLineInfo.iName);
  if (error != KErrNone)
    return error;

	//Open a new call on this line
  error = iCall.OpenNewCall(iLine, iNewCallName);
  if (error != KErrNone)
    return error;
#endif /* __WINS__ */
  iCallState = EInitialised;
    
  return error;
  }
  
TInt CPhoneCall::UnInitialise()
  {
  TInt error = KErrNone;
  
  if (IsActive())
    return EAlreadyCalling;
    
  if (iCallState == ENotCalling)
    return ENotInitialised;

#ifndef __WINS__
	//Close the phone, line and call connections
	//NOTE: This does not hang up the call
  iPhone.Close();
  iLine.Close();
  iCall.Close();

  //Unload the phone device driver
  error = iServer.UnloadPhoneModule(KTsyName);
  if (error != KErrNone)
    return error;

	//Close the connection to the tel server
  iServer.Close();
#endif /* __WINS__ */
  iCallState = ENotCalling;
  
  return error;
  }

TInt CPhoneCall::Dial()
  {
  TInt error = KErrNone;
  
  if (!iNumberSet)
    return ENumberNotSet;
    
  if (iCallState == ENotCalling)
    return ENotInitialised;

  if (IsActive())
    return EAlreadyCalling;
    
#ifndef __WINS__
  iCall.Dial(iStatus, iNumber);  
  SetActive();
#endif /* __WINS__ */  
  iCallState = ECalling;
  return error;
  }
  
TInt CPhoneCall::HangUp()
  {
  TInt error = KErrNone;
 
  if (iCallState == ECallInProgress || iCallState == ECalling) 
    {
#ifndef __WINS__
    error = iCall.HangUp(); //synchronous, should be changed to asynchronous
#endif /* __WINS__ */  
    iCallState = EInitialised;
    return error;
    }
    
  return ENotCallInProgress;
  }

#else /* EKA2 */
CPhoneCall::CPhoneCall()
  : CActive(CActive::EPriorityStandard), iCallStatusV1Pckg(iCallStatusV1)
  {
  CActiveScheduler::Add(this);
  }

void CPhoneCall::ConstructL()
  {
  iTelephony = CTelephony::NewL();
  iCallState = ENotCalling;
  iNumberSet = EFalse;
  iWaitingForCall = EFalse;
  iAnswering = EFalse;
  }

CPhoneCall::~CPhoneCall()
  {
  Cancel();
  delete iTelephony;
  }

void CPhoneCall::RunL()
  {
  // Check whether this was incoming call invocation
  if(iCallStatusV1.iStatus == CTelephony::EStatusRinging && iWaitingForCall)
    {
    // Fetch call number:
    TInt error = KErrNone;
    TBuf<CTelephony::KMaxTelNumberSize> number;
    
    CTelephony::TRemotePartyInfoV1 remotePartyInfoV1;
    CTelephony::TRemotePartyInfoV1Pckg remotePartyInfoV1Pckg(remotePartyInfoV1);
    
    CTelephony::TCallInfoV1 callInfoV1;
    CTelephony::TCallInfoV1Pckg callInfoV1Pckg(callInfoV1);
    
    CTelephony::TCallSelectionV1 callSelectionV1;
    CTelephony::TCallSelectionV1Pckg callSelectionV1Pckg(callSelectionV1);
    
    callSelectionV1.iLine = CTelephony::EVoiceLine;
    callSelectionV1.iSelect = CTelephony::EInProgressCall;
    
    error = iTelephony->GetCallInfo(callSelectionV1Pckg, callInfoV1Pckg, remotePartyInfoV1Pckg);
    
    // if(error==KErrNone) // XXX add
    
    if(remotePartyInfoV1.iRemoteIdStatus == CTelephony::ERemoteIdentityAvailable)
      number.Copy(remotePartyInfoV1.iRemoteNumber.iTelNumber);

    iCallMe.IncomingCall(static_cast<TInt>(iCallStatusV1.iStatus), number);
    
    iCallState = ECallInProgress;
    iWaitingForCall = EFalse;
    iAnswering = EFalse;
    
    return;
    }

  // Check whether this was answering call invocation
  if(iCallStatusV1.iStatus == CTelephony::EStatusRinging && iAnswering)
    {
    iCallState = ECallInProgress;
    return;
    }
   
  switch (iCallState) 
    {
    case ECalling:
      if(iStatus == KErrNone)
        iCallState = ECallInProgress;
      break;
    case EHangingUp:
      //if(iStatus == KErrNone)
        iCallState = EInitialised;
      break;
    default:
      break;
    }

  // All the rest:
  if(iCallBackSet)
    {
    TBuf<CTelephony::KMaxTelNumberSize> number;
    iCallMe.IncomingCall(static_cast<TInt>(iCallStatusV1.iStatus), number);
    }

  }
   
void CPhoneCall::DoCancel()
  {
  // XXX CancelAsync return code handling
  if(iCallState == ECalling)
    iTelephony->CancelAsync(CTelephony::EDialNewCallCancel);
  else if(iCallState == EHangingUp)
    iTelephony->CancelAsync(CTelephony::EHangupCancel);
  iCallState = EInitialised;
  
  if(iWaitingForCall) 
    {
    iTelephony->CancelAsync(CTelephony::EVoiceLineStatusChangeCancel);
    iWaitingForCall = EFalse;
    }

  if(iAnswering) 
    {
    iTelephony->CancelAsync(CTelephony::EAnswerIncomingCallCancel);
    iAnswering = EFalse;
    }
  }
  
TInt CPhoneCall::Initialise()
  {
  TInt error = KErrNone;

  if (iCallState != ENotCalling)
    return EInitialiseCalledAlready;

  iCallState = EInitialised;
    
  return error;
  }
  
TInt CPhoneCall::UnInitialise()
  {
  TInt error = KErrNone;
  
  if (iCallState == ECalling)
    return EAlreadyCalling;
    
  else if (iCallState == ENotCalling)
    return ENotInitialised;

  iCallState = ENotCalling;
  
  return error;
  }

TInt CPhoneCall::Dial()
  {
  TInt error = KErrNone;
  
  if (!iNumberSet)
    return ENumberNotSet;
    
  if (iCallState == ENotCalling)
    return ENotInitialised;

  else if (iCallState == ECalling)
    return EAlreadyCalling;
    
#ifndef __WINS__
  CTelephony::TTelNumber telNumber(iNumber);
  CTelephony::TCallParamsV1 callParams;
  callParams.iIdRestrict = CTelephony::ESendMyId;
  CTelephony::TCallParamsV1Pckg callParamsPckg(callParams);
  iTelephony->DialNewCall(iStatus, callParamsPckg, telNumber, iCallId);
  SetActive();
#endif /* __WINS__ */  
  iCallState = ECalling;
  return error;
  }
  
TInt CPhoneCall::HangUp()
  {
  TInt error = KErrNone;
  // Check the current status (if the other side has hang-up):
#ifndef __WINS__
  CTelephony::TCallStatusV1 callStatusV1;
  CTelephony::TCallStatusV1Pckg callStatusV1Pckg(callStatusV1);

  iTelephony->GetCallStatus(iCallId, callStatusV1Pckg);
  CTelephony::TCallStatus callStatus = callStatusV1.iStatus;
  if(callStatus == CTelephony::EStatusIdle) 
    return ENotCallInProgress;

  if (iCallState == ECallInProgress) // The call was answered, calling hang up is fine 
    {
    iTelephony->Hangup(iStatus, iCallId); //asynchronous
    SetActive();
    iCallState = EHangingUp;
    return error;
    }
  else if (iCallState == ECalling) // Waiting to be answered, calling hang up is not fine 
    {
    iTelephony->CancelAsync(CTelephony::EDialNewCallCancel);
    iCallState = EInitialised;
    return error;
    }
#else
  if (iCallState == ECallInProgress || iCallState == ECalling) 
    {
    iCallState = EInitialised;
    return error;
    }
#endif
  return ENotCallInProgress;
  }

TInt CPhoneCall::Answer()
  {
  TInt error = KErrNone;
  // Check the current status (if the other side has hang-up):
#ifndef __WINS__
    // Check whether this was incoming call invocation and exit if true
  if(iCallStatusV1.iStatus == CTelephony::EStatusRinging)
    {
    iTelephony->AnswerIncomingCall(iStatus, iCallId);
    SetActive();
    iAnswering = ETrue;
    return error;
    }
#else
  return error; // XXX
#endif
  return error; // XXX
  }

void CPhoneCall::SetIncomingCallBack(TPyPhoneCallBack& aCb) 
  {
#ifndef __WINS__
  // XXX Add here if !iWaitingForCall and test it.
  //     This way "waiting" will not start if we are already waiting
  iCallMe = aCb;
  iCallBackSet = ETrue;
  iWaitingForCall = ETrue;

  iTelephony->NotifyChange(iStatus, 
                           CTelephony::EVoiceLineStatusChange, 
                           iCallStatusV1Pckg);
  SetActive();
#endif
  }
#endif /* EKA2 */
