/**
 * ====================================================================
 * capturer.cpp
 * Copyright (c) 2006-2008 Nokia Corporation
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
#include "capturer.h"



CCapturer* CCapturer::NewL(PyObject* callback)
{
  CCapturer* self = new (ELeave) CCapturer(callback);
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop(self);
  return self;
};


void CCapturer::ConstructL()
{
  iSession = new (ELeave) RWsSession();
  
  User::LeaveIfError(iSession->Connect());
  iWinGroup = new (ELeave) RWindowGroup(*iSession);
  iWinGroup->Construct((TUint32)iWinGroup,EFalse);
  
  iWinGroup->SetOrdinalPosition(-1,ECoeWinPriorityNeverAtFront); 
  iWinGroup->EnableReceiptOfFocus(EFalse);
  
  iWinGroupName = CApaWindowGroupName::NewL(*iSession);
  iWinGroupName->SetHidden(ETrue); // Hide from tasklist.
  iWinGroupName->SetWindowGroupName(*iWinGroup);
  
  CActiveScheduler::Add(this);
};


CCapturer::CCapturer(PyObject* aCallback):
CActive(CActive::EPriorityStandard),iSession(NULL),iWinGroup(NULL),iWinGroupName(NULL),
iCallback(aCallback),iLastCapturedKey(0),iCallbackRunning(EFalse),iForwarding(EFalse),
iRunning(EFalse)
{
};


TInt32 CCapturer::SetKeyToBeCapturedL(TInt32 keyCode)
{
  TInt32 keyId;
  User::LeaveIfError(keyId = iWinGroup->CaptureKey(keyCode,0,0));
  return keyId;
};


void CCapturer::RemoveKey(TInt32 keyId)
{
  iWinGroup->CancelCaptureKey(keyId);
};


void CCapturer::StartCapturing()
{
  if(iRunning == EFalse){
    iSession->EventReady(&iStatus);
    SetActive();
    iRunning = ETrue;
  }
};


void CCapturer::DoCancel()
{
  if((iCallbackRunning == EFalse) && (iRunning != EFalse)){
    iSession->EventReadyCancel();
  }
  iRunning = EFalse;
};


CCapturer::~CCapturer()
{
  Cancel();
  if(iWinGroup){
    iWinGroup->Close();
    delete iWinGroup;
  }
  delete iWinGroupName;
  if(iSession){
    iSession->Close();
    delete iSession;
  }
};


void CCapturer::RunL()
{  
  if(iStatus==KErrNone){
    TWsEvent event;
    iSession->GetEvent(event);
    iLastCapturedKey = event.Key()->iCode;
        
    if(iCallback){
      iCallbackRunning = ETrue;
      PyGILState_STATE gstate = PyGILState_Ensure();
      PyObject* parameters = Py_BuildValue("(i)",iLastCapturedKey);
      PyObject* tmp = PyEval_CallObject(iCallback, parameters);
      Py_XDECREF(tmp);
      Py_XDECREF(parameters);
      if (PyErr_Occurred()){
        PyErr_Print();
      }
      PyGILState_Release(gstate);
      iCallbackRunning = EFalse;
    }
  
    iSession->EventReady(&iStatus);  
    
    if(iForwarding != EFalse){
      iSession->SendEventToWindowGroup(iSession->GetFocusWindowGroup(),event);
    }
        
    if(iRunning != EFalse){
      SetActive();     
    }else{
      Cancel();
    }
  }
};


TInt32 CCapturer::LastCapturedKey()
{
  return iLastCapturedKey;
};


void CCapturer::SetForwarding(TInt forwarding)
{
  if(forwarding == 0){
    iForwarding = EFalse;
  }else{
    iForwarding = ETrue;
  }
};

