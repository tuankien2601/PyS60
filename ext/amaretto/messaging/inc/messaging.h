/**
 * ====================================================================
 * messaging.h
 *
 * Copyright (c) 2007-2009 Nokia Corporation
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
#ifndef __MESSAGING_H__
#define __MESSAGING_H__

#include "Python.h"
#include "symbian_python_ext_util.h"

#include <e32std.h>
#include <eikenv.h>
#include <Gsmuelem.h>

#include <e32base.h>

#include <mtclreg.h>
#include <msvuids.h>
#include <txtrich.h>

#include <smut.h>
#include <smutset.h>
#include <smsclnt.h>
#include <smscmds.h>
#include <smuthdr.h>

#include <pythread.h>

#define MaxMessageLength 39015
#define MaxTelephoneNumberLength 30
#define MaxNameLength 60

/* The thread exit function which deletes the messaging object */
extern "C" void delete_mes_object(void);

/*
 *
 * Implementation of messaging.sms_send
 * (based on S60 SDK 2.0 example)
 *
 */
class MMsvObserver
{
 public:
  enum TStatus {ECreated, EMovedToOutBox, EScheduledForSend, ESent, EDeleted};
  enum TError {EScheduleFailed=5, ESendFailed, ENoServiceCentre, EFatalServerError};

  virtual void HandleStatusChange(TStatus aStatus) = 0;
  virtual void HandleError(TError aError) = 0;
};

/*
 * Python callback class
 * 
 */
#ifndef EKA2
class TPyMesCallBack
#else
NONSHARABLE_CLASS(TPyMesCallBack)
#endif
  {
  public: 
    TInt MessagingEvent(TInt aArg);
  public:
    PyObject* iCb; // Not owned.
  private: 
  };

/*
 *
 * The actual messaging handler
 *
 */
#ifndef EKA2
class CMsvHandler : public CActive, public MMsvSessionObserver
#else
NONSHARABLE_CLASS(CMsvHandler) : public CActive, public MMsvSessionObserver
#endif
{
 public:
  ~CMsvHandler();
  virtual TBool IsIdle() {
    return (iMtmRegistry ? ETrue : EFalse);
    }

 protected: // from CActive
  CMsvHandler(MMsvObserver&);
 
  void DoCancel() { if (iOperation) iOperation->Cancel(); }
  void ConstructL() { iSession = CMsvSession::OpenAsyncL(*this); }
  virtual void CompleteConstructL() { 
    iMtmRegistry = CClientMtmRegistry::NewL(*iSession); 
  }

  virtual void SetMtmEntryL(TMsvId);
  virtual void DeleteEntryL(TMsvEntry& aMsvEntry);
  
 protected:
  CMsvOperation*      iOperation;
  CMsvSession*        iSession;
  CBaseMtm*           iMtm;
  CClientMtmRegistry* iMtmRegistry;
  MMsvObserver&       iObserver;
};

/*
 *
 * SMS handler
 *
 */
#ifndef EKA2
class CSmsSendHandler : public CMsvHandler
#else
NONSHARABLE_CLASS(CSmsSendHandler) : public CMsvHandler
#endif
{
 public:
  static CSmsSendHandler* NewL(MMsvObserver&, const TDesC&, const TDesC&, const TDesC&, const TInt);
  static CSmsSendHandler* NewLC(MMsvObserver&, const TDesC&, const TDesC&, const TDesC&, const TInt);

  ~CSmsSendHandler();
 public: // from CMsvHandler
  TBool IsIdle();

 public: // from MMsvSessionObserver
  void HandleSessionEventL(TMsvSessionEvent aEvent, TAny* aArg1, TAny* aArg2, TAny* aArg3);

 protected:
  void RunL();
  CSmsSendHandler(MMsvObserver& aObserver, const TDesC& aTelNum, const TDesC& aMsgBody, const TDesC& aRecipientName, const TInt encoding ) :
    CMsvHandler(aObserver), iPhase(EIdle), iTelNum(aTelNum), iMessageText(aMsgBody), iRecipientName(aRecipientName), iEncoding(encoding) {;}

 private:
  void ConstructL() {CMsvHandler::ConstructL();}
  void CreateNewMessageL();
  TBool SetupSmsHeaderL();
  void PopulateMessageL(TMsvEntry& aMsvEntry);
  TBool InitializeMessageL();
  TBool MoveMessageEntryL(TMsvId);
  void SetScheduledSendingStateL(CMsvEntrySelection&);
  void HandleChangedEntryL(TMsvId);
  void SendToL() { CreateNewMessageL(); };
 private:
  enum TPhase {EIdle, EWaitingForCreate, EWaitingForMove,
               EWaitingForScheduled, EWaitingForSent, EWaitingForDeleted};

  TPhase                           iPhase;
  TBuf<MaxTelephoneNumberLength>   iTelNum;
  TBuf<MaxMessageLength>           iMessageText;
  TBuf<MaxNameLength>              iRecipientName;
  TInt                             iEncoding;
};

/*
 *
 * Observer for the status and errors.
 *
 */
#ifndef EKA2
class SmsObserver: public MMsvObserver
#else
NONSHARABLE_CLASS(SmsObserver) : public MMsvObserver
#endif
{
public:
  SmsObserver():iStatus(KErrNone) {;} 
  virtual ~SmsObserver() {}
  void SetCallBack(TPyMesCallBack &aCb);
  
  void HandleStatusChange(MMsvObserver::TStatus aStatus);
  void HandleError(MMsvObserver::TError aError);
  
private:
  TInt iStatus; // XXX Remove me
  TPyMesCallBack iCallMe;
};

//////////////TYPE DEFINITION/////////////////

#define MES_type ((PyTypeObject*)SPyGetGlobalString("MESType"))

struct MES_object {
  PyObject_VAR_HEAD
  CMsvHandler* messaging;
  TPyMesCallBack myCallBack;
  TBool callBackSet;
  MMsvObserver* smsObserver;
};

#endif /*__MESSAGING_H__*/
