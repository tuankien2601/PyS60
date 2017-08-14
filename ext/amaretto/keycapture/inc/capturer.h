/**
 * ====================================================================
 * capturer.h
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


#include "Python.h"
#include "symbian_python_ext_util.h"

#include <w32std.h>
#include <e32base.h>
#include <apgwgnam.h>
#include <coedef.h>


#ifndef EKA2
class CCapturer : public CActive
#else
NONSHARABLE_CLASS(CCapturer) : public CActive
#endif
{
  public:
    ~CCapturer();
    static CCapturer* NewL(PyObject* callback);
    TInt32 SetKeyToBeCapturedL(TInt32 keyCode);
    void RemoveKey(TInt32 keyId);
    void StartCapturing();
    void RunL();
    void DoCancel();
    TInt32 LastCapturedKey();
    void SetForwarding(TInt forwarding);
  private:
    CCapturer(PyObject* aCallback = NULL);
    void ConstructL();
    RWsSession* iSession;
    RWindowGroup* iWinGroup;
    CApaWindowGroupName* iWinGroupName;
    PyObject* iCallback;
    TInt32 iLastCapturedKey;
    TBool iCallbackRunning;
    TBool iForwarding;
    TBool iRunning;
};

