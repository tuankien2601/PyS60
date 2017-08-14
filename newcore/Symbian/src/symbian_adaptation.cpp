/* Copyright (c) 2008-2009 Nokia Corporation
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
 */

#include <e32std.h>
#include <e32cmn.h>
#include <hal.h>
#include "pyconfig.h"
#include "symbian_adaptation.h"

extern "C" {


int symbian_check_stack()
{
    TThreadStackInfo stackInfo;
    if (RThread().StackInfo(stackInfo) == KErrNone)
    {
        /* Obtain an approximation of the stack pointer by getting 
         * the address of a stack-allocated variable, and check if there is
         * enough space between that and the stack limit. 
         * _SPy_StackCheckSize is an arbitrary "large enough" number 
         * defined in pyconfig.h */
        if (((TUint)&stackInfo-(TUint)stackInfo.iLimit) > SPY_STACKCHECKSIZE)
            return 0; 
        else
            return -1;
     }
     else 
     {
         /* Querying the stack failed - this basically shouldn't happen in any sane situation.
          * The documentation says "KErrGeneral, if the thread doesn't have a user 
          * mode stack, or it has terminated."
          */      
         return -1; 
     }
}

double symbian_clock()
{
    TInt period = 1000*1000;

    HAL::Get(HALData::ESystemTickPeriod, period);

    /* Return TickCount in seconds */
    return (double)User::TickCount() * ((double)period / (1000.0 * 1000.0));
}

}
