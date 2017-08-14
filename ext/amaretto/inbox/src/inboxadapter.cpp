/**
 * ====================================================================
 * inboxadapter.cpp
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
 
#include "inbox.h"


// A helper function for the implementation of callbacks
// from C/C++ code to Python callables (modified from appuifwmodule.cpp)
TInt TPyInbCallBack::NewInboxEntryCreated(TInt aArg)
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

//////////////////////////////////////////////

CInboxAdapter* CInboxAdapter::NewL(TMsvId& aFolderType)
  {
  CInboxAdapter* self = NewLC(aFolderType);
  CleanupStack::Pop(self); 
  return self;
  }

CInboxAdapter* CInboxAdapter::NewLC(TMsvId& aFolderType)
  {
  CInboxAdapter* self = new (ELeave) CInboxAdapter();
  CleanupStack::PushL(self);
  self->iFolderType = aFolderType;
  self->ConstructL();
  return self;
  }

void CInboxAdapter::ConstructL()
  {
  iCallBackSet = EFalse;
  iSession = CMsvSession::OpenSyncL(*this); // new session is opened synchronously  
  CompleteConstructL();       // Construct the mtm registry
  }

void CInboxAdapter::CompleteConstructL()
  {
  // We get a MtmClientRegistry from our session
  // this registry is used to instantiate new mtms.
  iMtmReg = CClientMtmRegistry::NewL(*iSession);
  iMtm = iMtmReg->NewMtmL(KUidMsgTypeSMS);        // create new SMS MTM
  }

CInboxAdapter::~CInboxAdapter()
  {
  delete iMtm;
  delete iMtmReg;
  delete iSession; // must be last to be deleted
  }

void CInboxAdapter::DeleteMessageL(TMsvId aMessageId)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);
  iMtm->LoadMessageL();

	TMsvId parent = iMtm->Entry().Entry().Parent();

	iMtm->SwitchCurrentEntryL(parent);

	iMtm->Entry().DeleteL(aMessageId);
	}

void CInboxAdapter::GetMessageTimeL(TMsvId aMessageId, TTime& aTime)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);	
	iMtm->LoadMessageL();

  aTime = (iMtm->Entry().Entry().iDate);
	}

void CInboxAdapter::GetMessageUnreadL(TMsvId aMessageId, TBool& aUnread)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);	
	iMtm->LoadMessageL();

	aUnread = (iMtm->Entry().Entry().Unread());
	}

void CInboxAdapter::SetMessageUnreadL(TMsvId aMessageId, TBool aStatus)
  {
  iMtm->SwitchCurrentEntryL(aMessageId);
  iMtm->LoadMessageL();
  TMsvEntry msvEntryFromMtm = iMtm->Entry().Entry();
  msvEntryFromMtm.SetUnread(aStatus);
  iMtm->Entry().ChangeL(msvEntryFromMtm);
  }
  
void CInboxAdapter::GetMessageAddressL(TMsvId aMessageId, TDes& aAddress)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);	
	iMtm->LoadMessageL();
  TPtrC address = iMtm->Entry().Entry().iDetails;
	TInt length = address.Length();
	
	// Check length because address is read to a limited size TBuf
	if (length >= KMessageAddressLength) 
	  {
		aAddress.Append(address.Left(KMessageAddressLength - 1));
		}
	else 
		{
		aAddress.Append(address.Left(length));
	  }  
	}  

CArrayFixFlat<TMsvId>* CInboxAdapter::GetMessagesL()
  {
  // XXX add here e.g. e-mail and MMS type also:
  CMsvEntry* parentEntry=NULL;
  CMsvEntrySelection* entries=NULL;
  parentEntry = CMsvEntry::NewL(*iSession, 
                                iFolderType, 
                                TMsvSelectionOrdering()); //pointer to Messages Inbox
  entries = parentEntry->ChildrenWithMtmL(KUidMsgTypeSMS); // select sms entries
  delete parentEntry;
  return (CArrayFixFlat<TMsvId>*)entries;
  }

TBool CInboxAdapter::GetMessageL(TMsvId aMessageId, TDes& aMessage)
	{
	iMtm->SwitchCurrentEntryL(aMessageId);

  if (iMtm->Entry().HasStoreL()) 
		{
		// SMS message is stored inside Messaging store
		CMsvStore* store = iMtm->Entry().ReadStoreL();
		CleanupStack::PushL(store);
	
		if (store->HasBodyTextL())
			{
			CEikonEnv* const env = CEikonEnv::Static();
			CParaFormatLayer* pfl;
			CCharFormatLayer* cfl;
			
			if (env)
				{
				pfl = env->SystemParaFormatLayerL();
				cfl = env->SystemCharFormatLayerL();
				}
			else
				{
				pfl = CParaFormatLayer::NewL();
				CleanupStack::PushL(pfl);
				cfl = CCharFormatLayer::NewL();
				CleanupStack::PushL(cfl);
				}

			CRichText* richText = CRichText::NewL(pfl, cfl,CEditableText::EFlatStorage);
			richText->Reset();
			CleanupStack::PushL(richText);

			// Get the SMS body text.
			store->RestoreBodyTextL(*richText);
			const TInt length = richText->DocumentLength();
			TBuf<KMessageBodySize> message;

			// Check length because message is read to limited size TBuf
			if (length >= KMessageBodySize) 
				{
				message.Append(richText->Read(0, KMessageBodySize -1));
				}
			else 
				{
				message.Append(richText->Read(0, length));
				}

			aMessage.Append(message);
			CleanupStack::PopAndDestroy(richText);
			
			if (!env)
				{
				CleanupStack::PopAndDestroy(cfl);
				CleanupStack::PopAndDestroy(pfl);
				}
			}
		
		CleanupStack::PopAndDestroy(store);
		
		}
	else
		{
		return EFalse;
		}

	return ETrue;
	}

void CInboxAdapter::SetCallBack(TPyInbCallBack &aCb) 
  {
  iCallMe = aCb;
  iCallBackSet = ETrue;
  }

void CInboxAdapter::HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* /*aArg3*/)
  {
      TInt i; 
      switch(aEvent) {
	      // New entries only
        case EMsvEntriesCreated:
          {
          if (iCallBackSet) {
            // Messages that are created in Inbox
            TMsvId* parent;
	          parent = static_cast<TMsvId*>(aArg2);

            // XXX is this needed for Outbox etc.?
            // Check the parent folder to be global inbox
	          if(*parent != KMsvGlobalInBoxIndexEntryId) 
	            return;

	          CMsvEntrySelection* entries = static_cast<CMsvEntrySelection*>(aArg1);
	          for(i = 0; i < entries->Count(); i++) {
	            iErrorState = iCallMe.NewInboxEntryCreated(static_cast<TInt>(entries->At(i)));
	          }
          }
	        break;
          }
        default:
	        break;
     }
  }
