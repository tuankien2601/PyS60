/* Copyright (c) 2009 Nokia Corporation
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

#include <e32std.h>
#include <e32base.h>
#include <Python.h>
#include <pthread.h>

struct launchpad_args
{
    void (*f)(void *);
    void* arg;
};

class CPyThreadRoot: public CActive
{
public:
    CPyThreadRoot(TThreadFunction& aFunc, TAny* aArg) :
        CActive(EPriorityStandard), iFunc(aFunc), iArg(aArg)
    {
        CActiveScheduler::Add(this);
    }

    void Start()
    {
        iStatus = KRequestPending;
        SetActive();
        //iStatus = KErrNone; // Doing this will panic the 3.0 emulator with E32USER-CBase 46 "Stray signal".
        TRequestStatus* pstatus = &iStatus;
        RThread().RequestComplete(pstatus, 0);
        CActiveScheduler::Start();
    }

private:
    void DoCancel(){;}
    void RunL()
    {
        iFunc(iArg);
        CActiveScheduler::Stop();
    }

    TThreadFunction iFunc;
    TAny* iArg;
};

extern "C"
{

void launchpad(void* p)
{
    TThreadFunction func = (TThreadFunction) ((launchpad_args*) p)->f;
    TAny* arg = ((launchpad_args*) p)->arg;
    //PyMem_DEL(p);

    CTrapCleanup* cleanup_stack = CTrapCleanup::New();
    TRAPD(error,
            {
                CActiveScheduler* as = new (ELeave) CActiveScheduler;
                CleanupStack::PushL(as);
                CActiveScheduler::Install(as);

                CPyThreadRoot* pytroot = new (ELeave) CPyThreadRoot(func, arg);
                CleanupStack::PushL(pytroot);

                pytroot->Start();

                CleanupStack::PopAndDestroy();
                CleanupStack::PopAndDestroy();
            });

    __ASSERT_ALWAYS(!error, User::Panic(_L("Python thread"), error));

    delete cleanup_stack;
    RThread().Terminate(0); //pthread_exit(0) does not cleanup all the resources?
}

} // extern "C"
