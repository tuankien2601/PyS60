/* Copyright (c) 2008 - 2009 Nokia Corporation
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

//  Include Files  
#include "buildtime.h"
#include <e32base.h>
#include <e32std.h>
#include <e32cons.h>            // Console
#include <f32file.h>            //File operation 
#include <utf.h>



//  Constants
_LIT(KTextConsoleTitle, "Console");
_LIT(KExeToRun, "c:\\sys\\bin\\interpreter-startup.exe");
_LIT(KBuildFile, "\\data\\python\\test\\regrtest_emu.log");
_LIT(KTempFile, "\\data\\python\\test\\temp.log");
_LIT8( KTxtFormat, "Measurement -- Name : <Interpreter start_up>, Value : <%f>, \
Unit : <seconds>, Threshold : <0.1>, higher_is_better : <no>  \n" );

LOCAL_D CConsoleBase* console;


//  Global Functions

GLDEF_C TInt E32Main()
{
    // Create cleanup stack
    __UHEAP_MARK;
    CTrapCleanup* cleanup = CTrapCleanup::New();

    // Create output console
    TRAPD(createError, console = Console::NewL(KTextConsoleTitle, TSize(KConsFullScreen, KConsFullScreen)))
    ;
    if (createError)
        return createError;

    TTime iStartTime, iEndTime;
    TBuf8<20> iRead;
    TBuf8<150> iWriteTofile;
    TBuf<20> iConv;
  
    iStartTime.UniversalTime();
    RProcess proc;
    TInt iReturn = proc.Create(KExeToRun, KNullDesC);
    
    if (iReturn==KErrNone)
    {
        TRequestStatus status;
        proc.Rendezvous(status);
        if (status!=KRequestPending)
            proc.Kill(0); // abort startup
        else
            proc.Resume(); // logon OK - start the current process
        User::WaitForRequest(status);// wait for completion
        proc.Close();
    }
    
    RFs fsSession;
    RFile buildtime;
    User::LeaveIfError(fsSession.Connect());
    buildtime.Open(fsSession, KTempFile, EFileWrite);

    buildtime.Read(iRead);
    CnvUtfConverter::ConvertToUnicodeFromUtf8(iConv, iRead);
           
    //Lex conversion
    TLex iLex(iConv);
    TInt64 iVal;
    iLex.Val(iVal, EDecimal);
    
    iEndTime = iVal;
    TTimeIntervalMicroSeconds iDiff = iEndTime.MicroSecondsFrom(iStartTime);
   
    TReal iPy_InitializeTime = (TReal)(iDiff.Int64())/1000000;
    iWriteTofile.Format(KTxtFormat, iPy_InitializeTime);
    
    buildtime.Close();
    User::LeaveIfError(fsSession.Delete(KTempFile));

    RFile writefile;
    writefile.Replace(fsSession, KBuildFile, EFileWrite);
        
    writefile.Write(iWriteTofile);
    fsSession.Close();
    delete console;
    delete cleanup;
    __UHEAP_MARKEND;
    return KErrNone;
}

