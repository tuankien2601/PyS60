/*
* ====================================================================
*  messagingadapter.cpp  
*  
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

#include "messaging.h"

// A helper function for the implementation of callbacks
// from C/C++ code to Python callables (modified from appuifwmodule.cpp)
TInt TPyMesCallBack::MessagingEvent(TInt aArg)
  {
  PyGILState_STATE gstate = PyGILState_Ensure();

  TInt error = KErrNone;
  
  PyObject* arg = Py_BuildValue("(i)", aArg);
  PyObject* rval = PyEval_CallObject(iCb, arg);
  
  Py_DECREF(arg);
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
      PyErr_Fetch(&type, &value, &traceback);
      if (PyInt_Check(value))
        error = PyInt_AS_LONG(value);
      Py_XDECREF(type);
      Py_XDECREF(value);
      Py_XDECREF(traceback);
    } else {
      PyErr_Print();
    }
  }
  else
    Py_DECREF(rval);  

  PyGILState_Release(gstate);
  return error;
  }

/*
 *
 * Implementation of messaging.sms_send
 * (based on S60 SDK 2.0 example)
 *
 */

CMsvHandler::CMsvHandler(MMsvObserver& aObserver) :
  CActive(EPriorityStandard), 
  iOperation(NULL), 
  iSession(NULL), 
  iMtm(NULL), 
  iMtmRegistry(NULL), 
  iObserver(aObserver) 
{
  CActiveScheduler::Add(this);
}

CMsvHandler::~CMsvHandler()
{
  Cancel(); 
  delete iOperation;
  iOperation = NULL;
  delete iMtm;
  iMtm = NULL;
  delete iMtmRegistry;
  iMtmRegistry = NULL;
  delete iSession;    // session must be deleted last (and constructed first)
  iSession = NULL;
}

void CMsvHandler::SetMtmEntryL(TMsvId aEntryId)
{
  CMsvEntry* entry = iSession->GetEntryL(aEntryId);
  CleanupStack::PushL(entry);
    
  if ((iMtm == NULL) || (entry->Entry().iMtm != (iMtm->Entry()).Entry().iMtm)) {
    delete iMtm;
    iMtm = NULL;
    iMtm = iMtmRegistry->NewMtmL(entry->Entry().iMtm);
  }

  iMtm->SetCurrentEntryL(entry);
  CleanupStack::Pop(entry); 
}

void CMsvHandler::DeleteEntryL(TMsvEntry& aMsvEntry)
{
  CMsvEntry* parentEntry = CMsvEntry::NewL(*iSession, aMsvEntry.Parent(), TMsvSelectionOrdering());
  CleanupStack::PushL(parentEntry);
  iOperation = parentEntry->DeleteL(aMsvEntry.Id(), iStatus);
  CleanupStack::PopAndDestroy(parentEntry);
  SetActive();
}

/*
 *
 * SMS handler
 *
 */
CSmsSendHandler* CSmsSendHandler::NewL(MMsvObserver& aObserver, 
            const TDesC& aTelNum, const TDesC& aMsgBody, const TDesC& aRecipientName, const TInt encoding)
{
  CSmsSendHandler* self = CSmsSendHandler::NewLC(aObserver, aTelNum, aMsgBody, aRecipientName, encoding);
  CleanupStack::Pop();
  return self;
}

CSmsSendHandler* CSmsSendHandler::NewLC( MMsvObserver& aObserver, 
            const TDesC& aTelNum, const TDesC& aMsgBody, const TDesC& aRecipientName, const TInt encoding)
{
    CSmsSendHandler* self = new (ELeave) CSmsSendHandler(
                                      aObserver, aTelNum, aMsgBody, aRecipientName, encoding);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
}

CSmsSendHandler::~CSmsSendHandler()
{
    // No implementation required
}

TBool CSmsSendHandler::IsIdle()
{
  if (CMsvHandler::IsIdle() && (iPhase == EIdle)) {
    return ETrue;
  }
  else {
    return EFalse;
  }
}

void CSmsSendHandler::RunL()
{
  ASSERT(iOperation);
  User::LeaveIfError(iStatus.Int());
  
  TMsvLocalOperationProgress progress;
  
  TUid mtmUid = iOperation->Mtm();

  if (mtmUid == KUidMsvLocalServiceMtm) {
    progress = McliUtils::GetLocalProgressL(*iOperation);
    User::LeaveIfError(progress.iError);
  }
  else {
    if (iPhase != EWaitingForScheduled)
      /* User::Panic(KSms, ESmsStateError); */
      User::Leave(KErrUnknown); 
  }

  delete iOperation;
  iOperation = NULL;
  
  switch (iPhase) {
    case EWaitingForCreate:
      iObserver.HandleStatusChange(MMsvObserver::ECreated);
      SetMtmEntryL(progress.iId);
#ifdef __WINS__
      if (InitializeMessageL() && MoveMessageEntryL(KMsvDraftEntryId))
#else
      if (InitializeMessageL() && MoveMessageEntryL(KMsvGlobalOutBoxIndexEntryId))
#endif
        iPhase = EWaitingForMove;
      else
        iPhase = EIdle;
      break;
    case EWaitingForMove:
    {
      iObserver.HandleStatusChange(MMsvObserver::EMovedToOutBox);
      CMsvEntrySelection* selection = new (ELeave) CMsvEntrySelection;
      CleanupStack::PushL(selection);
      
      selection->AppendL(progress.iId);
      SetScheduledSendingStateL(*selection);
      CleanupStack::PopAndDestroy(selection);
      iPhase = EWaitingForScheduled;
    }
    break;
  case EWaitingForScheduled:
    {
      const TMsvEntry& msvEntry = iMtm->Entry().Entry();
      TInt state = msvEntry.SendingState();
      if (state != KMsvSendStateScheduled) {
        iObserver.HandleError(MMsvObserver::EScheduleFailed);
        iPhase = EIdle;
      }
      else {
        iObserver.HandleStatusChange(MMsvObserver::EScheduledForSend);
        iPhase = EWaitingForSent;
      }
    }
    break;
  case EWaitingForDeleted:
    iObserver.HandleStatusChange(MMsvObserver::EDeleted);
    iPhase = EIdle;
    break;
  case EWaitingForSent:  // We handle this in HandleSessionEventL
  case EIdle:            // Shouldn't get triggered in this state
  default:
    ASSERT(EFalse);
  }
}

void CSmsSendHandler::CreateNewMessageL()
{
  TMsvEntry newEntry;              // This represents an entry in the Message Server index
  newEntry.iMtm = KUidMsgTypeSMS;                         // message type is SMS
  newEntry.iType = KUidMsvMessageEntry;                   // this defines the type of the entry: message 
  newEntry.iServiceId = KMsvLocalServiceIndexEntryId;     // ID of local service (containing the standard folders)
  newEntry.iDate.UniversalTime();

  newEntry.SetInPreparation(ETrue);                       // a flag that this message is in preparation

  CMsvEntry* entry = CMsvEntry::NewL(*iSession, KMsvDraftEntryIdValue, TMsvSelectionOrdering());
  CleanupStack::PushL(entry);

  iOperation = entry->CreateL(newEntry, iStatus);
  CleanupStack::PopAndDestroy(entry);

  SetActive();
  iPhase = EWaitingForCreate;
}

TBool CSmsSendHandler::SetupSmsHeaderL()
{
  if (iMtm) {
    CSmsClientMtm* smsMtm = static_cast<CSmsClientMtm*>(iMtm);
    smsMtm->RestoreServiceAndSettingsL();
    
    CSmsHeader&   header          = smsMtm->SmsHeader();
    CSmsSettings& serviceSettings = smsMtm->ServiceSettings();

    CSmsSettings* sendOptions = CSmsSettings::NewL();
    CleanupStack::PushL(sendOptions);
    sendOptions->CopyL(serviceSettings); // restore existing settings
    sendOptions->SetDelivery(ESmsDeliveryImmediately);      // set to be delivered immediately
    sendOptions->SetCharacterSet(static_cast<TSmsDataCodingScheme::TSmsAlphabet>(iEncoding)) ;

    header.SetSmsSettingsL(*sendOptions);
    CleanupStack::PopAndDestroy(sendOptions);
    
    const TInt numSCAddresses = serviceSettings.ServiceCenterCount();
          
    if (header.Message().ServiceCenterAddress().Length() == 0) {
      if (numSCAddresses != 0) {           
        // set sc address to default. 
        CSmsServiceCenter& sc = serviceSettings.GetServiceCenter(
                              serviceSettings.DefaultServiceCenter() );
        header.Message().SetServiceCenterAddressL( sc.Address() );          
      }
      else {
        // here there could be a dialog in which user can add sc number
        iObserver.HandleError(MMsvObserver::ENoServiceCentre);
        #ifdef __WINS__
          return ETrue; 
        #else
          return EFalse;
        #endif
      }
    }
    smsMtm->SaveMessageL();
    return ETrue;
  }
  else {
    return EFalse;
  }
}

void CSmsSendHandler::PopulateMessageL(TMsvEntry& aMsvEntry)
{
  ASSERT(iMtm);

  CRichText& mtmBody = iMtm->Body();
  mtmBody.Reset();
  mtmBody.InsertL(0, iMessageText); 
  
  //If Recipient name is empty than set iDetails with iTelNum
  if(iRecipientName.Length() < 1)
    aMsvEntry.iDetails.Set(iTelNum);
  else
    aMsvEntry.iDetails.Set(iRecipientName);
  aMsvEntry.SetInPreparation(EFalse);         
  aMsvEntry.iDescription.Set(iMessageText);

  aMsvEntry.SetSendingState(KMsvSendStateWaiting);   
  aMsvEntry.iDate.UniversalTime();          
}

TBool CSmsSendHandler::InitializeMessageL()
{
  ASSERT(iMtm);

  TMsvEntry msvEntry = (iMtm->Entry()).Entry();

  PopulateMessageL(msvEntry);

  if (!SetupSmsHeaderL()) {
    return EFalse;
  }
  
  iMtm->AddAddresseeL(iTelNum, msvEntry.iDetails);
    
  CMsvEntry& entry = iMtm->Entry();
  entry.ChangeL(msvEntry);              
  iMtm->SaveMessageL();
  return ETrue;
}

TBool CSmsSendHandler::MoveMessageEntryL(TMsvId aTarget)
{
  ASSERT(iMtm);

  TMsvEntry msvEntry((iMtm->Entry()).Entry());

  if (msvEntry.Parent() != aTarget) {
    TMsvSelectionOrdering sort;
    sort.SetShowInvisibleEntries(ETrue);    
    CMsvEntry* parentEntry = CMsvEntry::NewL(iMtm->Session(), msvEntry.Parent(), sort);
    CleanupStack::PushL(parentEntry);
    
    iOperation = parentEntry->MoveL(msvEntry.Id(), aTarget, iStatus);
    
    CleanupStack::PopAndDestroy(parentEntry);
    SetActive();

    return ETrue;
  }
  return EFalse;
}

void CSmsSendHandler::SetScheduledSendingStateL(CMsvEntrySelection& aSelection)
{
  ASSERT(iMtm);
  TBuf8<1> dummyParams;
  iOperation = iMtm->InvokeAsyncFunctionL(ESmsMtmCommandScheduleCopy,
					  aSelection, dummyParams, iStatus);
  SetActive();
}

void CSmsSendHandler::HandleChangedEntryL(TMsvId aEntryId)
{
  if (iMtm) {
    TMsvEntry msvEntry((iMtm->Entry()).Entry());
    if (msvEntry.Id() == aEntryId) {
      if (msvEntry.SendingState() == KMsvSendStateSent) {
        #ifndef EKA2 
          iPhase = EWaitingForDeleted;
          iObserver.HandleStatusChange(MMsvObserver::ESent);
          DeleteEntryL(msvEntry);
        #else
          // Deletion happens automatically in EKA2, hence we need to signal
          // both states to the observer and not call DeleteEntryL explicitly
          iObserver.HandleStatusChange(MMsvObserver::ESent);        
          iObserver.HandleStatusChange(MMsvObserver::EDeleted);
          iPhase = EIdle;
        #endif

      }
      else if (msvEntry.SendingState() == KMsvSendStateFailed) {
        iPhase = EIdle;
        iObserver.HandleError(MMsvObserver::ESendFailed);
      }
    }
  }
}

void CSmsSendHandler::HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* /*aArg2*/, TAny* /*aArg3*/)
{
  switch (aEvent) {
  case EMsvEntriesChanged:
    {
      if (iPhase == EWaitingForSent) {
        CMsvEntrySelection* entries = static_cast<CMsvEntrySelection*>(aArg1);
        for(TInt i = 0; i < entries->Count(); i++) {
            HandleChangedEntryL(entries->At(i));
        }
      }
    }
    break;
  case EMsvServerReady:
    CompleteConstructL();       
    SendToL();
    break;
  case EMsvCloseSession:
    iSession->CloseMessageServer();
    break;
  case EMsvServerTerminated:
    iSession->CloseMessageServer();
    break;
  case EMsvGeneralError:
  case EMsvServerFailedToStart:
    iObserver.HandleError(MMsvObserver::EFatalServerError);
    break;
  default:
    break;
  }
}
/*
 *
 * SMS observer
 *
 */

void SmsObserver::SetCallBack(TPyMesCallBack &aCb) 
  {
  iCallMe = aCb;
  }
  
void SmsObserver::HandleStatusChange(MMsvObserver::TStatus aStatus)
{
  switch (aStatus) {
  case EDeleted:
  case ECreated:
  case EMovedToOutBox:
  case EScheduledForSend:  
  case ESent:
  default:
    iCallMe.MessagingEvent((TInt)aStatus);
    break;
  }
}

void SmsObserver::HandleError(MMsvObserver::TError aError)
{
  switch (aError) {
    case ENoServiceCentre:
#ifdef __WINS__
      iStatus = KErrNone;
      iCallMe.MessagingEvent((TInt)aError);      
      break;
#endif 
    case EScheduleFailed:
    case ESendFailed:
    case EFatalServerError:
    default:
      iStatus = KErrGeneral;
      iCallMe.MessagingEvent((TInt)aError); 
      break;
  }
}
