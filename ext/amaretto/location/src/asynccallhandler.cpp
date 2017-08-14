/**
 * ====================================================================
 * asynccallhandler.cpp
 *
 * Copyright (c) 2006 Nokia Corporation
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

#include <e32base.h>
#include "AsyncCallHandler.h"

void CAsyncCallHandler::ConstructL()
{ 
  iWait=new (ELeave) CActiveSchedulerWait;
  CActiveScheduler::Add(this);
};

CAsyncCallHandler::CAsyncCallHandler(CTelephony& telephony):CActive(CActive::EPriorityStandard),iTelephony(telephony)
{
};

CAsyncCallHandler* CAsyncCallHandler::NewL(CTelephony& telephony)
{
  CAsyncCallHandler* self=new (ELeave) CAsyncCallHandler(telephony);
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop(self);
  return self;
};

void CAsyncCallHandler::DoCancel()
{
  this->iTelephony.CancelAsync(CTelephony::EGetCurrentNetworkInfoCancel);  
};

CAsyncCallHandler::~CAsyncCallHandler()
{
  Cancel();
  delete iWait;
};

void CAsyncCallHandler::RunL()
{
 this->iWait->AsyncStop();
};

void CAsyncCallHandler::MakeAsyncCall(CTelephony::TNetworkInfoV1Pckg& networkInfo,TRequestStatus& status)
{
  this->iTelephony.GetCurrentNetworkInfo(iStatus,networkInfo);
  this->SetActive();
  this->iWait->Start();
  status=this->iStatus;
}
