/*
* ====================================================================
*  Python_launcher.cpp
*  
*  Launchpad EXE for starting Python server scripts 
*
* Copyright (c) 2005-2008 Nokia Corporation
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

#include <python.h>
#include <e32base.h>
#include <e32std.h>
#include <utf.h>

TInt RunScript(int argc, char** argv)
{
  if (argc < 1)
    return KErrArgument;
  char* filename = argv[0];

  FILE *fp = fopen(filename, "r");
  if (!fp)
    return KErrNotFound;

  PySys_SetArgv(argc, argv);
  int err = PyRun_SimpleFile(fp, filename);
  fclose(fp);

  return (err ? KErrGeneral : KErrNone);
}

static TInt AsyncRunCallbackL(TAny* aArg)
{
  char *argv[1];
  int argc;
  TBuf8<KMaxFileName> namebuf;

  CnvUtfConverter::ConvertFromUnicodeToUtf8(namebuf, *((TDesC*)aArg));
  argv[0] = (char*)namebuf.PtrZ();
  argc = 1;

  Py_VerboseFlag = EFalse;
  Py_NoSiteFlag = EFalse;
  Py_TabcheckFlag = EFalse;
  Py_DebugFlag = EFalse;
  
  SPy_DLC_Init();
  SPy_SetAllocator(SPy_DLC_Alloc, SPy_DLC_Realloc, SPy_DLC_Free, NULL);
  Py_Initialize();
  
  PyGILState_STATE gstate = PyGILState_Ensure();
  TInt error = RunScript(argc, argv);
  PyGILState_Release(gstate);
  
  Py_Finalize();
  SPy_DLC_Fini();

  CActiveScheduler::Stop();

  return (error ? KErrGeneral : KErrNone);
}

static void RunServerL(const TDesC& aScriptName)
{
  CActiveScheduler* as = new (ELeave) CActiveScheduler;
  CleanupStack::PushL(as);
  CActiveScheduler::Install(as);
  
  TCallBack cb(&AsyncRunCallbackL, (TAny*)&aScriptName);
  CAsyncCallBack* async_callback =
    new (ELeave) CAsyncCallBack(cb, CActive::EPriorityHigh);
  CleanupStack::PushL(async_callback);
  async_callback->CallBack();

  CActiveScheduler::Start();

  CleanupStack::PopAndDestroy(2);
}

GLDEF_C TInt E32Main()
{  
  TInt error;
  TFileName script;
  User::CommandLine(script);
  CTrapCleanup* cleanupStack = CTrapCleanup::New();
  
  TRAP(error, RunServerL(script));
  if (error != KErrNone)
    User::Panic(_L("Python server script"), error);

  delete cleanupStack;
  return KErrNone;
}
