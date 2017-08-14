/*
* ====================================================================
*  e32socketmodule.cpp
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

#if SERIES60_VERSION>=26
#define __BC70s__
#endif
#include <eikenv.h>
#include <e32std.h>
#include <es_sock.h>
#include <in_sock.h>
#include <bt_sock.h>
#include <btmanclient.h>
#include <bttypes.h>
#include <btextnotifiers.h>
#include <btsdp.h>
#include <OBEX.h>
#include "staticarrayc.h"
#ifdef HAVE_SSL
#include <SecureSocket.h>
#endif


#include <CommDbConnPref.h>

// If building for 2.8. These headers must be copied from 2.6 SDk
#include <aplistitemlist.h> 
#include <aputils.h> 

#include <COMMDB.H>

#include <apselect.h>

#include <Python.h>
#include <pythread.h>
#include <object.h>
#include "apselection.h"
#include <symbian_python_ext_util.h>

#ifdef PRESDK20
#define KProtocolUnknown 0xdead
#endif

#define MaxIPAddrLength 16
#define MaxBufferSize 1024
#define MaxAttributeSize 48
#define MaxServiceNameSize 48

#define KAfBT 0
#define KWaitAll 0x100     //random number to implement socket flag MSG_WAITALL

#define RfcommService 101  //random numbers
#define ObexService   102

#define KAUT 0x0001
#define KENC 0x0002
#define KAUTHOR 0x0004

const TInt KRfcommChannel = 1;
const TUint KBtProtocolIdOBEX = 0x0008;

const TUint KObServiceClass = 0x1105; 
const TInt KRFCServiceClass = 0x1101; 

//#define KProtocol_TLS 1502  //random numbers
//#define KProtocol_SSL 1503

_LIT(KRFCServiceDescription,"Rfcomm Transfer");
_LIT(KObServiceDescription,"Object Exchange");
_LIT(KServerTransportName,"RFCOMM");

PyObject *PySocket_Error;

static PyObject * PySocket_Err(TInt);
static PyObject * PySocket_Err(char*);


/*
 *.........................Bluetooth.............................*
 */
/*
 * implementation of virtual class MSdpAttributeValueVisitor
 */
enum TSdpAttributeParserPanics 
{
    ESdpAttributeParserInvalidCommand = 1,
    ESdpAttributeParserNoValue,
    ESdpAttributeParserValueIsList,
    ESdpAttributeParserValueTypeUnsupported
};

enum TBTServiceSearcherPanics 
{
    EBTServiceSearcherNextRecordRequestComplete = 1,
    EBTServiceSearcherAttributeRequestResult,
    EBTServiceSearcherAttributeRequestComplete,
    EBTServiceSearcherInvalidControlIndex,
    EBTServiceSearcherProtocolRead,
    EBTServiceSearcherAttributeRequest,
    EBTServiceSearcherSdpRecordDelete
};

class MSdpAttributeNotifier 
{
 public:
    virtual void FoundElementL(TInt aKey, CSdpAttrValue& aValue) = 0;
};

#ifndef EKA2
class TSdpAttributeParser : public MSdpAttributeValueVisitor 
#else
NONSHARABLE_CLASS(TSdpAttributeParser) : public MSdpAttributeValueVisitor 
#endif
{
 public:
    enum TNodeCommand { ECheckType, ECheckValue, ECheckEnd, ESkip, EReadValue, EFinished };
    struct SSdpAttributeNode { TNodeCommand iCommand; TSdpElementType iType; TInt iValue; };
    typedef const TStaticArrayC<SSdpAttributeNode> TSdpAttributeList;

    TSdpAttributeParser(TSdpAttributeList& aNodeList, MSdpAttributeNotifier& aObserver):
        iObserver(aObserver), iNodeList(aNodeList), iCurrentNodeIndex(0) {};
    TBool HasFinished() const { return CurrentNode().iCommand == EFinished; };

 public: // from MSdpAttributeValueVisitor
    void VisitAttributeValueL(CSdpAttrValue& aValue, TSdpElementType aType);
    void StartListL(CSdpAttrValueList& /*aList*/) {};
    void EndListL();

 private:
    void CheckTypeL(TSdpElementType aElementType) const;
    void CheckValueL(CSdpAttrValue& aValue) const;
    void ReadValueL(CSdpAttrValue& aValue) const 
        { iObserver.FoundElementL(CurrentNode().iValue, aValue); };
    const SSdpAttributeNode& CurrentNode() const
        { return  iNodeList[iCurrentNodeIndex]; };
    void AdvanceL();

 private:
    MSdpAttributeNotifier& iObserver;
    TSdpAttributeList& iNodeList;
    TInt iCurrentNodeIndex;
};

const TSdpAttributeParser::SSdpAttributeNode gRfcommProtocolListData[] = 
{
        { TSdpAttributeParser::ECheckType, ETypeDES },
           { TSdpAttributeParser::ECheckType, ETypeDES },
               { TSdpAttributeParser::ECheckValue, ETypeUUID, KL2CAP },
           { TSdpAttributeParser::ECheckEnd },
           { TSdpAttributeParser::ECheckType, ETypeDES },
               { TSdpAttributeParser::ECheckValue, ETypeUUID, KRFCOMM },
               { TSdpAttributeParser::EReadValue, ETypeUint, KRfcommChannel },
           { TSdpAttributeParser::ECheckEnd },
        { TSdpAttributeParser::ECheckEnd },
        { TSdpAttributeParser::EFinished }
};

const TSdpAttributeParser::SSdpAttributeNode gObexProtocolListData[] = 
{
        { TSdpAttributeParser::ECheckType, ETypeDES },
            { TSdpAttributeParser::ECheckType, ETypeDES },
                { TSdpAttributeParser::ECheckValue, ETypeUUID, KL2CAP },
            { TSdpAttributeParser::ECheckEnd },
            { TSdpAttributeParser::ECheckType, ETypeDES },
                { TSdpAttributeParser::ECheckValue, ETypeUUID, KRFCOMM },
                { TSdpAttributeParser::EReadValue, ETypeUint, KRfcommChannel },
            { TSdpAttributeParser::ECheckEnd }, 
            { TSdpAttributeParser::ECheckType, ETypeDES },
                { TSdpAttributeParser::ECheckValue, ETypeUUID, KBtProtocolIdOBEX },
            { TSdpAttributeParser::ECheckEnd },
        { TSdpAttributeParser::ECheckEnd },
        { TSdpAttributeParser::EFinished }
};

const TStaticArrayC<TSdpAttributeParser::SSdpAttributeNode> gRfcommProtocolList =
CONSTRUCT_STATIC_ARRAY_C(
    gRfcommProtocolListData
);

const TStaticArrayC<TSdpAttributeParser::SSdpAttributeNode> gObexProtocolList =
CONSTRUCT_STATIC_ARRAY_C(
    gObexProtocolListData
);

void TSdpAttributeParser::VisitAttributeValueL(CSdpAttrValue& aValue, TSdpElementType aType)
{
    switch(CurrentNode().iCommand)
        {
        case ECheckType:
            CheckTypeL(aType);
            break;
        case ECheckValue:
            CheckTypeL(aType);
            CheckValueL(aValue);
            break;
        case EReadValue:         
            CheckTypeL(aType);   
            ReadValueL(aValue);          
            break;
        case ECheckEnd:
        case EFinished:
            User::Leave(KErrGeneral);   
            break;
        case ESkip:
            break; 
        default:
            SPyErr_SetFromSymbianOSErr(ESdpAttributeParserInvalidCommand);
        }
    AdvanceL();
}

void TSdpAttributeParser::EndListL()
{
    if (CurrentNode().iCommand != ECheckEnd)
          User::Leave(KErrGeneral);
    AdvanceL();
}

void TSdpAttributeParser::CheckTypeL(TSdpElementType aElementType) const
{
    if (CurrentNode().iType != aElementType)
          User::Leave(KErrGeneral);   
}

void TSdpAttributeParser::CheckValueL(CSdpAttrValue& aValue) const
{
    switch(aValue.Type())
        {
        case ETypeNil:
            SPyErr_SetFromSymbianOSErr(ESdpAttributeParserNoValue);
            break;
        case ETypeUint:
            if (aValue.Uint() != (TUint)CurrentNode().iValue)
                User::Leave(KErrArgument);
            break;
        case ETypeInt:
            if (aValue.Int() != CurrentNode().iValue)
                User::Leave(KErrArgument);
            break;
        case ETypeBoolean:
            if (aValue.Bool() != CurrentNode().iValue)
                User::Leave(KErrArgument);
            break;
        case ETypeUUID:
            if (aValue.UUID() != TUUID(CurrentNode().iValue))
                User::Leave(KErrArgument);
            break;

        case ETypeDES:
        case ETypeDEA:
            SPyErr_SetFromSymbianOSErr(ESdpAttributeParserValueIsList);
            break;
        default:
            SPyErr_SetFromSymbianOSErr(ESdpAttributeParserValueTypeUnsupported);
            break;
        }
}

void TSdpAttributeParser::AdvanceL()
{
    if (CurrentNode().iCommand == EFinished)
          User::Leave(KErrEof);

    ++iCurrentNodeIndex;
}

/* Dictionary to store remotely advertised services and relative port numbers */

struct DictElement {
   TBuf8<MaxServiceNameSize> key;
   int value;
};

class Dictionary: CBase {
public:
  static Dictionary* NewL();
  int GetValue() { return item.value; };
  TBuf8<MaxServiceNameSize>& GetKey() { return item.key; };
  static const TInt iOffset; 
  DictElement item;

private:
  Dictionary(){};
  TSglQueLink* iLink;
};

Dictionary* Dictionary::NewL() {
  Dictionary* self = new (ELeave) Dictionary();
  self->item.key.FillZ(MaxServiceNameSize);
  self->item.value = 0;
  return self;
}

/* It sets the offset of the link object from the start of a list element */
const TInt Dictionary::iOffset = _FOFF(Dictionary,iLink);

#ifndef EKA2
class CStack: public CBase {
#else
NONSHARABLE_CLASS(CStack): public CBase {
#endif
public:
  static CStack* NewL();
  ~CStack();
  TSglQue<Dictionary>     iStack;
  TSglQueIter<Dictionary> iStackIter;
private:
  CStack();
};

CStack* CStack::NewL() {
  CStack* self = new (ELeave) CStack;
  return self;
}

CStack::CStack()
  :iStack(Dictionary::iOffset), iStackIter(iStack)
{}

CStack::~CStack(){
  Dictionary* d;
  iStackIter.SetToFirst();
  while ( (d = iStackIter++) != NULL) {
    iStack.Remove(*d);
    delete d;
  };
}

/*
 * implementation of virtual classes needed by CSdpAgent
 */

#ifndef EKA2 
class BTSearcher: public CBase,
                  public MSdpAgentNotifier,
                  public MSdpAttributeNotifier
#else
NONSHARABLE_CLASS(BTSearcher): public CBase,
                  public MSdpAgentNotifier,
                  public MSdpAttributeNotifier
#endif
{
public:
  BTSearcher(int); 
  ~BTSearcher();
        
  void SelectDeviceByDiscoveryL(TRequestStatus& aObserverRequestStatus);
  void FindServiceL(TRequestStatus&);
  void FindServiceL(TRequestStatus&, const TBTDevAddr&);
  
  void AttributeRequestComplete(TSdpServRecordHandle aHandle, TInt aError);
  void AttributeRequestResult(TSdpServRecordHandle aHandle, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue);
  void NextRecordRequestComplete(TInt aError, TSdpServRecordHandle aHandle, TInt aTotalRecordsCount);

  TBool ReadNotStatus() { return iIsNotifierConnected; };
  void  WriteNotStatus(TBool aStatus) { iIsNotifierConnected = aStatus; };
  TInt Port() { return iPort; };

  CStack* SDPDict;
  
protected:
  virtual const TSdpAttributeParser::TSdpAttributeList& ProtocolList() 
        { return aServiceType == RfcommService ? gRfcommProtocolList : gObexProtocolList; };
  virtual void FoundElementL(TInt, CSdpAttrValue&);
  virtual const TUUID& ServiceClass() const { return iServiceClass; };  
  virtual TBool HasFinishedSearching() const { return EFalse; };
  void Finished(TInt aError = KErrNone);

 private:
    void NextRecordRequestCompleteL(TInt aError, TSdpServRecordHandle aHandle, TInt aTotalRecordsCount);
    void AttributeRequestResultL(TSdpServRecordHandle aHandle, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue);
    void AttributeRequestCompleteL(TSdpServRecordHandle, TInt aError);

 public:
    TBTDeviceResponseParamsPckg  aResponse;
    RNotifier                    notifier;
    TBTDeviceSelectionParamsPckg selectionFilter;

 private:
    CSdpAgent           *iAgent;
    CSdpSearchPattern   *iSdpSearchPattern;
    TBool               iIsNotifierConnected;
    TBool               iIsNameFound;
    TBool               iIsServiceFound;
    TRequestStatus      *iStatusObserver;
    TUUID               iServiceClass;
    TInt                iPort;
    int                 aServiceType;
    TBuf8<MaxAttributeSize> attribute;
    /* since the parsing is done first at the port and later at service names, this variables is needed to avoid
       to get in the dictionary an item with the right port number but meaningless name */
    enum TSequence { NONE, ADDNAME, ADDATTR };
    TSequence    sequence;
    CSdpAttrIdMatchList *iMatchList;
};

BTSearcher::BTSearcher(int s):
  iIsNotifierConnected(EFalse),
  iIsNameFound(EFalse), 
  iIsServiceFound(EFalse), 
  iPort(-1),
  aServiceType(s)
{
  if (aServiceType == RfcommService)
    iServiceClass = KRFCServiceClass;
  else
    iServiceClass = KObServiceClass; 
  TRAPD(error, ( SDPDict = CStack::NewL()));
  if (error != KErrNone) {
    SPyErr_SetFromSymbianOSErr(error);
    return;
    }
  attribute.FillZ(MaxAttributeSize);
  sequence = NONE;
}

BTSearcher::~BTSearcher()
{
  if (iIsNotifierConnected)
    {
      notifier.CancelNotifier(KDeviceSelectionNotifierUid);
      notifier.Close();
      WriteNotStatus(EFalse);
    }
  delete iAgent;
  iAgent = NULL;
  delete iMatchList;
  iMatchList = NULL;
  delete iSdpSearchPattern;
  iSdpSearchPattern = NULL;
  delete SDPDict;
}

void BTSearcher::SelectDeviceByDiscoveryL(TRequestStatus &aStatus)
{
  if (!ReadNotStatus())
    {
      User::LeaveIfError(notifier.Connect());
      WriteNotStatus(ETrue);
    }
  selectionFilter().SetUUID(ServiceClass());
  notifier.StartNotifierAndGetResponse(aStatus, KDeviceSelectionNotifierUid, selectionFilter, aResponse);
}

void BTSearcher::FindServiceL(TRequestStatus &aStatus)
{
  if (!aResponse().IsValidBDAddr())
    User::Leave(KErrNotFound);

  iIsServiceFound = EFalse;
  iAgent = CSdpAgent::NewL(*this, aResponse().BDAddr());

  if (iSdpSearchPattern != NULL) {
    delete iSdpSearchPattern;
    iSdpSearchPattern = NULL;
  }
  iSdpSearchPattern = CSdpSearchPattern::NewL();

  iSdpSearchPattern->AddL(ServiceClass()); 

  iAgent->SetRecordFilterL(*iSdpSearchPattern);
  
  iStatusObserver = &aStatus;
  iAgent->NextRecordRequestL();
}

void BTSearcher::FindServiceL(TRequestStatus &aStatus, const TBTDevAddr &address)
{
  iIsServiceFound = EFalse;
  iAgent = CSdpAgent::NewL(*this, address);

  if (iSdpSearchPattern != NULL) {
    delete iSdpSearchPattern;
    iSdpSearchPattern = NULL;
  } 
  iSdpSearchPattern = CSdpSearchPattern::NewL();

  iSdpSearchPattern->AddL(ServiceClass()); 

  iAgent->SetRecordFilterL(*iSdpSearchPattern);

  iStatusObserver = &aStatus;
  iAgent->NextRecordRequestL();
}

void BTSearcher::AttributeRequestComplete(TSdpServRecordHandle aHandle, TInt aError)
{
  TRAPD(error,
        (AttributeRequestCompleteL(aHandle, aError) ));
  if (error != KErrNone) {
    SPyErr_SetFromSymbianOSErr(EBTServiceSearcherAttributeRequestComplete);
    return;
  }
}

void BTSearcher::AttributeRequestCompleteL(TSdpServRecordHandle /*aHandle*/, TInt /*aError*/)
{
  if (!HasFinishedSearching())     // have not found a suitable record so request another
    iAgent->NextRecordRequestL();
  else 
    Finished();
}

void BTSearcher::AttributeRequestResult(TSdpServRecordHandle aHandle, TSdpAttributeID aAttrID, CSdpAttrValue* aAttrValue )
{
  TRAPD(error,
        (AttributeRequestResultL(aHandle, aAttrID, aAttrValue) ));
  if (error != KErrNone) {
    SPyErr_SetFromSymbianOSErr(EBTServiceSearcherAttributeRequestResult);
    return;
  }

  if (sequence == ADDNAME)   
    if (iIsNameFound && iIsServiceFound) {
      Dictionary* tmp = Dictionary::NewL();
      tmp->item.key.Copy(attribute.Ptr(), attribute.Length());
      tmp->item.value = Port();
      SDPDict->iStack.AddLast(*tmp); 
      sequence = NONE;
    }

  attribute.FillZ(MaxAttributeSize);
  delete aAttrValue;
}

void BTSearcher::AttributeRequestResultL(TSdpServRecordHandle /*aHandle*/, TSdpAttributeID aAttrID,
                                         CSdpAttrValue* aAttrValue)
{
  TSdpAttributeParser parser(ProtocolList(), *this);

  if ( aAttrID == KSdpAttrIdBasePrimaryLanguage+KSdpAttrIdOffsetServiceName ) {
    if ( aAttrValue->Type() == ETypeString)  {
      TPtrC8 tmp(aAttrValue->Des()); 
      attribute.Copy((TUint8*)(tmp.Ptr()), tmp.Length());
      if (tmp.Length()>0)
        iIsNameFound = ETrue;
      sequence = ADDNAME;
    }
  }

  if (aAttrID == KSdpAttrIdProtocolDescriptorList) {
    if (aAttrValue->Type() == ETypeUint) {
      iPort = aAttrValue->Uint();
    }

    if ((aAttrValue->Type() == ETypeDEA) || (aAttrValue->Type() == ETypeDES)) {
      aAttrValue->AcceptVisitorL(parser);
      sequence = ADDATTR;
    }
    if (parser.HasFinished())       // Found a suitable record so change state
      iIsServiceFound = ETrue;  
  }
}

void BTSearcher::NextRecordRequestComplete(TInt aError, TSdpServRecordHandle aHandle, TInt aTotalRecordsCount)
{
  TRAPD(error,
        (NextRecordRequestCompleteL(aError, aHandle, aTotalRecordsCount) ));

  if (error != KErrNone) {
    SPyErr_SetFromSymbianOSErr(EBTServiceSearcherNextRecordRequestComplete);
    return;
  }
}

void BTSearcher::NextRecordRequestCompleteL(TInt aError, TSdpServRecordHandle aHandle, TInt aTotalRecordsCount)
{
  if (aError == KErrEof) {
    Finished();
    return;
  }
  if (aError != KErrNone) {
    Finished(aError);
    return;
  }
  if (aTotalRecordsCount == 0) {
    Finished(KErrNotFound);
    PyErr_SetString(PyExc_TypeError, "NRRC No records");
    return;
  }
  iMatchList = CSdpAttrIdMatchList::NewL();
  iMatchList->AddL(TAttrRange(KSdpAttrIdProtocolDescriptorList));
  iMatchList->AddL(TAttrRange(KSdpAttrIdBasePrimaryLanguage+KSdpAttrIdOffsetServiceName));
  iAgent->AttributeRequestL(aHandle, *iMatchList);   
}

void BTSearcher::Finished(TInt aError) //KErrNone given by defualt
{
  if (aError == KErrNone && !iIsServiceFound && !iIsNameFound)
    aError = KErrNotFound;
  User::RequestComplete(iStatusObserver, aError);
}

void BTSearcher::FoundElementL(TInt /*aKey*/, CSdpAttrValue& aValue) 
{ 
  iPort = aValue.Uint(); 
}

/* client object for Obex connection */
#ifndef EKA2
class CObjectExchangeClient : public CActive
#else
NONSHARABLE_CLASS(CObjectExchangeClient) : public CActive
#endif
{
 public:
  static CObjectExchangeClient* NewL(const TInt&);      
  ~CObjectExchangeClient();
  void ConnectL();

  void Disconnect();
  void SendObjectL();
  void StopL();
  TBool IsConnected() { return iState == EWaitingToSend; };
  TBool IsBusy() { return IsActive(); };
  
  TInt DevServResearch(); 
  TInt ServiceResearch(const TBTDevAddr &);
  TInt ObexSend(TBTSockAddr &, TInt &, TPtrC &);
  void SetPort(TInt aPort) { port = aPort; };

  BTSearcher*   iBTSearcher;

 protected: 
  void DoCancel() {};
  void RunL();

 private:
  static CObjectExchangeClient* NewLC(const TInt&);     
  CObjectExchangeClient(const TInt&);   
  void ConstructL();
  void ConnectToServerL(TBTSockAddr &, TInt &); 

 private:
  enum TState { EWaitingToGetDevice, EGettingDevice, EWaitingToSend, EDisconnecting, EDone };

  TState        iState;
#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait locker;
#endif
  CObexClient*  obexClient;
  CObexFileObject* obexObject;
  TInt          port;
  int           iService; 
  TInt          error;
};

CObjectExchangeClient* CObjectExchangeClient::NewL(const TInt &whichService)    
{
  CObjectExchangeClient* self = NewLC(whichService);    
  return self;
}
    
CObjectExchangeClient* CObjectExchangeClient::NewLC(const TInt &whichService)   
{
  CObjectExchangeClient* self = new (ELeave) CObjectExchangeClient(whichService);       
  self->ConstructL();
  return self;
}

CObjectExchangeClient::CObjectExchangeClient(const TInt &whichService): 
  CActive(CActive::EPriorityStandard),
  iState(EWaitingToGetDevice),
  port(-1),
  error(KErrNone)
{
  if ( whichService == RfcommService)
     iService = RfcommService;
  else
     iService = ObexService;

  CActiveScheduler::Add(this);
}

void CObjectExchangeClient::ConstructL()
{
  iBTSearcher = new BTSearcher(iService);
}

CObjectExchangeClient::~CObjectExchangeClient()
{
  StopL();
  //  Cancel();
  Disconnect();
  delete obexObject;
  obexObject = NULL;
  delete obexClient;
  obexClient = NULL;
  delete iBTSearcher;
  iBTSearcher = NULL;
}

TInt CObjectExchangeClient::DevServResearch()
{
  TRAP(error, (ConnectL() ));
  if (error != KErrNone) 
    return error;
  Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT
  locker.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  return error;
}

TInt CObjectExchangeClient::ServiceResearch(const TBTDevAddr &aAddress)
{
  iState = EDone;
  iStatus = KRequestPending; 
  TRAP(error, (iBTSearcher->FindServiceL(iStatus, aAddress) ));
  if (error != KErrNone)
    return error;
  SetActive();
  Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT
  locker.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  return error;
}

TInt CObjectExchangeClient::ObexSend(TBTSockAddr &address, TInt &port, TPtrC &obj)
{
  TRAP(error, (obexObject = CObexFileObject::NewL(obj) ));
  if (error != KErrNone)
    return error;
  TRAP(error, (obexObject->SetDescriptionL(obj) ));
  if (error != KErrNone) 
    return error;
  TRAP(error, (obexObject->InitFromFileL(obj) ));
  if (error != KErrNone) 
    return error;
  TRAP(error, (ConnectToServerL(address, port) ));
  if (error != KErrNone) 
    return error;
  Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT
  locker.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  
  TRAP(error, (SendObjectL() ));
  if (error != KErrNone) 
    return error;
  Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT
  locker.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  return error;
}

void CObjectExchangeClient::RunL()
{
  if (iStatus != KErrNone) {
    iState = EDone;
    error = iStatus.Int();
#ifdef HAVE_ACTIVESCHEDULERWAIT
    locker.AsyncStop();
#else
    CActiveScheduler::Stop();
#endif
  }
  else {
    switch (iState) {
    case EGettingDevice:
      iState = EDone;
      iStatus = KRequestPending; // this means that the RunL can not be called until
                                 // this program does something to iStatus
      TRAP(error, (iBTSearcher->FindServiceL(iStatus) ));
      if (error != KErrNone) return;
      SetActive();
      break;
    case EDisconnecting:
      iState = EWaitingToGetDevice;
#ifdef HAVE_ACTIVESCHEDULERWAIT
      locker.AsyncStop();
#else
      CActiveScheduler::Stop();
#endif
      break;
    case EDone:
#ifdef HAVE_ACTIVESCHEDULERWAIT
      locker.AsyncStop();
#else
      CActiveScheduler::Stop();
#endif
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "BT Object Exchange Sdp Record Delete");
      break;
    };
  }
}

void CObjectExchangeClient::ConnectL()
{
  if (iState == EWaitingToGetDevice && !IsActive()) {
    iBTSearcher->SelectDeviceByDiscoveryL(iStatus);
    iState = EGettingDevice;
    SetActive();
  }
  else {
    SPyErr_SetFromSymbianOSErr(KErrInUse);
    return;
  }
  
  iBTSearcher->WriteNotStatus(ETrue);
}

void CObjectExchangeClient::ConnectToServerL(TBTSockAddr &BTSAddress, TInt &port)
{
  TObexBluetoothProtocolInfo protocolInfo;

  protocolInfo.iTransport.Copy(KServerTransportName);

  protocolInfo.iAddr.SetBTAddr(BTSAddress.BTAddr());
  protocolInfo.iAddr.SetPort(port);

  if (obexClient) {
    delete obexClient;
    obexClient = NULL;
  }
  obexClient = CObexClient::NewL(protocolInfo);
  
  obexClient->Connect(iStatus);
  SetActive();
  iState = EDone;
}

void CObjectExchangeClient::SendObjectL()
{
  if (IsActive()) 
    User::Leave(KErrInUse);
  
  obexClient->Put(*obexObject, iStatus);
  SetActive();
  iState = EDone;
}

void CObjectExchangeClient::StopL()
{
  if (obexClient && obexClient->IsConnected()) {
    obexClient->Abort();
    iState = EDone;
  }
  iBTSearcher->WriteNotStatus(EFalse);
}

void CObjectExchangeClient::Disconnect()
{
  if (iState == EGettingDevice) {
    SPyErr_SetFromSymbianOSErr(KErrInUse);
    return;
  }

  if (IsConnected()) 
    if ((iState == EWaitingToSend) || (iState == EDone)) {
      iState = EDisconnecting;
      obexClient->Disconnect(iStatus);
      SetActive();
      Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT
      locker.Start();
#else
      CActiveScheduler::Start();
#endif
      Py_END_ALLOW_THREADS
      iBTSearcher->WriteNotStatus(EFalse);
    }

  error = iStatus.Int();
  // KErrDisconnected is a valid behaviour: the server dropped the transport before sending an Obex response 
  if ((error != KErrNone) && (error != KErrDisconnected)) return;
} 

/* service advertiser class */
#ifndef EKA2
class CBTServiceAdvertiser : public CBase
#else
NONSHARABLE_CLASS(CBTServiceAdvertiser) : public CBase
#endif
{
 public:
    ~CBTServiceAdvertiser();
    void StartAdvertisingL(TInt, TPtrC16&);
    void StopAdvertisingL();
    TBool IsAdvertising() { return iRecord != 0; };
    void UpdateAvailabilityL(TBool aIsAvailable);

 protected:
    CBTServiceAdvertiser():
      iRecord(0), iIsConnected(EFalse) {};
    virtual void BuildProtocolDescriptionL(CSdpAttrValueDES* aProtocolDescriptor, TInt aPort) = 0;
    virtual TInt ServiceClass() = 0;
    virtual const TPtrC ServiceName() = 0;
    virtual const TDesC& ServiceDescription() = 0;
        
 private:
    void ConnectL();

 private:
    RSdp                 iSdpSession;
    RSdpDatabase         iSdpDatabase;
    TSdpServRecordHandle iRecord;
    TInt                 iRecordState;
    TBool                iIsConnected;
};

CBTServiceAdvertiser::~CBTServiceAdvertiser()
{
  if (IsAdvertising()) {
    TRAPD(err, StopAdvertisingL());
    if (err != KErrNone) {
      PySocket_Err(err);
      return;
    }
  }
  iSdpDatabase.Close();
  iSdpSession.Close();
}

void CBTServiceAdvertiser::ConnectL()
{
  if (!iIsConnected) {
    User::LeaveIfError(iSdpSession.Connect());
    User::LeaveIfError(iSdpDatabase.Open(iSdpSession));
    iIsConnected = ETrue;
  }
}

void CBTServiceAdvertiser::StartAdvertisingL(TInt aPort, TPtrC16& attributeName)
{
  if (IsAdvertising())
      StopAdvertisingL();   // could be advertising on a different port

  if (! iIsConnected)
      ConnectL();
  
  iSdpDatabase.CreateServiceRecordL(ServiceClass(), iRecord);

  CSdpAttrValueDES* vProtocolDescriptor = CSdpAttrValueDES::NewDESL(NULL);     
  CleanupStack::PushL(vProtocolDescriptor);
 
  BuildProtocolDescriptionL(vProtocolDescriptor,aPort);

  iSdpDatabase.UpdateAttributeL(iRecord, KSdpAttrIdProtocolDescriptorList, *vProtocolDescriptor);

  CleanupStack::PopAndDestroy(vProtocolDescriptor);

  iSdpDatabase.UpdateAttributeL(iRecord, 
                                KSdpAttrIdBasePrimaryLanguage+KSdpAttrIdOffsetServiceName, 
                                attributeName); 
  
  iSdpDatabase.UpdateAttributeL(iRecord, 
                                KSdpAttrIdBasePrimaryLanguage+KSdpAttrIdOffsetServiceDescription, 
                                ServiceDescription());
}

void CBTServiceAdvertiser::UpdateAvailabilityL(TBool aIsAvailable)
{
  TUint state;
  if (aIsAvailable)
      state = 0xFF;  //  Fully unused
  else
      state = 0x00;  //  Fully used -> can't connect

  //  Update the availibility attribute field
  iSdpDatabase.UpdateAttributeL(iRecord, KSdpAttrIdServiceAvailability, state);
  //  Mark the record as changed - by increasing its state number (version)
  iSdpDatabase.UpdateAttributeL(iRecord, KSdpAttrIdServiceRecordState, ++iRecordState);
}

void CBTServiceAdvertiser::StopAdvertisingL()
{
  if (IsAdvertising()) {
    iSdpDatabase.DeleteRecordL(iRecord);
    iRecord = 0;
  }
}

#ifndef EKA2
class CObjectExchangeServiceAdvertiser : public CBTServiceAdvertiser
#else
NONSHARABLE_CLASS(CObjectExchangeServiceAdvertiser) : public CBTServiceAdvertiser
#endif
{
 public:
  static CObjectExchangeServiceAdvertiser* NewL(TPtrC&);
  static CObjectExchangeServiceAdvertiser* NewLC(TPtrC&);
  ~CObjectExchangeServiceAdvertiser() {};
  void SetServiceType(int);

 protected: // from CBTServiceAdvertiser
  void BuildProtocolDescriptionL(CSdpAttrValueDES* aProtocolDescriptor, TInt aPort);
  void BuildProtocolDescriptionRfcommL(CSdpAttrValueDES* aProtocolDescriptor, TInt aPort);
  void BuildProtocolDescriptionObexL(CSdpAttrValueDES* aProtocolDescriptor, TInt aPort);
  TInt ServiceClass()     
        { return aServiceType == RfcommService ? KRFCServiceClass : KObServiceClass; };
  const TPtrC ServiceName() 
        { return serviceName; };
  const TDesC& ServiceDescription() 
        { return aServiceType == RfcommService ? KRFCServiceDescription : KObServiceDescription; };
    
 private:
  int           aServiceType;
  TUUID         iServiceClass;
  TDesC&        serviceName;
  CObjectExchangeServiceAdvertiser(TPtrC &sName):
    serviceName(sName) {};
  void ConstructL() {};
};

CObjectExchangeServiceAdvertiser* CObjectExchangeServiceAdvertiser::NewL(TPtrC &aName)
{
  CObjectExchangeServiceAdvertiser* self = CObjectExchangeServiceAdvertiser::NewLC(aName);
  return self;
}

CObjectExchangeServiceAdvertiser* CObjectExchangeServiceAdvertiser::NewLC(TPtrC &aName)
{
  CObjectExchangeServiceAdvertiser* self = new (ELeave) CObjectExchangeServiceAdvertiser(aName);
  self->ConstructL();
  return self;
}

void CObjectExchangeServiceAdvertiser::SetServiceType(int st)
{
  aServiceType = st;

  if (aServiceType == RfcommService)
    iServiceClass = KRFCServiceClass;
  else
    iServiceClass = KObServiceClass;
};

void CObjectExchangeServiceAdvertiser::BuildProtocolDescriptionL(CSdpAttrValueDES* aProtocolDescriptor, TInt aPort)
{
  if (aServiceType == ObexService)
      BuildProtocolDescriptionObexL(aProtocolDescriptor, aPort);
  else if (aServiceType == RfcommService)
      BuildProtocolDescriptionRfcommL(aProtocolDescriptor, aPort);
  else 
      User::Leave(KErrArgument);
}

void CObjectExchangeServiceAdvertiser::BuildProtocolDescriptionObexL(CSdpAttrValueDES* aProtocolDescriptor, TInt aPort)
{
   TBuf8<1> channel;
   channel.Append((TChar)aPort);

   aProtocolDescriptor
   ->StartListL()   
        ->BuildDESL()
        ->StartListL()
            ->BuildUUIDL(KL2CAP)
        ->EndListL()

        ->BuildDESL()
        ->StartListL()
            ->BuildUUIDL(KRFCOMM)
            ->BuildUintL(channel)
        ->EndListL()

        ->BuildDESL()
        ->StartListL()
            ->BuildUUIDL(KBtProtocolIdOBEX)
        ->EndListL()
   ->EndListL();
}

void CObjectExchangeServiceAdvertiser::BuildProtocolDescriptionRfcommL(CSdpAttrValueDES* aProtocolDescriptor, TInt aPort)
{
   TBuf8<1> channel;
   channel.Append((TChar)aPort);

   aProtocolDescriptor
   ->StartListL() 
        ->BuildDESL()
        ->StartListL() 
            ->BuildUUIDL(KL2CAP)
        ->EndListL()

        ->BuildDESL()
        ->StartListL()
            ->BuildUUIDL(KRFCOMM)
            ->BuildUintL(channel)
        ->EndListL()
   ->EndListL();
}

/* server object for obex connection */
#ifndef EKA2
class CObjectExchangeServer: public CBase, 
                             public MObexServerNotify
#else
NONSHARABLE_CLASS(CObjectExchangeServer): public CBase, 
                             public MObexServerNotify
#endif
{
 public:
  static CObjectExchangeServer* NewL();
  ~CObjectExchangeServer();
  int StartSrv();
  void DisconnectL();
  TBool IsConnected() { return iObexServer != NULL; };
  void SetPort(TInt aPort) { port = aPort; }; 

  static void SetSecurityOnChannelL(TBool aAuthentication, TBool aEncryption, TBool aAuthorisation, TInt aPort);
  TBool IsPortSet() {   return (port != -1) ? ETrue: EFalse; };

  TBuf<50> fileName;

 private:
  CObjectExchangeServer():
    port(-1), error(KErrNone) {};
  void ConstructL();
  void UpdateAvailabilityL(TBool aIsAvailable);
  void InitialiseServerL();
  static TInt AvailableServerChannelL();

 private:
 // from MObexServerNotify (these methods are defined as private in MObexServerNotify)
  void ErrorIndication(TInt /*aError*/) {};
  void TransportUpIndication() {};   
  void TransportDownIndication() {};
  TInt ObexConnectIndication(const TObexConnectInfo& /*aRemoteInfo*/, const TDesC8& /*aInfo*/) 
        { return KErrNone; };
  void ObexDisconnectIndication(const TDesC8& /*aInfo*/) {};
  CObexBufObject* PutRequestIndication() { return iObexBufObject; };
  TInt PutPacketIndication() { return KErrNone; };
  TInt PutCompleteIndication();
  CObexBufObject* GetRequestIndication(CObexBaseObject* /*aRequestedObject*/) { return NULL; };
  TInt GetPacketIndication() { return KErrNone; };
  TInt GetCompleteIndication() { return KErrNone; };
  TInt SetPathIndication(const CObex::TSetPathInfo& /*aPathInfo*/, const TDesC8& /*aInfo*/)
        { return KErrNone; };
  void AbortIndication() {};

 private:
  CObexServer*     iObexServer;         /* manages the OBEX client connection */
  CBufFlat*        iObexBufData;        /* the raw data that has been transferred */
  CObexBufObject*  iObexBufObject;      /* the OBEX object that has been transferred*/

#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait locker;
#endif
  TInt          port;
  TInt          error;
};
  
CObjectExchangeServer* CObjectExchangeServer::NewL()
{
  CObjectExchangeServer* self = new (ELeave) CObjectExchangeServer();
  self->ConstructL();
  return self;
}

void CObjectExchangeServer::ConstructL()
{
  iObexBufData   = CBufFlat::NewL(MaxBufferSize);
  iObexBufObject = CObexBufObject::NewL(iObexBufData);
}

CObjectExchangeServer::~CObjectExchangeServer()
{
  if (iObexServer && iObexServer->IsStarted())
    iObexServer->Stop();
  delete iObexServer;
  iObexServer = NULL; 
  DisconnectL();
  delete iObexBufData;
  iObexBufData = NULL;
  delete iObexBufObject;
  iObexBufObject = NULL;
}

TInt CObjectExchangeServer::PutCompleteIndication()
{
  TPtrC filePath(fileName);
  error = iObexBufObject->WriteToFile(filePath);
#ifdef HAVE_ACTIVESCHEDULERWAIT
  locker.AsyncStop();
#else
  CActiveScheduler::Stop();
#endif
  return error;
} 

void CObjectExchangeServer::DisconnectL()
{
  if (iObexServer && iObexServer->IsStarted())
    iObexServer->Stop();
 
  delete iObexServer;
  iObexServer = NULL;
}

int CObjectExchangeServer::StartSrv()
{
  TRAPD(err, InitialiseServerL());
  Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT
  locker.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  if (err != KErrNone)
    DisconnectL();
  return err;
}

void CObjectExchangeServer::InitialiseServerL()
{
  RSocketServ socketServer;
  RSocket socket;
  TBool newServerSession = EFalse;

  if (iObexServer) { 
    ASSERT(IsConnected()); // server already running
    return;
  }

  TInt channel;

  if (!IsPortSet()) 
    channel = AvailableServerChannelL();
  else {
    User::LeaveIfError(socketServer.Connect());
    newServerSession = ETrue;
    User::LeaveIfError(socket.Open(socketServer, KServerTransportName));
    channel = port;
  }
        
  SetSecurityOnChannelL(EFalse, EFalse, ETrue, channel);

  TObexBluetoothProtocolInfo obexProtocolInfo;
  obexProtocolInfo.iTransport.Copy(KServerTransportName);
  obexProtocolInfo.iAddr.SetPort(channel);

  iObexServer = CObexServer::NewL(obexProtocolInfo);
  iObexServer->Start(this);
    
  if (newServerSession) {
    socket.Close();
    socketServer.Close();
  }
}

TInt CObjectExchangeServer::AvailableServerChannelL() 
{
  RSocketServ socketServer;
  RSocket socket;
  User::LeaveIfError(socketServer.Connect());
  
  User::LeaveIfError(socket.Open(socketServer, KServerTransportName));

  TInt channel;
  User::LeaveIfError(   socket.GetOpt(KRFCOMMGetAvailableServerChannel, KSolBtRFCOMM, channel));

  socket.Close();
  socketServer.Close();
  return channel;
}

void CObjectExchangeServer::SetSecurityOnChannelL(TBool aAuthentication, TBool aEncryption, 
                                                  TBool aAuthorisation, TInt aChannel)
{
#ifndef EKA2
      RBTMan secManager;
#if SERIES60_VERSION>=26
    RBTSecuritySettingsB secSettingsSession;
#else
    RBTSecuritySettings secSettingsSession;
#endif

    User::LeaveIfError(secManager.Connect());
    User::LeaveIfError(secSettingsSession.Open(secManager));
    /* NOTE: original implementation uses like first paramether the App UId.
       It must just be an unique identifier for the service  */
    const TUid KUidBTObjectExchangeApp = {0x10105b8e};
#if SERIES60_VERSION>=26
    TBTServiceSecurityB serviceSecurity(KUidBTObjectExchangeApp, KSolBtRFCOMM, 0);
#else
    TBTServiceSecurity serviceSecurity(KUidBTObjectExchangeApp, KSolBtRFCOMM, 0);
#endif

    serviceSecurity.SetAuthentication(aAuthentication);
    serviceSecurity.SetEncryption(aEncryption); 
    serviceSecurity.SetAuthorisation(aAuthorisation);
    serviceSecurity.SetChannelID(aChannel);
    TRequestStatus status;
    secSettingsSession.RegisterService(serviceSecurity, status);

    User::WaitForRequest(status); 
    User::LeaveIfError(status.Int());
    
#endif /*EKA2*/
}

/*
 *
 * Implementation of Bluetooth API's for Python 
 *
 */

#define BTDevice_type ((PyTypeObject*)SPyGetGlobalString("BTDeviceType"))

struct BTDevice_data {
  CObjectExchangeServer *aBTObjExSrv;
  CObjectExchangeClient *aBTObjExCl;
  TBTDevAddr  address;
  TInt        port;
  TInt        error;
};

/*
 *
 * Implementation of e32socket.Socket object and methods
 *
 */


#define AP_type ((PyTypeObject*)SPyGetGlobalString("APType"))

struct AP_object {
  PyObject_VAR_HEAD
  RConnection* connection;
  TInt apId;
  TBool started;
};

class TStaticData {
public:
  RSocketServ rss;
  AP_object *apo;
};

extern "C"
void socket_mod_cleanup();

extern "C" PyObject *
ap_start(AP_object *self, PyObject* /*args*/);

extern "C" PyObject *
ap_stop(AP_object *self, PyObject* /*args*/);

TStaticData* GetServer()
{
  if (Dll::Tls())
  {
      return static_cast<TStaticData*>(Dll::Tls());
  }
  else 
  {
      TInt error = KErrNone;
      TStaticData* sd = NULL;
      TRAP(error, {
        sd = new (ELeave) TStaticData;
      });
      if(error!=KErrNone){
        return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }
      
      // Nullify the content.
      sd->apo=NULL;
       
      error = sd->rss.Connect(KESockDefaultMessageSlots*2); 
      if (error != KErrNone){
        delete sd;
        return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }
      error = Dll::SetTls(sd);
      if(error!=KErrNone){
        sd->rss.Close();
        delete sd;
        return (TStaticData*) SPyErr_SetFromSymbianOSErr(error);
      }    
      
      PyThread_AtExit(socket_mod_cleanup);
      return static_cast<TStaticData*>(Dll::Tls()); 
  }   
}

/*
 * CSocketEngine
 */

class CSocketEngine;

#ifndef EKA2
class CSocketAo : public CActive
#else
NONSHARABLE_CLASS(CSocketAo) : public CActive
#endif
{
public:
  CSocketAo():CActive(EPriorityStandard),iState(EIdle),iDataBuf(0,0) {
    CActiveScheduler::Add(this);
  }

  PyObject* HandleReturn(PyObject* aSo) {
    Py_XINCREF(aSo);
    iState = EBusy;
    SetActive();
    if (iPyCallback) {
      iSo = aSo;
      Py_INCREF(iPyCallback);
      Py_INCREF(Py_None);
      return Py_None;
    }
    Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_ACTIVESCHEDULERWAIT
    iWait.Start();
#else
    CActiveScheduler::Start();
#endif
    Py_END_ALLOW_THREADS
    PyObject* r = iCallback(iStatus.Int(), this);
    Py_XDECREF(aSo);
    return r;
  }

  TAny* (iData)[3];
  PyObject* iPyCallback;
  PyObject* (*iCallback)(TInt, CSocketAo*);
  enum {EIdle, EBusy} iState;
  TPtr8 iDataBuf;
  TSockXfrLength iXLen;
  TInetAddr iInetAddr; 
  TBTSockAddr iBtAddr;

private:
  void RunL();
  void DoCancel() {;}
#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
  PyObject* iSo;
};

void CSocketAo::RunL()
{
  iState = EIdle;
  iXLen = iDataBuf.Length();
  if (!iPyCallback) {
#ifdef HAVE_ACTIVESCHEDULERWAIT
    iWait.AsyncStop();
#else
    CActiveScheduler::Stop();
#endif
  }
  else {
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject* so = iSo;
    PyObject* r = iCallback(iStatus.Int(), this);
    PyObject* arg;
    if (r)
      arg = Py_BuildValue("(N)", r);
    else {
      PyObject *t, *v, *tb;
      PyErr_Fetch(&t, &v, &tb);
      PyErr_NormalizeException(&t, &v, &tb);
      arg = Py_BuildValue("((OO))", t, v);
    }
    PyObject* cb = iPyCallback;
    iPyCallback = NULL;
    PyObject* tmp_r = PyEval_CallObject(cb, arg);
    Py_DECREF(cb);
    Py_XDECREF(tmp_r);
    Py_XDECREF(arg);
    Py_XDECREF(so);
    PyGILState_Release(gstate);
  }
}

class CSocketEngine : public CBase
{
  friend class SSLEngine;
 public:
  CSocketEngine():iPort(-1),iAdvertiser(NULL) {;}

  ~CSocketEngine() {
    // At this point these invariants should hold:
    //   iReadAo.iState == EIdle
    //   iReadAo.iPyCallback == NULL
    //   iWriteAo.iState == EIdle
    //   iWriteAo.iPyCallback == NULL
    iSocket.Close();
    if (iAdvertiser) {
      iAdvertiser->StopAdvertisingL();
      delete iAdvertiser;
    }
  }

  TUint GetProtoInfo() {
    TProtocolDesc protocol;
    TInt error = iSocket.Info(protocol);
    if (error != KErrNone)
      return error;
    return protocol.iProtocol;
  }

  TInt          iPort;
  RSocket       iSocket;
  CSocketAo     iReadAo;
  CSocketAo     iWriteAo;
  CObjectExchangeServiceAdvertiser* iAdvertiser;
};

/* Python APIs */

#define Socket_type ((PyTypeObject*)SPyGetGlobalString("SocketType"))

/*
 *  No class members here as they do not get
 *  properly initialized by PyObject_New() !!!
 */
struct Socket_object {
  PyObject_VAR_HEAD
  CSocketEngine* SE;
  int ob_is_closed;
  int ob_is_connected;
  unsigned int ob_proto;
};

extern "C" {

  static void Socket_dealloc(Socket_object *so)
  {
    delete so->SE;
    so->SE = NULL;
    PyObject_Del(so);
  }

  void socket_mod_cleanup()
  {
    TStaticData* sd = GetServer();
    
    if(sd!=NULL){
      if(sd->apo!=NULL){
        sd->apo->started = EFalse;
        ap_stop(sd->apo,NULL);
        Py_DECREF(sd->apo);
        sd->apo = NULL;  
      }
      sd->rss.Close();
      delete static_cast<TStaticData *>(Dll::Tls());
      Dll::SetTls(NULL);
    }
  }
}

static void str2Hex(const TDesC &aBTHexAddr, TBuf8<6> &aBTAddress) 
{
  /* convert string into Hex address */
  for (TInt i=0; i<6; i++)
    {
      TInt j =0;
      if (i!=0)
        j = i +(i*2);
      TLex hexValue(aBTHexAddr.Mid(j, 2));
      TUint8 n;
      hexValue.Val(n, EHex);
      aBTAddress.Append(TChar(n));
    }   
  return;
}

static TInt set_symbian_inet_addr(TInetAddr &aInetAddr, char* str, int length, int port)
{
  TBuf<MaxIPAddrLength> address;
  address.FillZ(MaxIPAddrLength);
  address.Copy((TPtrC8((TUint8*)str, length)));
  
  if (port)
    aInetAddr.SetPort(port);
  return aInetAddr.Input(address);
}

static void set_symbian_bt_addr(TBTSockAddr &address, char* str, int length, int port)
{
  TBuf<MaxIPAddrLength+1> aBTHexAddr;
  aBTHexAddr.FillZ(MaxIPAddrLength+1);
  aBTHexAddr.Copy((TPtrC8 ((TUint8*)str, length)));
  TBuf8<6> aBTAddress;
  str2Hex(aBTHexAddr, aBTAddress);
  address.SetBTAddr(aBTAddress);
  if (port != 0)
    address.SetPort(port);
}

static PyObject* create_socket_object()
{
  Socket_object *so = PyObject_New(Socket_object, Socket_type);
  if (so == NULL) 
    return PyErr_NoMemory();
    
  so->SE = new CSocketEngine();
  if (so->SE == NULL) {
    PyObject_Del(so);
    return PyErr_NoMemory();
  }
  
  so->ob_is_closed = 0;
  so->ob_is_connected = 0;
  return (PyObject*) so;
}

static int KErrToErrno(TInt aError)
{
  int i;
  // FIXME Replace unreadable magic numbers with symbolic error codes.
  switch(aError) {
  case (-1):
    i = ENOENT;
    break;
  case (-3):
    i = EINTR;
    break;
  case (-4):
    i = ENOMEM;
    break;
  case (-5):
    i = ENOSYS;
    break;
  case (-6):
  case (-28):
    i = EINVAL;
    break;
  case (-9):
  case (-10):
    i = ERANGE;
    break;
  case (-11):
    i = EEXIST;
    break;
  case (-12):
    i = ENOENT;
    break;
  case (-13):
  case (-15):
  case (-25):
  case (-36):
    i = EPIPE;
    break;
  case (-14):
    i = EACCES;
    break;
  case (-16):
    i = EBUSY;
    break;
  case (-21):
  case (-22):
    i = EACCES;
    break;
  case (-24):
    i = ENODEV;
    break;
  case (-26):
  case (-43):
    i = ENOSPC;
    break;
  case (-29):
  case (-30):
  case (-31):
  case (-32):
    i = ECOMM;
    break;
  case (-33):
    i = ETIMEDOUT;
    break;
  case (-34):
    i = ECONNREFUSED;
    break;
  case (-41):
  case (-42):
    i = EDOM;
    break;
  default:
    i = 0;
    break;
  }

  return i;
}

static PyObject* _mod_dict_get_s(char* s)
{
  PyInterpreterState *interp = PyThreadState_Get()->interp;
  PyObject* m = PyDict_GetItemString(interp->modules, "e32socket");
  return PyDict_GetItemString(PyModule_GetDict(m), s);
}

static PyObject *
PySocket_Err(TInt aError)
{
  PyObject *v;
  char *s;

  if (aError == KErrPython)
    return NULL;
  
  int i = KErrToErrno(aError);
  if (i == 0)
    s = "Error";
  else
    s = strerror(i);
  
  v = Py_BuildValue("(is)", i, s);
  if (v != NULL) {
    PyErr_SetObject(_mod_dict_get_s("error"), v);
    Py_DECREF(v);
  }
  return NULL;
}

static PyObject *
PySocket_Err(char* aMsg)
{
  PyErr_SetString(_mod_dict_get_s("error"), aMsg);
  return NULL;
}

/* for Host resolver */
static PyObject *
PyGAI_Err(TInt aError)
{
  PyObject *v;
  int error = KErrToErrno(aError);

  v = Py_BuildValue("(is)", error, "getaddrinfo failed");
  if (v != NULL) {
    PyErr_SetObject(_mod_dict_get_s("gaierror"), v);
    Py_DECREF(v);
  }
  return NULL;
}

#define RETURN_SOCKET_ERROR_OR_PYNONE(error) \
if (error != KErrNone)\
  return PySocket_Err(error);\
else {\
  Py_INCREF(Py_None);\
  return Py_None;\
}

#define E32SOCKET_CHECK
#ifdef E32SOCKET_CHECK
#define CHECK_NOTCLOSED(op) \
if (op->ob_is_closed)\
  return PySocket_Err("Attempt to use a closed socket")
#define CHECK_NOTCLOSED_NOTREADBUSY(op) \
if (op->ob_is_closed)\
  return PySocket_Err("Attempt to use a closed socket");\
if (op->SE->iReadAo.iState == CSocketAo::EBusy)\
  return PySocket_Err(KErrInUse)
#define CHECK_NOTCLOSED_NOTWRITEBUSY(op) \
if (op->ob_is_closed)\
  return PySocket_Err("Attempt to use a closed socket");\
if (op->SE->iWriteAo.iState == CSocketAo::EBusy)\
  return PySocket_Err(KErrInUse)
#define VERIFY_PROTO(c) \
if (!(c)) return PySocket_Err("Bad protocol")
#else
#define CHECK_NOTCLOSED(op)
#define CHECK_NOTCLOSED_NOTREADBUSY(op)
#define CHECK_NOTCLOSED_NOTWRITEBUSY(op)
#define VERIFY_PROTO(c)
#endif /* E32SOCKET_CHECK */

extern "C" PyObject *
new_Socket_object(PyObject* /*self*/, PyObject *args)
{  
  TUint family;
  TUint type;
  TInt protocol = -1;
  AP_object *apo = NULL;
  TStaticData* sd = GetServer();
  if(sd==NULL){
    return NULL;
  } 
  if (!PyArg_ParseTuple(args, "iiiO", &family, &type, &protocol, &apo)) {
      return NULL;
  }
  if (family == KAfInet) {
    if (type == KSockStream) {
      if (protocol && (protocol != (TInt)KProtocolInetTcp))
        return PySocket_Err("Bad protocol");
      else 
        protocol = KProtocolInetTcp;
    }
    else if (type == KSockDatagram) {
      if (protocol && (protocol != (TInt)KProtocolInetUdp))
        return PySocket_Err("Bad protocol");
      else 
        protocol = KProtocolInetUdp;
    }
    else 
      return PySocket_Err("Bad socket type");
  }
  else if (family == KAfBT) {
    if (type == KSockStream) {
      if ((protocol == (TInt)KRFCOMM) || (protocol == 0))
        protocol = KRFCOMM;
      else 
        return PySocket_Err("Bad protocol");
    }
    else 
      return PySocket_Err("Bad socket type");
  }
  else
    return PySocket_Err("Unknown address family");
  
  Socket_object* socket = (Socket_object*) create_socket_object();
  if (socket == NULL)
    return NULL;

  socket->ob_proto = protocol;

  TInt error = KErrNone;
  if (family == KAfInet)
  {

    if (apo != NULL && (PyObject*)apo != Py_None)
    {
      if (!ap_start(apo,NULL)) {
        Socket_dealloc(socket);
        PyErr_SetString(PyExc_RuntimeError, "Couldn't start RConnection");
        return NULL; 
      }
      else {
        error = socket->SE->iSocket.Open(sd->rss, family, type, protocol, *apo->connection);
      } 
    }

    else if (sd->apo != NULL && (PyObject*)sd->apo!=Py_None)
    {
  
      if (!ap_start(sd->apo,NULL)) {
        Socket_dealloc(socket);
        PyErr_SetString(PyExc_RuntimeError, "Couldn't start RConnection");
        return NULL; 
      }
      else {
        error = socket->SE->iSocket.Open(sd->rss, family, type, protocol, *sd->apo->connection);
      } 
    }
    else {
      error = socket->SE->iSocket.Open(sd->rss, family, type, protocol);
    }
  }
  
  else if (family == KAfBT) {
    error = socket->SE->iSocket.Open(sd->rss, KServerTransportName);
  }
  if (error != KErrNone) {
    if (apo != NULL && (PyObject*)apo != Py_None) {
      ap_stop(apo, NULL);
      delete apo;
    }
    Socket_dealloc(socket);
    return PySocket_Err(error);
  }
  return (PyObject*) socket;
}

extern "C" PyObject *
socket_accept_return(TInt aError, CSocketAo* aOp);

extern "C" PyObject *
socket_accept(Socket_object* self, PyObject *args)
{
  CHECK_NOTCLOSED_NOTREADBUSY(self);
  VERIFY_PROTO((self->ob_proto == KRFCOMM) ||
	       (self->ob_proto == KProtocolInetTcp));
  
  PyObject* c = NULL;
  TStaticData* sd = NULL;

  if (!PyArg_ParseTuple(args, "|O", &c))
    return NULL;

  if (c == Py_None)
    c = NULL;
  else if (c && !PyCallable_Check(c)) {
    PyErr_BadArgument();
    return NULL;
  }

  CSocketEngine* new_SE = new CSocketEngine();
  if (!new_SE)
    return PyErr_NoMemory();

  sd = GetServer();
  if(sd==NULL){
    return NULL;
  }

  TInt error = new_SE->iSocket.Open(sd->rss);
  if (error != KErrNone) {
    delete new_SE;
    return PySocket_Err(error);
  }
  self->SE->iReadAo.iData[0] = (TAny*)new_SE;
  self->SE->iReadAo.iCallback = &socket_accept_return;
  self->SE->iReadAo.iPyCallback = c;

  self->SE->iSocket.Accept(new_SE->iSocket, self->SE->iReadAo.iStatus);
  return self->SE->iReadAo.HandleReturn((PyObject*)self);
}

extern "C" PyObject *
socket_accept_return(TInt aError, CSocketAo* aOp)
{
  CSocketEngine* new_SE = (CSocketEngine*)aOp->iData[0];

  if (aError != KErrNone) {
    delete new_SE;
    return PySocket_Err(aError);
  }

  TProtocolDesc protocol;
  TInt error = new_SE->iSocket.Info(protocol);
  if (error != KErrNone) {
    delete new_SE;
    return PySocket_Err(error);
  }

  Socket_object* new_so = PyObject_New(Socket_object, Socket_type);
  if (!new_so) {
    delete new_SE;
    return PyErr_NoMemory();
  }
  new_so->ob_proto = protocol.iProtocol;
  new_so->ob_is_closed = 0;
  new_so->ob_is_connected = 1;
  new_so->SE = new_SE;

  PyObject* r;
  if (new_so->ob_proto == KProtocolInetTcp ) {
    TSockAddr remote;
    new_so->SE->iSocket.RemoteName(remote);
    char str[MaxIPAddrLength];
    PyOS_snprintf(str, MaxIPAddrLength, "%d.%d.%d.%d",
                  remote[0], remote[1], remote[2], remote[3]);
    r = Py_BuildValue("N(Ni)", new_so, PyString_FromString(str), remote.Port());
  }
  else { //BLUETOOTH
    TBTSockAddr remote;
    new_so->SE->iSocket.RemoteName(remote);
    TBuf<6> aBTA;
    aBTA.FillZ(6);
    aBTA.Copy(remote.BTAddr().Des());
    char str[MaxIPAddrLength+2];
    PyOS_snprintf(str, MaxIPAddrLength+2, "%02x:%02x:%02x:%02x:%02x:%02x",
                  aBTA[0],  aBTA[1],  aBTA[2],  aBTA[3],  aBTA[4],  aBTA[5]);
    r = Py_BuildValue("(Ns)", new_so, str);
  }
  return r;
}

extern "C" PyObject *
socket_bind(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED(self);

  TInt error = KErrNone;
  char* addr;
  int addr_len;
  int port;

  if (!PyArg_ParseTuple(args, "(s#i)", &addr, &addr_len, &port) ) {
    return NULL;
  }
  
  TSockAddr* address;
  TBTSockAddr bt_addr;
  TInetAddr inet_addr; 

  if ((self->ob_proto == KProtocolInetTcp) ||
      (self->ob_proto == KProtocolInetUdp)) { 
    error = set_symbian_inet_addr(inet_addr, addr, addr_len, port);
    address = &inet_addr;
  }
  else if (self->ob_proto == KRFCOMM) {
    bt_addr.SetPort(port);
    address = &bt_addr;
  }
  else
    return PySocket_Err("Unknown protocol");
 
  if (error == KErrNone) {
    Py_BEGIN_ALLOW_THREADS
    self->SE->iPort = address->Port();
    error = self->SE->iSocket.Bind(*address); 
    Py_END_ALLOW_THREADS
  }
  RETURN_SOCKET_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
socket_close(Socket_object *self)
{

  TStaticData* sd = GetServer();  
  self->SE->iSocket.CancelAll();
  self->SE->iSocket.Close();
  if (sd !=NULL) {
    sd->apo->started = EFalse;
    sd->apo->connection->Stop(RConnection::EStopNormal);
    sd->rss.Close();
  }
 
  self->ob_is_connected = 0;
  self->ob_is_closed = 1;
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
socket_connect_return(TInt aError, CSocketAo* aOp);

extern "C" PyObject *
socket_connect(Socket_object *self, PyObject *args)
{
  char* addr;
  int addr_len;
  int port;
  PyObject* c = NULL;
  
  CHECK_NOTCLOSED_NOTWRITEBUSY(self);
  
  if(!PyArg_ParseTuple(args, "(s#i)|O", &addr, &addr_len, &port, &c))
    return NULL;

  if (c == Py_None)
    c = NULL;
  else if (c && !PyCallable_Check(c)) {
    PyErr_BadArgument();
    return NULL;
  }

  CSocketAo* ao = &(self->SE->iWriteAo);
  TSockAddr* address;

  if ((self->ob_proto == KProtocolInetTcp) || (self->ob_proto == KProtocolInetUdp)) { 
    TInt error = set_symbian_inet_addr(ao->iInetAddr, addr, addr_len, port);
    if (error != KErrNone)
      return PySocket_Err(error);
    address = &ao->iInetAddr;
  }
  else if (self->ob_proto == KRFCOMM) {
    set_symbian_bt_addr(ao->iBtAddr, addr, addr_len, port);
    address = &ao->iBtAddr;
  }
  else
    return PySocket_Err("Bad protocol");

  ao->iData[0] = (TAny*)self;
  ao->iCallback = &socket_connect_return;
  ao->iPyCallback = c;

  self->SE->iSocket.Connect(*address, ao->iStatus);
  return ao->HandleReturn((PyObject*)self);
}

extern "C" PyObject *
socket_connect_return(TInt aError, CSocketAo* aOp)
{
  if (aError == KErrNone)
    ((Socket_object*)aOp->iData[0])->ob_is_connected = 1;
  RETURN_SOCKET_ERROR_OR_PYNONE(aError);
}

extern "C" PyObject *
socket_getsockopt(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED(self);

  int level;
  TUint optname;
  int buflen = 0;

  if (!PyArg_ParseTuple(args, "ii|i", &level, &optname, &buflen))
    return NULL;

  if ((optname!=KSoTcpOobInline) && (optname!=KSORecvBuf) &&
      (optname!=KSoReuseAddr) && (optname!=KSOSendBuf) &&
      (optname!=KSockStream) && (optname!=KSockDatagram)) 
    return PySocket_Err("Unsupported option");

  if (buflen == 0) {
    TInt option = 0;
    TInt error = self->SE->iSocket.GetOpt(optname, level, option);
    if (error != KErrNone)
      return PySocket_Err(error);
    return Py_BuildValue("i", option); 
  } else {
    PyObject *option = PyString_FromStringAndSize(NULL, buflen);
    TPtr8 rec_buf((TUint8*)(PyString_AsString(option)), 0, buflen);
    TInt error = self->SE->iSocket.GetOpt(optname, level, rec_buf);
    if (error != KErrNone)
      return PySocket_Err(error);
    if (_PyString_Resize(&option, rec_buf.Size()))
      return NULL;
    else
      return option; 
  }
}

extern "C" PyObject *
socket_listen(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED(self);
  VERIFY_PROTO((self->ob_proto != KProtocolUnknown) &&
	       (self->ob_proto != KProtocolInetUdp));
  int q_sz;
  if (!PyArg_ParseTuple(args, "i", &q_sz))
    return NULL;
  if (q_sz <= 0) 
    return PySocket_Err("Invalid queue size");

  TInt error = self->SE->iSocket.Listen(q_sz);
  RETURN_SOCKET_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
socket_not_implemented(Socket_object* /*self*/, PyObject* /*args*/)
{
  return PySocket_Err("Not Implemented feature");
}

extern "C" PyObject *
socket_recv_return(TInt aError, CSocketAo* aOp);

extern "C" PyObject *
socket_recv(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED_NOTREADBUSY(self);

  int request = -1;
  TUint flag = 0;
  PyObject* c = NULL;

  if (!PyArg_ParseTuple(args, "i|iO", &request, &flag, &c))
    return NULL;

  if (request < 0) {
    PyErr_SetString(PyExc_ValueError, "negative buffersize in recv");
    return NULL;
  }

  if (flag && 
      (flag != KWaitAll) && 
      (flag != KSockReadPeek) && 
      (flag != (KSockReadPeek|KWaitAll))) 
    return PySocket_Err("Unsupported option");

  if (c == Py_None)
    c = NULL;
  else if (c && !PyCallable_Check(c)) {
    PyErr_BadArgument();
    return NULL;
  }

  PyObject *my_str = PyString_FromStringAndSize(NULL, request);
  if (!my_str)
    return PyErr_NoMemory();
  
  CSocketAo* ao = &(self->SE->iReadAo);
  ao->iData[0] = (TAny*)my_str;
  ao->iData[1] = 0;
  ao->iCallback = &socket_recv_return;
  ao->iPyCallback = c;
  ao->iDataBuf.Set((TUint8*)(PyString_AsString(my_str)),0,request);
  
  if (self->ob_proto == KProtocolInetUdp){
    ao->iData[1] = (TAny*)1;
    self->SE->iSocket.Recv(ao->iDataBuf, (flag & KSockReadPeek), ao->iStatus);
  }
  else if (flag & KWaitAll)
    self->SE->iSocket.Recv(ao->iDataBuf, (flag & KSockReadPeek), ao->iStatus);
  else {
    ao->iData[1] = (TAny*)1;
    self->SE->iSocket.RecvOneOrMore(ao->iDataBuf, (flag & KSockReadPeek), ao->iStatus, ao->iXLen);
  }
  return ao->HandleReturn((PyObject*)self);
}

extern "C" PyObject *
socket_recv_return(TInt aError, CSocketAo* aOp)
{
  PyObject* my_str = (PyObject*)aOp->iData[0];

  if (aError != KErrNone && aError != KErrEof) {
    Py_DECREF(my_str);
    return PySocket_Err(aError);
  }

  if (aOp->iData[1] && (aOp->iXLen() != PyString_Size(my_str)))
    if (_PyString_Resize(&my_str, aOp->iXLen()) == -1)
      return NULL;
  
  return my_str;
}

extern "C" PyObject *
socket_recvfrom_return(TInt aError, CSocketAo* aOp);

extern "C" PyObject *
socket_recvfrom(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED_NOTREADBUSY(self);
  VERIFY_PROTO(self->ob_proto == KProtocolInetUdp);

  int request;
  TUint flag = 0;
  PyObject* c = NULL;

  if (!PyArg_ParseTuple(args, "i|iO", &request, &flag, &c))
    return NULL;
  
  if (flag && (flag != KWaitAll) && (flag != KSockReadPeek)) 
    return PySocket_Err("Unsupported option");

  if (c == Py_None)
    c = NULL;
  else if (c && !PyCallable_Check(c)) {
    PyErr_BadArgument();
    return NULL;
  }

  PyObject *my_str = PyString_FromStringAndSize(NULL, request);
  if (!my_str)
    return PyErr_NoMemory();
  
  TSockAddr* address = new TSockAddr();
  if (!address) {
    Py_DECREF(my_str);
    return PyErr_NoMemory();
  }

  CSocketAo* ao = &(self->SE->iReadAo);
  ao->iData[0] = (TAny*)my_str;
  ao->iData[1] = address;
  ao->iData[2] = self->SE;
  ao->iCallback = &socket_recvfrom_return;
  ao->iPyCallback = c;
  ao->iDataBuf.Set((TUint8*)(PyString_AsString(my_str)),0,request);
  
  self->SE->iSocket.RecvFrom(ao->iDataBuf, *address, flag, ao->iStatus);
  return ao->HandleReturn((PyObject*)self);
}

extern "C" PyObject *
socket_recvfrom_return(TInt aError, CSocketAo* aOp)
{
  PyObject* my_str = (PyObject*)aOp->iData[0];
  TSockAddr* address = (TSockAddr*)aOp->iData[1];
  CSocketEngine* SE = (CSocketEngine*)aOp->iData[2];

  PyObject* r;
  if (aError != KErrNone && aError != KErrEof) {
    Py_DECREF(my_str);
    r = PySocket_Err(aError);
  }
  else {
    if (_PyString_Resize(&my_str, aOp->iDataBuf.Size()))
      r = NULL;
    else {
      SE->iSocket.RemoteName(*address);
      char str[MaxIPAddrLength];
      TUint ipv4=TInetAddr(*address).Address();
      PyOS_snprintf(str, MaxIPAddrLength, "%d.%d.%d.%d",
		    ipv4>>24&0xff, ipv4>>16&0xff, ipv4>>8&0xff, ipv4&0xff);
      r = Py_BuildValue("N(Ni)", my_str,
                        PyString_FromString(str), address->Port());
    }
  }
  delete address;
  return r;
}

extern "C" PyObject *
socket_write_return(TInt aError, CSocketAo* aOp);

extern "C" PyObject *
socket_write(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED_NOTWRITEBUSY(self);

  char* data;
  int data_len;
  int flag = 0;
  PyObject* c = NULL;

  if (!PyArg_ParseTuple(args, "s#|iO", &data, &data_len, &flag, &c))
    return NULL;
  
  if (c == Py_None)
    c = NULL;
  else if (c && !PyCallable_Check(c)) {
    PyErr_BadArgument();
    return NULL;
  }

  CSocketAo* ao = &(self->SE->iWriteAo);
  ao->iData[0] = (TAny*)data_len;
  ao->iCallback = &socket_write_return;
  ao->iPyCallback = c;
  ao->iDataBuf.Set((TUint8*)data, data_len, data_len);
  
  self->SE->iSocket.Send(ao->iDataBuf, flag, ao->iStatus);
  return ao->HandleReturn((PyObject*)self);
}

extern "C" PyObject *
socket_write_return(TInt aError, CSocketAo* aOp)
{
  if (aError != KErrNone)
    return PySocket_Err(aError);

  int data_len = (int)aOp->iData[0];
  return Py_BuildValue("i", data_len); 
}

extern "C" PyObject *
socket_sendto(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED_NOTWRITEBUSY(self);
  VERIFY_PROTO(self->ob_proto == KProtocolInetUdp);

  char* data;
  int data_len;
  char* addr = NULL;
  int addr_len = 0;
  int port = -1;
  int flag = 0;
  PyObject* c = NULL;

  if (!PyArg_ParseTuple(args, "s#i(s#i)|O", &data, &data_len,
                        &flag, &addr, &addr_len, &port, &c)) {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "s#(s#i)|O", &data, &data_len,
                          &addr, &addr_len, &port, &c))
      return NULL;
  }

  if (c == Py_None)
    c = NULL;
  else if (c && !PyCallable_Check(c)) {
    PyErr_BadArgument();
    return NULL;
  }

  CSocketAo* ao = &(self->SE->iWriteAo);
  ao->iData[0] = (TAny*)data_len;
  ao->iCallback = &socket_write_return;
  ao->iPyCallback = c;
  ao->iDataBuf.Set((TUint8*)data, data_len, data_len);
  
  TInt error = set_symbian_inet_addr(ao->iInetAddr, addr, addr_len, port);
  if (error != KErrNone)
    return PySocket_Err(error);
  
  self->SE->iSocket.SendTo(ao->iDataBuf, ao->iInetAddr, flag, ao->iStatus);
  return ao->HandleReturn((PyObject*)self);
}

extern "C" PyObject *
socket_setsockopt(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED(self);

  int level;
  TUint opt;
  int val = 0;
  char* val_str = NULL;
  int val_str_len = 0;

  if (!PyArg_ParseTuple(args, "iii", &level, &opt, &val)) {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "iis#", &level, &opt, &val_str, &val_str_len))
      return NULL;
  }

  if ((opt!=KSoTcpOobInline) && (opt!=KSORecvBuf) && (opt!=KSoReuseAddr)
      && (opt!=KSOSendBuf) && (opt!=KSockStream) && (opt!=KSockDatagram)) 
    return PySocket_Err("Unsupported option");

  TInt error;

  if (val_str_len == 0)
    error = self->SE->iSocket.SetOpt(opt, level, val);
  else {
    TPtrC8 buf_value((TUint8*)val_str, val_str_len);
    error = self->SE->iSocket.SetOpt(opt, level, buf_value);
  }
  RETURN_SOCKET_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
socket_shutdown(Socket_object *self, PyObject *args)
{
  CHECK_NOTCLOSED(self);
  if(!self->ob_is_connected)
    return PySocket_Err("Attempt to shut down a disconnected socket");

  int how;
  if (!PyArg_ParseTuple(args, "i", &how))
    return NULL;
  
  RSocket::TShutdown mode = RSocket::ENormal;
  switch (how) {
    //  case 0:
    //    mode = RSocket::EStopInput;
    //    break;
    //  case 1:
    //    mode = RSocket::EStopOutput;
    //    break;
  case 2:
    mode = RSocket::EImmediate;
    break;
  default:
    return PySocket_Err("Only shutdown option 2 supported");
  }

  // do always as 2

  TRequestStatus st;
  self->SE->iSocket.Shutdown(mode, st);
  User::WaitForRequest(st);
  RETURN_SOCKET_ERROR_OR_PYNONE(st.Int());
}
extern "C" PyObject *
socket_list_aps()
{
  TInt error = KErrNone;
  CCommsDatabase* db = NULL;
  CApListItemList* apListItemList = NULL;
  CApSelect* apSelect = NULL;
  CApUtils *aputil = NULL;

  PyObject *py_aplist = NULL;
  TInt listPosition=0;
  TInt itemsInList=0;
  
  TRAP(error, {
    db=CCommsDatabase::NewL(EDatabaseTypeIAP);
    apListItemList = new (ELeave) CApListItemList();
    
    apSelect = CApSelect::NewLC(*db,
                                KEApIspTypeAll,
                                EApBearerTypeAll,
                                KEApSortNameAscending);
    aputil = CApUtils::NewLC(*db);
    CleanupStack::Pop(2);
  });
  if (error != KErrNone) 
  {
    delete aputil;
    delete apSelect;
    delete apListItemList;
    delete db;
    return SPyErr_SetFromSymbianOSErr(error);
  }                          
   
  py_aplist = PyList_New(apSelect->Count());
  if(py_aplist==NULL){
    delete aputil;
    delete apSelect;
    delete apListItemList;
    delete db;
    return PyErr_NoMemory();
  }
  
  TRAP(error, itemsInList = apSelect->AllListItemDataL(*apListItemList))
  if (error != KErrNone)
  {
    Py_DECREF(py_aplist);
    delete aputil;
    delete apSelect;
    delete apListItemList;
    delete db;
    return SPyErr_SetFromSymbianOSErr(error);                             
  } 
  
  if (itemsInList>0)
  {  
    if (apSelect->MoveToFirst())
    {
      do
      {
        TUint32 uid = apSelect->Uid();
        TInt iapid = aputil->IapIdFromWapIdL(uid);
        PyObject *apValue = NULL;
        apValue = Py_BuildValue("{s:i,s:u#}", "iapid",
                                               iapid, 
                                               "name",
                                               apSelect->Name().Ptr(), apSelect->Name().Length()); 
       
        if((apValue == NULL) || (PyList_SetItem(py_aplist,listPosition,apValue)!=0)){
          Py_XDECREF(apValue);
          Py_DECREF(py_aplist);
          delete aputil;
          delete apSelect;
          delete apListItemList;
          delete db;
          return NULL;
        }
        listPosition++;
      }
      while(apSelect->MoveNext());
    }
  }
  

  delete aputil;
  delete apSelect;
  delete apListItemList;
  delete db;

  return py_aplist;
}

extern "C" {

  const static PyMethodDef Socket_methods[] = {
    {"accept", (PyCFunction)socket_accept, METH_VARARGS},
    {"bind", (PyCFunction)socket_bind, METH_VARARGS},
    {"close", (PyCFunction)socket_close, METH_NOARGS},
    {"connect", (PyCFunction)socket_connect, METH_VARARGS},
    {"connect_ex", (PyCFunction)socket_not_implemented, METH_VARARGS},
    {"fileno", (PyCFunction)socket_not_implemented, METH_NOARGS},
    {"getpeername", (PyCFunction)socket_not_implemented, METH_VARARGS},
    {"getsockname", (PyCFunction)socket_not_implemented, METH_VARARGS},
    {"getsockopt", (PyCFunction)socket_getsockopt, METH_VARARGS},
    {"gettimeout", (PyCFunction)socket_not_implemented, METH_NOARGS},
    {"listen", (PyCFunction)socket_listen, METH_VARARGS},
    {"recv", (PyCFunction)socket_recv, METH_VARARGS},  
    {"recvfrom", (PyCFunction)socket_recvfrom, METH_VARARGS},  
    {"send", (PyCFunction)socket_write, METH_VARARGS},
    {"sendall", (PyCFunction)socket_write, METH_VARARGS},
    {"sendto", (PyCFunction)socket_sendto, METH_VARARGS},
    {"setblocking", (PyCFunction)socket_not_implemented, METH_VARARGS},
    {"settimeout", (PyCFunction)socket_not_implemented, METH_VARARGS},
    {"setsockopt", (PyCFunction)socket_setsockopt, METH_VARARGS},
    {"shutdown", (PyCFunction)socket_shutdown, METH_VARARGS},
    {NULL, NULL}           // sentinel
  };

  static PyObject *
  Socket_getattr(Socket_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)Socket_methods, (PyObject *)op, name);
  }

  static const PyTypeObject c_Socket_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "e32socket.Socket",                       /*tp_name*/
    sizeof(Socket_object),                    /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)Socket_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)Socket_getattr,              /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };
} //extern C

/*
 *
 * Implementation of socket.SSL
 *
 * SSLEngine: an AO to manage SSL operations
 */

#ifdef HAVE_SSL

#ifndef EKA2
class SSLEngine: public CActive
#else
NONSHARABLE_CLASS(SSLEngine): public CActive
#endif
{
 public:
  SSLEngine();
  ~SSLEngine();

  TInt Connect(CSocketEngine &aSE, TPtrC8  &host);

  TInt SSLSend(TPtrC8 &, TSockXfrLength &);
  TInt SSLRecv(TPtr8&);
  TInt SSLRecv(TPtr8&, TSockXfrLength&);

 private:
  void CloseConnection();
  void RunL();
  void DoCancel() { if (runState != EConnectionClosed) CloseConnection(); };

  CSecureSocket* SecSocket;

#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif

  TInt error;
  enum State {EIdle, EConnectionClosed, EBusy};
  State runState;
};

SSLEngine::SSLEngine():
  CActive(CActive::EPriorityStandard),
  SecSocket(NULL),
  error(KErrNone),
  runState(EIdle)
{
  CActiveScheduler::Add(this);
}

TInt SSLEngine::Connect(CSocketEngine &aSE, TPtrC8 &host)
{ 
  /*  if (protocol == KProtocol_TLS)
    SecSocket = CSecureSocket::NewL(aSE.iSocket, _L("TLS1.0"));
    else if (protocol == KProtocol_SSL) */
  SecSocket = CSecureSocket::NewL(aSE.iSocket, _L("SSL3.0"));
  
  if (SecSocket == NULL)
    return KErrNoMemory;
  
  Py_BEGIN_ALLOW_THREADS
  SecSocket->FlushSessionCache();

  #if SERIES60_VERSION>=30
    if (host.Length() > 0) {
      error = SecSocket->SetOpt(KSoSSLDomainName, KSolInetSSL, host);
    }
  #endif

  SecSocket->StartClientHandshake(iStatus);
  runState = EBusy;
  SetActive();
#ifdef HAVE_ACTIVESCHEDULERWAIT
  iWait.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS;
  return error;
}

SSLEngine::~SSLEngine()
{
  if (runState != EConnectionClosed)
    CloseConnection();

  delete SecSocket; SecSocket = NULL;
}

TInt SSLEngine::SSLSend(TPtrC8 &data, TSockXfrLength &len)
{
  if (runState != EIdle) 
    if (runState == EConnectionClosed) {
      PySocket_Err("Attempt to use a closed socket");
      return KErrPython;
    } else
      return KErrInUse;

  Py_BEGIN_ALLOW_THREADS
  runState = EBusy;
  SecSocket->Send(data, iStatus, len);
  SetActive();
#ifdef HAVE_ACTIVESCHEDULERWAIT
  iWait.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  return error;
}

TInt SSLEngine::SSLRecv(TPtr8& buff)
{
  if (runState != EIdle) 
    if (runState == EConnectionClosed) {
      PySocket_Err("Attempt to use a closed socket");
      return KErrPython;
    } else
      return KErrInUse;

  Py_BEGIN_ALLOW_THREADS
  runState = EBusy;
  SecSocket->Recv(buff, iStatus);
  SetActive();
#ifdef HAVE_ACTIVESCHEDULERWAIT
  iWait.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  return error;
}

TInt SSLEngine::SSLRecv(TPtr8& buff, TSockXfrLength& len)
{
  if (runState != EIdle) 
    if (runState == EConnectionClosed) {
      PySocket_Err("Attempt to use a closed socket");
      return KErrPython;
    } else
      return KErrInUse;

  Py_BEGIN_ALLOW_THREADS
  runState = EBusy;
  SecSocket->RecvOneOrMore(buff, iStatus, len);
  SetActive();
#ifdef HAVE_ACTIVESCHEDULERWAIT
  iWait.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  return error;
}

void SSLEngine::CloseConnection()
{
  SecSocket->CancelAll();
  SecSocket->Close();
  runState = EConnectionClosed;
}

void SSLEngine::RunL()
{
  if (iStatus != KErrNone) {
    error = iStatus.Int();
    CloseConnection();
#ifdef HAVE_ACTIVESCHEDULERWAIT
    iWait.AsyncStop();
#else
    CActiveScheduler::Stop();
#endif
    return;
  }

  switch (runState) {
    case EBusy:     
      error = iStatus.Int();
#ifdef HAVE_ACTIVESCHEDULERWAIT
      iWait.AsyncStop();
#else
      CActiveScheduler::Stop();
#endif
      runState = EIdle;
      break;    

    default:
#ifdef HAVE_ACTIVESCHEDULERWAIT
      iWait.AsyncStop();
#else
      CActiveScheduler::Stop();
#endif
      break;
    }
}

#define SSL_type ((PyTypeObject*)SPyGetGlobalString("SSLType"))

/*
 *  No class members here as they do not get
 *  properly initialized by PyObject_New() !!!
 */
struct SSL_object {
  PyObject_VAR_HEAD
  SSLEngine* SSLE;
};

extern "C" PyObject *
new_ssl_object(PyObject* /*self*/, PyObject *args)
{
  Socket_object *so = NULL;
  char *key_file = NULL;
  char *cert_file = NULL;
  char *hostName = NULL;
  TPtrC8 host;
 
  if ( !PyArg_ParseTuple(args, "O!|zzz", Socket_type, &so, &key_file, &cert_file, &hostName) )
    return NULL;

  if ((key_file != NULL) || (cert_file != NULL))
    return PySocket_Err("Unsupported feature");

  if (so->SE->GetProtoInfo() != KProtocolInetTcp) 
    return PySocket_Err("Required Socket must be of AF_INET, IPPROTO_TCP type");

  if (!so->ob_is_connected) 
    return PySocket_Err("TCP Socket must be connected");
 
  if (hostName != NULL) {
    host.Set((unsigned char *)hostName);
  }
 
  SSL_object *sslo = PyObject_New(SSL_object, SSL_type);
  if (sslo == NULL)
    return PyErr_NoMemory();

  sslo->SSLE = new SSLEngine();
  if (sslo->SSLE == NULL) {
    PyObject_Del(sslo);
    return PyErr_NoMemory();
  }

  int err=sslo->SSLE->Connect(*(so->SE), host);
  if (err != KErrNone) {
    delete key_file;
    key_file=NULL;
    delete cert_file;
    cert_file=NULL;
    delete hostName;
    hostName=NULL;
    return SPyErr_SetFromSymbianOSErr(err);
  }
  return (PyObject*) sslo;
}

extern "C" PyObject *
ssl_write(SSL_object* self, PyObject *args)
{
  char* data;
  int dataLenght = 0;
  TInt error = KErrNone;
  TSockXfrLength length;

  if (!PyArg_ParseTuple(args, "s#", &data, &dataLenght))
      return NULL;

  if (dataLenght == 0) 
      return PySocket_Err("Data to be sent missing");

  TPtrC8 dataToSend((TUint8*)data, dataLenght);

  error = self->SSLE->SSLSend(dataToSend, length);

  if (error != KErrNone)
    return PySocket_Err(error);

  if (length() > 0)
    return Py_BuildValue("i", length() ); 
  else
    return Py_BuildValue("i", dataToSend.Size());  
}

extern "C" PyObject *
ssl_read(SSL_object* self, PyObject *args)
{
  int request = 0;
  TInt error = KErrNone;

  if ( !PyArg_ParseTuple(args, "|i", &request) )
       return NULL;

  if (request >= 1) {
      PyObject *my_str = PyString_FromStringAndSize(NULL, request);
      TPtr8 rec_buf((TUint8*)(PyString_AsString(my_str)), 0, request);
      rec_buf.FillZ(request);

      error = self->SSLE->SSLRecv(rec_buf);
  
      if (error != KErrNone && error != KErrEof) {
        Py_DECREF(my_str);
        return PySocket_Err(error);
      }

      _PyString_Resize(&my_str, rec_buf.Size());
      return my_str;
  }
  else if (request == 0) {       //read all data
      int totalDataRead = 0;
      TSockXfrLength length;

      PyObject *totalData = PyString_FromStringAndSize(NULL, MaxBufferSize*4);
      TPtr8 rec_buf((TUint8*)(PyString_AsString(totalData)), 0, MaxBufferSize*4);

      while ((error != KErrEof) && (error != KErrDisconnected)) {
        length=0;
        error = self->SSLE->SSLRecv(rec_buf, length);

        totalDataRead += length();

        if (error != KErrNone && error != KErrEof && error != KErrDisconnected) {
          Py_DECREF(totalData);
          return PySocket_Err(error);
        } 
    
        if ( (rec_buf.MaxLength()-totalDataRead) < MaxBufferSize && (error != KErrEof)) 
          _PyString_Resize(&totalData, (PyString_Size(totalData)+MaxBufferSize*4));

        rec_buf.Set((TUint8*)(PyString_AsString(totalData)+totalDataRead), 0, PyString_Size(totalData)-totalDataRead);     
      } 
  /* object size could be bigger than buffer used */
      _PyString_Resize(&totalData, totalDataRead);
      return totalData;
  }
  else /* (request < 0) */
       return PySocket_Err("Invalid amount of data requested");
}

extern "C" {

  static void ssl_dealloc(SSL_object *sslo)
  {
    if (sslo->SSLE) {
      delete sslo->SSLE;
      sslo->SSLE = NULL;
    }
    PyObject_Del(sslo);
  }

  const static PyMethodDef ssl_methods[] = {
    {"write", (PyCFunction)ssl_write, METH_VARARGS},
    {"read", (PyCFunction)ssl_read, METH_VARARGS},
    {NULL, NULL}  
  };

  static PyObject *
  ssl_getattr(SSL_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)ssl_methods, (PyObject *)op, name);
  }

  static const PyTypeObject c_ssl_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "e32socket.ssl",                          /*tp_name*/
    sizeof(SSL_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)ssl_dealloc,                  /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)ssl_getattr,                 /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };
} //extern C

#endif //HAVE_SSL

extern "C" PyObject *
ap_start(AP_object *self, PyObject* /*args*/)
{
  if (self->started==EFalse)
  {
    TInt error = KErrNone;
    TStaticData* sd = GetServer();
    if(sd==NULL){
      return NULL;
    }
    error = self->connection->Open(sd->rss);

    if (error != KErrNone)
    {
      return SPyErr_SetFromSymbianOSErr(error);
    }
    

    TCommDbConnPref prefs;
    
    prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
    prefs.SetDirection(ECommDbConnectionDirectionOutgoing);
    prefs.SetIapId(self->apId);   
    error = self->connection->Start(prefs);
    if(error!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);
    }
    self->started=ETrue;
  }

  if (self != NULL) {
    TUint iActiveConnections = 0;
    TInt err = self->connection->EnumerateConnections(iActiveConnections);
    if (err!=KErrNone){
        return SPyErr_SetFromSymbianOSErr(err);
    }
    if (iActiveConnections == 0) {
      TCommDbConnPref prefs;
    
      prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
      prefs.SetDirection(ECommDbConnectionDirectionOutgoing);
      prefs.SetIapId(self->apId);   
      err = self->connection->Start(prefs);
      if(err!=KErrNone){
        return SPyErr_SetFromSymbianOSErr(err);
      }
    }
  }

  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
ap_stop(AP_object *self, PyObject* /*args*/)
{
  if (self->connection != NULL) 
  {
    self->connection->Close();
    self->started=EFalse;
  }
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
ap_ip(AP_object *self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  if(self->apId == 0)
  {
    return PySocket_Err("Default access point is not set"); 
  }

  TStaticData* sd = GetServer();
  if(sd==NULL){
    return NULL;
  } 

  RSocket iSocket;
  error = iSocket.Open(sd->rss,KAfInet,KSockStream,KProtocolInetTcp);
  if (error != KErrNone) {
    SPyErr_SetFromSymbianOSErr(error);
    return NULL;
  }

  TSoInetInterfaceInfo ifinfo;
  TPckg<TSoInetInterfaceInfo> ifinfopkg(ifinfo);
  TSoInetIfQuery ifquery;
  TPckg<TSoInetIfQuery> ifquerypkg(ifquery);


  error = iSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }

  while((error = iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, ifinfopkg)) == KErrNone)
  {
    TSoInetInterfaceInfo &info = ifinfopkg();
    ifquerypkg().iName = info.iName;
    error = iSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, ifquerypkg);
    if (error != KErrNone) {
      break;
    }
    if (ifquerypkg().iZone[1] == (TUint)self->apId) 
    {
      if (info.iState != EIfUp)
      {
        Py_INCREF(Py_None);
        return Py_None;
      }
      else {
          TBuf<40> ifAddress;
          ifAddress.FillZ(40);
          ifinfo.iAddress.Output(ifAddress);    

          TBuf8<MaxIPAddrLength> final;
          final.FillZ(MaxIPAddrLength);
          final.Copy(ifAddress);
   
         return Py_BuildValue("s#", final.Ptr(), final.Size() );   
  

      }
    }
  }
  return SPyErr_SetFromSymbianOSErr(error);
}

/*
 * Deallocate ap.
 */
extern "C" {
  static void ap_dealloc(AP_object *apo)
  {
    if (apo->connection) {
      apo->connection->Close();
      delete apo->connection;
      apo->connection = NULL;
    }
    apo->apId = 0;
    PyObject_Del(apo);
  }
}


/*
 * Allocate ap.
 */
extern "C" PyObject *
new_ap_object(TInt accessPointId)
{
  TInt error = KErrNone;
  
  AP_object *apo = PyObject_New(AP_object, AP_type);
  if (apo == NULL)
  {
    return PyErr_NoMemory();
  }
  
  apo->connection = NULL;
  apo->apId = accessPointId;
  apo->started = EFalse;

  TRAP(error, {
    apo->connection = new (ELeave) RConnection();
  });
  if(error!=KErrNone){
    ap_dealloc(apo);
    return SPyErr_SetFromSymbianOSErr(error);
  }

  TStaticData* sd = GetServer();
  if(sd==NULL){
    return NULL;
  }
  return (PyObject*) apo;
}

/*
 * Return Access point object.
 */
extern "C" PyObject *
access_point(PyObject* /*self*/, PyObject* args)
{
  TInt accessPointId = -1;
  if (!PyArg_ParseTuple(args, "i", &accessPointId)){ 
    return NULL;
  }
  if(accessPointId <= 0){
    PyErr_SetString(PyExc_ValueError, "illegal access point id");
    return NULL;
  }
  return new_ap_object(accessPointId);
}

/*
 * set default access point 
 */  
extern "C" PyObject *
set_default_ap(PyObject* /*self*/, PyObject* args)
{
  AP_object *defaultAP = NULL;
  TStaticData* sd = GetServer();
  if(sd==NULL){
    return NULL;
  } 
  
  if (!PyArg_ParseTuple(args, "O", &defaultAP)){
    return NULL;
  }
  
  if(PyObject_TypeCheck(defaultAP, AP_type)){

    Py_XDECREF(sd->apo);  
    Py_INCREF(defaultAP);
    sd->apo = defaultAP;
      
    Py_INCREF(Py_None);
    return Py_None;
  
  }
  else if ((PyObject*)defaultAP == Py_None){
    if(sd->apo!=NULL){
      sd->apo->apId = 0;
      sd->apo->started = EFalse;
      Py_DECREF(sd->apo);
      sd->apo = NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
  }
  else {
    PyErr_SetString(PyExc_ValueError, 
                    "Parameter must be access point object or None");
    return NULL;
  }
}

extern "C" PyObject *
select_ap(PyObject* /*self*/, PyObject* /*args*/)
{  
  return ap_select_ap();
}

extern "C" {
  
  const static PyMethodDef ap_methods[] = {
    {"start", (PyCFunction)ap_start, METH_NOARGS},
    {"stop", (PyCFunction)ap_stop, METH_NOARGS},
    {"ip", (PyCFunction)ap_ip, METH_NOARGS},
    {NULL, NULL}  
  };

  static PyObject*
  ap_getattr(AP_object *apo, char *name)

  {
    return Py_FindMethod((PyMethodDef*)ap_methods, (PyObject*) apo, name);
  }

  static const PyTypeObject c_ap_type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "_ap.AP",
    sizeof(AP_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)ap_dealloc,                  /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)ap_getattr,                 /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };
  

} //extern C


/*
 *
 * Implementation of socket.HostResolver services
 *
 */
/*
 * HREngine: an AO to manage HostResolver operations
 */
#ifndef EKA2
class HREngine: public CActive
#else
NONSHARABLE_CLASS(HREngine): public CActive
#endif
{
 public:
  HREngine();
  TInt Connect();
  ~HREngine();
  TInt ResolveHbyN(TPtrC&, TNameEntry&);

  RHostResolver   hr;

 private:
  void RunL();
  void DoCancel() {};
  void StopL() { iState = EDisconnected; };

#ifdef HAVE_ACTIVESCHEDULERWAIT
  CActiveSchedulerWait iWait;
#endif
  TInt error;
  enum State {EIdle, EDisconnected, EResolving };
  State iState;
};

HREngine::HREngine():
  CActive(CActive::EPriorityStandard),
  error(KErrNone),
  iState(EIdle)
{
  CActiveScheduler::Add(this);
}

TInt HREngine::Connect()
{
  if (GetServer() == NULL) {
    PySocket_Err("Could not open socket server");
    return -1;
  } 
  if (GetServer()->apo != NULL) {
    if (!ap_start(GetServer()->apo, NULL)) {
      return -1;
    }
    error = hr.Open(GetServer()->rss, KAfInet, KProtocolInetTcp, *GetServer()->apo->connection);
  }
  else {
    error = hr.Open(GetServer()->rss, KAfInet, KProtocolInetTcp);
  }
  if (error != KErrNone) {
    if (GetServer()->apo != NULL) {
      ap_stop(GetServer()->apo,NULL);
    }
    PySocket_Err(error);
    return -1;  
  }  
  return KErrNone;
}

HREngine::~HREngine()
{
  StopL();
  hr.Close();
}

TInt HREngine::ResolveHbyN(TPtrC &name, TNameEntry &result)
{
  if (iState != EIdle) 
    return KErrInUse;

  Py_BEGIN_ALLOW_THREADS
  hr.GetByName(name, result, iStatus);
  iState = EResolving;
  SetActive();
#ifdef HAVE_ACTIVESCHEDULERWAIT
  iWait.Start();
#else
  CActiveScheduler::Start();
#endif
  Py_END_ALLOW_THREADS
  return error;
}

void HREngine::RunL()
{
  if (iStatus != KErrNone) {
    error = iStatus.Int();
    StopL();
#ifdef HAVE_ACTIVESCHEDULERWAIT
    iWait.AsyncStop();
#else
    CActiveScheduler::Stop();
#endif
    return;
  }
  
  switch (iState) {
  case EResolving:
    error = iStatus.Int();
#ifdef HAVE_ACTIVESCHEDULERWAIT
    iWait.AsyncStop();
#else
    CActiveScheduler::Stop();
#endif
    iState = EIdle;
    break;    
 
  default:
#ifdef HAVE_ACTIVESCHEDULERWAIT
    iWait.AsyncStop();
#else
    CActiveScheduler::Stop();
#endif
    break;
  }
}

extern "C" PyObject *
get_host_by_addr(PyObject* /*self*/, PyObject *args)
{
  char* ipText;
  int ipTextL;
  TInt error = KErrNone;
  TNameEntry result;
  TNameRecord aNameRecord;

  if (!PyArg_ParseTuple(args, "s#", &ipText, &ipTextL))
    return NULL;

  if (ipTextL <= 0) 
    return PySocket_Err("Data missing");

  HREngine* hre = new HREngine();
  if (hre == NULL)
    return PyErr_NoMemory();
  if (hre->Connect() != KErrNone) {
    delete hre;
    hre = NULL;
    return NULL;
  }

  TInetAddr aAddr;
  error = set_symbian_inet_addr(aAddr, ipText, ipTextL, 0);
  if (error != KErrNone) {
    delete hre;
    hre = NULL;
    return PySocket_Err(error);
  }

  Py_BEGIN_ALLOW_THREADS
  error = hre->hr.GetByAddress(aAddr, result);
  Py_END_ALLOW_THREADS

  if (error == KErrNone) {
    aNameRecord = result();
    delete hre;
    hre = NULL;
    return Py_BuildValue("(u#[][s#])", aNameRecord.iName.Ptr(), (aNameRecord.iName).Length(), ipText, ipTextL ); 
  }
  else {
    delete hre;
    hre = NULL;
    return PyGAI_Err(error);
  }
}

extern "C" PyObject *
get_host_by_name(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  TNameEntry result; 
  TNameRecord aNameRecord;
  char *aHostNamePtr;
  int aHostNameLen;

  if (!PyArg_ParseTuple(args, "s#", &aHostNamePtr, &aHostNameLen))
      return NULL;

  /* Input is a plain string which must be converted into a unicode buffer.
   * We go through all these contortions because the original Python 2.2.2 gethostbyname accepts
   * both strings and unicode objects. */
  PyObject* str = PyString_FromStringAndSize(aHostNamePtr, aHostNameLen);
  if (!str)
      return NULL;
  PyObject* str16 = PyUnicode_FromObject(str);  
  if (!str16) {
      Py_DECREF(str);
      return NULL; /* Conversion failed. */
  }
  
  TPtrC aName((TUint16 *)PyUnicode_AS_DATA(str16), PyUnicode_GetSize(str16));  
  HREngine* hre = new HREngine();
  if (hre == NULL) {
    Py_DECREF(str);
    Py_DECREF(str16);
    return PyErr_NoMemory();
  }
  if (hre->Connect() != KErrNone) {
    delete hre;
    hre = NULL;
    Py_DECREF(str);
    Py_DECREF(str16);
    return NULL;
  }
  error = hre->ResolveHbyN(aName, result);  
  Py_DECREF(str);
  Py_DECREF(str16);

  if ( error == KErrNone) {
    aNameRecord = result();   
    TBuf<MaxIPAddrLength> ipAddr;
    TInetAddr::Cast(aNameRecord.iAddr).Output(ipAddr); 
    TBuf8<MaxIPAddrLength> final;
    final.FillZ(MaxIPAddrLength);
    final.Copy(ipAddr);
   
    delete hre;
    hre = NULL;   
    return Py_BuildValue("s#", final.Ptr(), final.Size() );   
  }
  else {
    delete hre;
    hre = NULL;
    return PyGAI_Err(error);
  }
}

extern "C" PyObject *
get_host_by_name_ex(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  char* ipText = NULL;
  int ipTextL = 0;
  TNameEntry result; 
  TNameRecord aNameRecord;

  if (!PyArg_ParseTuple(args, "s#", &ipText, &ipTextL))
      return NULL;

  if (ipTextL <= 0) 
      return PySocket_Err("Data missing");

  HREngine* hre = new HREngine();
  if (hre == NULL)
    return PyErr_NoMemory();
  if (hre->Connect() != KErrNone) {
    delete hre;
    hre = NULL;
    return NULL;
  }
  /* Input is a plain string which must be converted into a unicode buffer.
   * We go through all these contortions because the original Python 2.2.2 gethostbyname_ex accepts
   * both strings and unicode objects. */
  PyObject* str = PyString_FromStringAndSize(ipText, ipTextL);
  if (!str)
      return NULL;
  PyObject* str16 = PyUnicode_FromObject(str);  
  if (!str16) {
      Py_DECREF(str);
      return NULL; /* Conversion failed. */
  }
  TPtrC aName((TUint16 *)PyUnicode_AS_DATA(str16), PyUnicode_GetSize(str16));  

  error = hre->ResolveHbyN(aName, result);
  Py_DECREF(str);
  Py_DECREF(str16);

  if ( error == KErrNone) {
    aNameRecord = result();   
    TBuf<MaxIPAddrLength> ipAddr;
    TInetAddr::Cast(aNameRecord.iAddr).Output(ipAddr);
      
    TBuf8<MaxIPAddrLength> final;
    final.FillZ(MaxIPAddrLength);
    final.Copy(ipAddr);

    delete hre;
    hre = NULL;   
    return Py_BuildValue("(s#[][s#])", ipText, ipTextL, final.Ptr(), final.Size()); 
  }
  else {
    delete hre;
    hre = NULL;
    return PyGAI_Err(error);
  }
}

#ifdef not_def
/* it doesn't seem to work, so it is excluded from building */
extern "C" PyObject *
host_next(HostRes_object *self)
{
  TInt error = KErrNone;
  TNameRecord aNameRecord;

  if (self->hr_obj->status == NONE) 
          return PySocket_Err("get_by_address or get_by_name must be called first");

  error = self->hr_obj->hr.Next(self->hr_obj->entry);

  if (error != KErrNone)
          if (self->hr_obj->status == BYNAME) {
              aNameRecord = self->hr_obj->entry();
              TBuf<15> ipAddr;
                  TInetAddr::Cast(aNameRecord.iAddr).Output(ipAddr);
              return Py_BuildValue("u#", ipAddr.Ptr(), ipAddr.Length() );   
          }
          else if (self->hr_obj->status == BYADDRESS) {
              aNameRecord = self->hr_obj->entry();
              return Py_BuildValue("u#", aNameRecord.iName.Ptr(), (aNameRecord.iName).Length() ); 
          }

  RETURN_SOCKET_ERROR_OR_PYNONE(error);
}
#endif

#ifdef not_def
/* Symbian DB for Service Resolver looks empty at the moment  */
extern "C" PyObject *
get_service(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  TInt port = -1;
  char* name = NULL;
  int nameL = 0;
  int family = 0;
  int type = 0;
  int protocol = 0;

  if (!PyArg_ParseTuple(args, "iiii", &port, &family, &type, &protocol )) {
    PyErr_Clear();
    if (!PyArg_ParseTuple(args, "s#iii", &name, &nameL, &family, &type, &protocol ))
      return NULL;
  }

  RServiceResolver resolver;
  RSocketServ socketServer;

  error = socketServer.Connect();
  if (error != KErrNone)
    return PySocket_Err(error);
  
  error = resolver.Open(socketServer, family, type, protocol);
  if (error != KErrNone) {
    socketServer.Close();    
    return PySocket_Err(error);
  }
  
  if (port!=-1) { //port given
    TBuf<MaxIPAddrLength*2> desName;
    desName.FillZ(MaxIPAddrLength*2);
    error = resolver.GetByNumber(port, desName);
    resolver.Close();
    socketServer.Close();    

    if (error != KErrNone) 
      return PySocket_Err(error);
    return Py_BuildValue("s#", desName.Ptr(), desName.Length());

  } else { // name given
    TPtr desName((TUint16*)name, nameL);
    TPortNum portBuff;
    portBuff.Num(port);
    error = resolver.GetByName(desName, portBuff);

    resolver.Close();
    socketServer.Close();    

    if (error != KErrNone) 
      return PySocket_Err(error);
    return Py_BuildValue("i", port);
  }
}
#endif

/*
 *
 * Implementation of e32socket.bt_discover() & bt_obex_discover()
 *
 */

/* Service Dictionary creation from existing Symbian Dictionary */
PyObject* createDictionary(BTDevice_data &BTDD) {

   PyObject* D = PyDict_New();

   while (! BTDD.aBTObjExCl->iBTSearcher->SDPDict->iStack.IsEmpty() ) {

     Dictionary* dItem = BTDD.aBTObjExCl->iBTSearcher->SDPDict->iStack.First();

     PyObject* desc = Py_BuildValue("s#", (dItem->GetKey()).Ptr(), (dItem->GetKey()).Length() );
     PyObject* port = Py_BuildValue("i", dItem->GetValue());
     PyObject* uObj = PyUnicode_FromObject(desc);

     if ( PyDict_GetItem(D, desc) == NULL)
       PyDict_SetItem(D, (uObj), port);
     
     Py_DECREF(desc);
     Py_DECREF(port);
     Py_DECREF(uObj);

     BTDD.aBTObjExCl->iBTSearcher->SDPDict->iStack.Remove(*dItem);
   }

   return D;
}

PyObject* discovery(const int type, char* addr, int &addrL)
{
  TInt error = KErrNone;
  char str[MaxIPAddrLength+2];

  BTDevice_data BTDD;

  BTDD.aBTObjExCl = CObjectExchangeClient::NewL(type);
  if (BTDD.aBTObjExCl == NULL) 
    return PyErr_NoMemory();
  
  if(addrL < MaxIPAddrLength) {
     error = BTDD.aBTObjExCl->DevServResearch();
     if (error != KErrNone) 
       return PySocket_Err(error);

     BTDD.address = BTDD.aBTObjExCl->iBTSearcher->aResponse().BDAddr();
     BTDD.port    = BTDD.aBTObjExCl->iBTSearcher->Port();

     TBuf8<6> bta;
     bta.FillZ(6);
     bta.Copy(BTDD.address.Des());
     PyOS_snprintf(str, MaxIPAddrLength+2, "%02x:%02x:%02x:%02x:%02x:%02x", bta[0], bta[1], bta[2], bta[3], bta[4], bta[5]);
     
     PyObject* servicesDictionary = createDictionary(BTDD);
     delete BTDD.aBTObjExCl;
     BTDD.aBTObjExCl = NULL;
     return Py_BuildValue("(sO)", str, servicesDictionary);
  }
  else {
     TBTSockAddr address;
     TBuf<MaxIPAddrLength+2> aBTHexAddr;
     aBTHexAddr.FillZ(MaxIPAddrLength+2);
     aBTHexAddr.Copy((TPtrC8 ((TUint8*)addr, addrL)));
     TBuf8<6> aBTAddress;
     str2Hex(aBTHexAddr, aBTAddress);
     address.SetBTAddr(aBTAddress);

     PyOS_snprintf(str, MaxIPAddrLength+2, "%02x:%02x:%02x:%02x:%02x:%02x",  aBTAddress[0],  aBTAddress[1],  aBTAddress[2],  aBTAddress[3],  aBTAddress[4],  aBTAddress[5]);

     error = BTDD.aBTObjExCl->ServiceResearch(address.BTAddr());
     if (error != KErrNone) 
       return PySocket_Err(error);

     PyObject* servicesDictionary = createDictionary(BTDD);
     delete BTDD.aBTObjExCl;
     BTDD.aBTObjExCl = NULL;
     return Py_BuildValue("(sO)", str, servicesDictionary);
  }
}

extern "C" PyObject *
bt_discover(Socket_object* /*self*/, PyObject *args)
{
  char *addr;
  int addrL = 0;
 
  if (!PyArg_ParseTuple(args, "|s#", &addr, &addrL))
      return NULL;

  return discovery(RfcommService, addr, addrL);
}

extern "C" PyObject *
bt_obex_discover(Socket_object* /*self*/, PyObject *args)
{
  char *addr;
  int addrL = 0;

  if (!PyArg_ParseTuple(args, "|s#", &addr, &addrL))
      return NULL;

  return discovery(ObexService, addr, addrL);
}

/*
 *
 * Implementation of e32socket.bt_rfcomm_get_available_server_channel()
 *
 */

extern "C" PyObject *
bt_rfcomm_get_available_server_channel(Socket_object* /*self*/, PyObject *args)
{
  TInt error = KErrNone; 
  TInt channel = -1;
  Socket_object *so = NULL;

  if (!PyArg_ParseTuple(args, "O!", Socket_type, &so))
      return NULL;

  error = so->SE->iSocket.GetOpt(KRFCOMMGetAvailableServerChannel, KSolBtRFCOMM, channel);

  if (error != KErrNone)
     return PySocket_Err(error);
  else
     return Py_BuildValue("i", channel);
}

/*
 *
 * Implementation of e32socket.bt_advertise_service() for rfcomm and obex
 *
 */

extern "C" PyObject *
bt_advertise_service(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  char* service;
  int serviceL = 0;
  Socket_object *so = NULL;
  int type = RfcommService;
  int runFlag = 0;
  TInt channel = -1;

  if (!PyArg_ParseTuple(args, "u#O!i|i", &service, &serviceL, Socket_type, &so, &runFlag, &type))  
    return NULL;

  if ((type != RfcommService) && (type != ObexService))
    return PySocket_Err("Service advertising can be only RFCOMM or OBEX");

  if ( (serviceL <1)
       || ((runFlag != 1) && (runFlag != 0)) ) 
          return PySocket_Err("Service name or flag missing or invalid");

  TPtrC serviceName((TUint16*)service, serviceL);
  if (!so->SE->iAdvertiser){
    so->SE->iAdvertiser = CObjectExchangeServiceAdvertiser::NewL(serviceName);
  }
  so->SE->iAdvertiser->SetServiceType(type);       //type can be RfcommService || ObexService

  channel = so->SE->iPort;
  if (channel < 0)
    return PySocket_Err("Socket has not a valid port. Bind it first");

  if (runFlag == 1 ) {
      TRAP(error, (so->SE->iAdvertiser->StartAdvertisingL(channel, serviceName) ));
      if (error != KErrNone) 
        return PySocket_Err(error);

      TRAP(error, (so->SE->iAdvertiser->UpdateAvailabilityL(ETrue) ));  
      if (error != KErrNone)
        return PySocket_Err(error);
  }
  else {
      TRAP(error, (so->SE->iAdvertiser->StopAdvertisingL() ));
      if (error != KErrNone) 
        return PySocket_Err(error);
  }

  RETURN_SOCKET_ERROR_OR_PYNONE(error);
}

/*
 *
 * Implementation of e32socket.set_security()
 *
 */

extern "C" PyObject *
set_security(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  Socket_object *so = NULL;
  /* mode: authentication = 1, encryption = 2, author = 4*/
  int mode = 0;
  int authentication = 1;
  int encryption = 0;
  int authorization = 0;
  int channel = -1;

  if ( !PyArg_ParseTuple(args, "O!i", Socket_type, &so, &mode) )
    return NULL;

  if (so->SE->GetProtoInfo() != KRFCOMM)
    return PySocket_Err("Given Socket is not of AF_BT family");

  channel = so->SE->iPort;
  if (channel < 0)
    return PySocket_Err("Socket has not a valid port. Bind it first");

  switch (mode) {
  case 1:
    break;
  case 2:
    authentication = 0;
    encryption = 1;
    break;
  case 3:
    encryption = 1;
    break;
  case 4:
    authentication = 0;
    authorization = 1;
    break;
  case 5:
    authorization = 1;
    break;
  case 6:
    authentication = 0;
    encryption = 1;
    authorization = 1;
    break;
  case 7:
    encryption = 1;
    authorization = 1;
    break;
  default:
    break;
  } 

  TRAP(error, 
  (CObjectExchangeServer::SetSecurityOnChannelL(authentication, encryption, authorization, channel)));
         
  RETURN_SOCKET_ERROR_OR_PYNONE(error);
}

/*
 * Implementation of e32socket.bt_obex_send_file()
 */

extern "C" PyObject *
bt_obex_send_file(PyObject* /*self*/, PyObject *args)
{
  char* iAddr = NULL;
  char* iFile = NULL;
  int iAddrL = 0;
  int iFileL = 0;
  int port = -1;
  TInt error = KErrNone;

  if ( !PyArg_ParseTuple(args, "s#iu#", &iAddr, &iAddrL, &port, &iFile, &iFileL) )
       return NULL;

  if (port <=0)
    return PySocket_Err("invalid channel");

  TPtrC file((TUint16*)iFile, iFileL);

  BTDevice_data BTDD;
  BTDD.aBTObjExCl = CObjectExchangeClient::NewL(ObexService);
  if (BTDD.aBTObjExCl == NULL) 
    return PyErr_NoMemory();

  TBTSockAddr btsaddress;
  set_symbian_bt_addr(btsaddress, iAddr, iAddrL, 0);

  error = BTDD.aBTObjExCl->ObexSend(btsaddress, port, file);

  delete BTDD.aBTObjExCl;
  BTDD.aBTObjExCl = NULL;

  RETURN_SOCKET_ERROR_OR_PYNONE(error);
}

/*
 * Implementation of e32socket.bt_obex_receive()
 */
extern "C" PyObject *
bt_obex_receive(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  Socket_object *so = NULL;
  char* path = NULL;
  int pathL = 0;
  TInt channel = -1;

  if ( !PyArg_ParseTuple(args, "O!u#", Socket_type, &so, &path, &pathL) )
          return NULL;

  if (so->SE->GetProtoInfo() != KRFCOMM) 
    return PySocket_Err("Given Socket is not of AF_BT family. Other types not supported");

  channel = so->SE->iPort;
  if (channel < 0)
    return PySocket_Err("Socket has not a valid port. Bind it first");

  BTDevice_data BTDD;
  BTDD.aBTObjExSrv = CObjectExchangeServer::NewL();
  if (BTDD.aBTObjExSrv == NULL) 
    return PyErr_NoMemory();

  BTDD.aBTObjExSrv->SetPort(channel);

  BTDD.aBTObjExSrv->fileName.Copy((TUint16*)path, pathL);

  /* Can't use Py_BEGIN_ALLOW_THREADS. Following code uses its own locker */
  //  TRAP(error, (BTDD.aBTObjExSrv->StartL() ));
  error = BTDD.aBTObjExSrv->StartSrv();

  delete BTDD.aBTObjExSrv;
  BTDD.aBTObjExSrv = NULL;

  RETURN_SOCKET_ERROR_OR_PYNONE(error);
}

extern "C" {

  static const PyMethodDef socket_methods[] = {
    
    {"socket", (PyCFunction)new_Socket_object, METH_VARARGS, NULL},
#ifdef HAVE_SSL
    {"ssl", (PyCFunction)new_ssl_object, METH_VARARGS, NULL},
#endif
    {"gethostbyaddr", (PyCFunction)get_host_by_addr, METH_VARARGS, NULL},
    {"gethostbyname", (PyCFunction)get_host_by_name, METH_VARARGS, NULL},
    {"gethostbyname_ex", (PyCFunction)get_host_by_name_ex, METH_VARARGS, NULL},
#ifdef not_def
    {"getservice", (PyCFunction)get_service, METH_VARARGS, NULL},
#endif
    {"bt_discover", (PyCFunction)bt_discover, METH_VARARGS},
    {"bt_obex_discover", (PyCFunction)bt_obex_discover, METH_VARARGS},
    {"bt_rfcomm_get_available_server_channel", (PyCFunction)bt_rfcomm_get_available_server_channel, METH_VARARGS},
    {"bt_advertise_service", (PyCFunction)bt_advertise_service, METH_VARARGS, NULL},
    {"set_security", (PyCFunction)set_security, METH_VARARGS, NULL},
    {"bt_obex_send_file", (PyCFunction)bt_obex_send_file, METH_VARARGS, NULL},
    {"bt_obex_receive", (PyCFunction)bt_obex_receive, METH_VARARGS, NULL},
    {"access_point", (PyCFunction)access_point, METH_VARARGS, NULL},
    {"set_default_access_point", (PyCFunction)set_default_ap, METH_VARARGS, NULL},
    {"select_access_point", (PyCFunction)select_ap, METH_NOARGS},
    {"access_points", (PyCFunction)socket_list_aps, METH_NOARGS},
    {NULL, NULL}           /* sentinel */
  };

  DL_EXPORT(void) initsocket(void)
  {
    PyTypeObject* socket_type = PyObject_New(PyTypeObject, &PyType_Type);
    *socket_type = c_Socket_type;
    socket_type->ob_type = &PyType_Type;

    SPyAddGlobalString("SocketType", (PyObject*)socket_type);

#ifdef HAVE_SSL
    PyTypeObject* ssl_type = PyObject_New(PyTypeObject, &PyType_Type);
    *ssl_type = c_ssl_type;
    ssl_type->ob_type = &PyType_Type;

    SPyAddGlobalString("SSLType", (PyObject*)ssl_type);
#endif


    PyTypeObject* ap_type = PyObject_New(PyTypeObject, &PyType_Type);
    *ap_type = c_ap_type;
    ap_type->ob_type = &PyType_Type;
    SPyAddGlobalString("APType", (PyObject*)ap_type);


    PyObject *m, *d;
    m = Py_InitModule("e32socket", (PyMethodDef*)socket_methods);
    d = PyModule_GetDict(m);
    
    PyObject *PyGAI_Error;
  
    PySocket_Error = PyErr_NewException("socket.error", NULL, NULL);
    if (PySocket_Error == NULL)
      return;
    PyDict_SetItemString(d, "error", PySocket_Error);
    PyGAI_Error = PyErr_NewException("socket.gaierror", PySocket_Error,
                                     NULL);
    if (PyGAI_Error == NULL)
      return;
      

    PyDict_SetItemString(d, "gaierror", PyGAI_Error);
    PyDict_SetItemString(d, "MSG_WAITALL", PyInt_FromLong(KWaitAll));
    PyDict_SetItemString(d, "MSG_PEEK", PyInt_FromLong(KSockReadPeek));
    PyDict_SetItemString(d, "SO_OOBINLINE", PyInt_FromLong(KSoTcpOobInline));
    PyDict_SetItemString(d, "SO_RCVBUF", PyInt_FromLong(KSORecvBuf));
    PyDict_SetItemString(d, "SO_REUSEADDR", PyInt_FromLong(KSoReuseAddr));
    PyDict_SetItemString(d, "SO_SNDBUF", PyInt_FromLong(KSOSendBuf));
    PyDict_SetItemString(d, "AF_INET", PyInt_FromLong(KAfInet));
    PyDict_SetItemString(d, "AF_BT", PyInt_FromLong(KAfBT));
    PyDict_SetItemString(d, "SOCK_STREAM", PyInt_FromLong(KSockStream));
    PyDict_SetItemString(d, "SOCK_DGRAM", PyInt_FromLong(KSockDatagram));
    PyDict_SetItemString(d, "IPPROTO_TCP", PyInt_FromLong(KProtocolInetTcp));
    PyDict_SetItemString(d, "IPPROTO_UDP", PyInt_FromLong(KProtocolInetUdp));
    PyDict_SetItemString(d, "BTPROTO_RFCOMM", PyInt_FromLong(KRFCOMM));
    PyDict_SetItemString(d, "RFCOMM", PyInt_FromLong(RfcommService));
    PyDict_SetItemString(d, "OBEX", PyInt_FromLong(ObexService));
    PyDict_SetItemString(d, "AUTH", PyInt_FromLong(KAUT));
    PyDict_SetItemString(d, "ENCRYPT", PyInt_FromLong(KENC));
    PyDict_SetItemString(d, "AUTHOR", PyInt_FromLong(KAUTHOR));
    //PyDict_SetItemString(d, "TLS", PyInt_FromLong(KProtocol_TLS));
    //PyDict_SetItemString(d, "SSL", PyInt_FromLong(KProtocol_SSL));
    return;
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason aReason)
{
  return KErrNone;
}
#endif
