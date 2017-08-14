/**
 * ====================================================================
 * asynccallhandler.h
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

#ifndef __ASYNCCALLHANDLER_H__
#define __ASYNCCALLHANDLER_H__


#include <e32base.h>
#include <etel.h>
#include <etel3rdparty.h>

#ifndef EKA2
class CAsyncCallHandler:public CActive
#else
NONSHARABLE_CLASS (CAsyncCallHandler):public CActive
#endif
{
public:
 void MakeAsyncCall(CTelephony::TNetworkInfoV1Pckg& networkInfo,TRequestStatus& status);
 void RunL();
 void DoCancel();
 static CAsyncCallHandler* NewL(CTelephony& telephony);
 virtual ~CAsyncCallHandler();
private:
 CActiveSchedulerWait* iWait;
 CTelephony& iTelephony;
 CAsyncCallHandler(CTelephony& telephony);
 void ConstructL();
};

#endif /* __ASYNCCALLHANDLER_H__ */










