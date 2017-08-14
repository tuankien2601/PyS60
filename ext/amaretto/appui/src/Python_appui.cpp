/*
 * Copyright (c) 2005 - 2009 Nokia Corporation
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

#include <utf.h>
#include "Python_appui.h"
#include "mem_wrapper.h"
#include "Container.h"
#include "pythread.h"

void initappuifw();
void finalizeappuifw();

//
// class CAmarettoCallback
//

void CAmarettoCallback::Call(void* aArg) {
  TInt error = CallImpl(aArg);     // Enter (potentially) interpreter!
  iAppUi->ReturnFromInterpreter(error);
  User::LeaveIfError(error);
}


//
// class CAmarettoAppUi
//

TInt CAmarettoAppUi::EnableTabs(const CDesCArray* aTabTexts,
                                CAmarettoCallback* aFunc)
{
  TRAPD(error, ((CAmarettoContainer*)iContainer)->
        EnableTabsL(aTabTexts, aFunc));
  return error;
}

void CAmarettoAppUi::SetActiveTab(TInt aIndex)
{
  ((CAmarettoContainer*)iContainer)->SetActiveTab(aIndex);
}

EXPORT_C void CAmarettoAppUi::DisablePointerForwarding(TBool aValue)
{
  ((CAmarettoContainer*)iContainer)->DisablePointerForwarding(aValue);
}

void CAmarettoAppUi::RefreshDirectionalPad()
{
    ((CAmarettoContainer*)iContainer)->RefreshDirectionalPad();
}

TInt CAmarettoAppUi::SetHostedControl(CCoeControl* aControl,
                                      CAmarettoCallback* aFunc, TInt aControlName)
{
  TInt rval = KErrNone;
  TRAPD(error, (rval = ((CAmarettoContainer*)iContainer)->
                SetComponentL(aControl, aFunc, aControlName)));
  return ((rval != KErrNone) ? rval : error);
}

void CAmarettoAppUi::RefreshHostedControl()
{
  ((CAmarettoContainer*)iContainer)->Refresh();
}

void CAmarettoAppUi::ConstructL()
{
  TInt error;
#if SERIES60_VERSION>=20
  BaseConstructL(EAknEnableSkin);
#else
  BaseConstructL();
#endif /* SERIES60_VERSION */

  iContainer = CAmarettoContainer::NewL(ClientRect(), this);
  AddToStackL(iContainer);

  SPy_DLC_Init();
  SPy_SetAllocator(SPy_DLC_Alloc, SPy_DLC_Realloc, SPy_DLC_Free, NULL);
  Py_Initialize();

  iInterpreterExitPending = EFalse;

  SetKeyBlockMode(ENoKeyBlock);
#if SERIES60_VERSION>=28
  SetOrientationL(EAppUiOrientationAutomatic);
#endif

  initappuifw();
}

CAmarettoAppUi::~CAmarettoAppUi()
{
  RemoveFromStack(iContainer);
  delete iContainer;
  if (iDoorObserver)
    iDoorObserver->NotifyExit(MApaEmbeddedDocObserver::EEmpty);

  if (aSubPane) {
    /* no deletion since the pointer doesn't referes to any new allocated object */
    aSubPane = NULL;
  }
}

void CAmarettoAppUi::HandleCommandL(TInt aCommand)
{
  switch (aCommand) {
  case EAknSoftkeyExit:
    if (iExitFunc) {
      iExitFunc->Call();
      break;
    }
  case EAknSoftkeyBack:
  case EAknCmdExit:
  case EEikCmdExit:
    DoExit();
    break;
  default:
    if (iMenuCommandFunc && (aCommand >= EPythonMenuExtensionBase) &&
        (aCommand < (EPythonMenuExtensionBase+KMaxPythonMenuExtensions)))
      iMenuCommandFunc->Call((void*)aCommand);
    break;
  }
}

void CAmarettoAppUi::HandleForegroundEventL(TBool aForeground)
{
  if (iFocusFunc) {
    iFocusFunc->Call((void*)aForeground);
  }
  CAknAppUi::HandleForegroundEventL(aForeground);

  ((CAmarettoContainer*)iContainer)->RefreshDirectionalPad();
}

void CAmarettoAppUi::ReturnFromInterpreter(TInt aError)
{
  if (iInterpreterExitPending) {
    if (iDoorObserver)
      /* TODO leave should be handled here? */
      ProcessCommandL(EAknCmdExit);
    else
      DoExit();
  }
}

void CAmarettoAppUi::HandleResourceChangeL(TInt aType)
{
  CAknAppUi::HandleResourceChangeL(aType);
  if ( aType == KEikDynamicLayoutVariantSwitch ) {
      SetScreenmode(iScreenMode);
  }

}

void CAmarettoAppUi::DoRunScriptL()
{
  // Enter interpreter!
  TInt error = RunScript();
  ReturnFromInterpreter(error);
  User::LeaveIfError(error);
}

TInt CAmarettoAppUi::RunScript()
{
  PyGILState_STATE gstate = PyGILState_Ensure();

  // When an application is started, the PWD is its private directory.
  // Run launcher.py from the private directory.
  char *runCmd = "try:\n"
                 "    execfile('launcher.py')\n"
                 "except:\n"
                 "    import sys\n"
                 "    import traceback\n"
                 "    import appuifw\n"
                 "    import series60_console\n"
                 "    my_console = series60_console.Console()\n"
                 "    appuifw.app.body = my_console.text\n"
                 "    sys.stderr = sys.stdout = my_console\n"
                 "    traceback.print_exc()\n"
                 "    print 'Press Exit key to quit!'\n";

  int err = PyRun_SimpleString(runCmd);

  PyGILState_Release(gstate);

  return (err ? KErrGeneral : KErrNone);
}

void CAmarettoAppUi::DoExit()
{
  iInterpreterExitPending = EFalse;
  PrepareToExit();
  /* Exiting in any case, hence not processing error *
   * Trying to trap here also leads to a panic       *
   * L suffix should be added to the method name     */
  DeactivateActiveViewL();
  finalizeappuifw();
  Py_Finalize();
  free_pthread_locks();
  free_all_allocations();
  SPy_DLC_Fini();
  Exit();
}

void CAmarettoAppUi::DynInitMenuPaneL(TInt aMenuId, CEikMenuPane* aMenuPane)
{
  TAmarettoMenuDynInitParams params;

  params.iMenuId=aMenuId;
  params.iMenuPane=aMenuPane;

  if ( ((aMenuId == iExtensionMenuId) && iMenuDynInitFunc)
       || (aMenuId >= R_PYTHON_SUB_MENU_00) )
    iMenuDynInitFunc->Call((void*)&params);
}

void CAmarettoAppUi::CleanSubMenuArray()
{
  for (int i=0; i<KMaxPythonMenuExtensions; i++)
    subMenuIndex[i] = 0;
}

void CAmarettoAppUi::SetScreenmode(TInt aMode)
{
  if (aMode == EFull) {
    iScreenMode = EFull;
    iContainer->SetRect( ApplicationRect() );
  }
  else if (aMode == ELarge) {
    CEikStatusPane* statusPane = (CEikonEnv::Static())->AppUiFactory()->StatusPane();
    TRAPD(error, (statusPane->SwitchLayoutL(R_AVKON_STATUS_PANE_LAYOUT_EMPTY) ));
    if (error != KErrNone) {
      SPyErr_SetFromSymbianOSErr(error);
             }
    iScreenMode = ELarge;
    iContainer->SetRect( ClientRect() );
  }
  else {
    CEikStatusPane* statusPane = (CEikonEnv::Static())->AppUiFactory()->StatusPane();
      TRAPD(error, (statusPane->SwitchLayoutL(R_AVKON_STATUS_PANE_LAYOUT_USUAL) ));
    if (error != KErrNone) {
         SPyErr_SetFromSymbianOSErr(error);
    }

    iScreenMode = ENormal;
    iContainer->SetRect( ClientRect() );
  }
}

TInt AsyncRunCallbackL(TAny* aArg)
{
  CAmarettoAppUi* appui = (CAmarettoAppUi*)aArg;
  delete appui->iAsyncCallback;
  appui->iAsyncCallback = NULL;

  appui->DoRunScriptL();

  return KErrNone;
}

TBool CAmarettoAppUi::ProcessCommandParametersL(TApaCommand aCommand,
                                             TFileName& /*aDocumentName*/,
                                             const TDesC8& /*aTail*/)
{
  if (aCommand == EApaCommandRun) {


    //    TCallBack cb(&AsyncRunCallbackL);
    //    iAsyncCallback = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityHigh);
    //    iAsyncCallback->CallBack();

    DoRunScriptL();

    return EFalse;
  }
  else
    return ETrue;
}

void CAmarettoAppUi::RunScriptL(const TDesC& aFileName, const TDesC* aArg)
{
  iScriptName.Copy(aFileName);
  if (aArg)
    CnvUtfConverter::ConvertFromUnicodeToUtf8(iEmbFileName, *aArg);

  TCallBack cb(&AsyncRunCallbackL, this);
  iAsyncCallback = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityHigh);
  iAsyncCallback->CallBack();
}

EXPORT_C CEikAppUi* CreateAppUiInstance()
{
  TInt extensionId = R_PYTHON_EXTENSION_MENU;
  return new CAmarettoAppUi(extensionId);
}

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
{
  return (KErrNone);
}
#endif /*EKA2*/
