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
*  messagingmodule.cpp  
*  
*  Python messaging services on Symbian OS
*
*  Implements currently the following Python classes / methods:
*
*   Messaging
* 
*   sms_rsend(string, unicode_string)
*   mms_send(unicode_string, unicode_string, unicode_string)
*
*  TODO
*   o Add delete of Messaging object to the wrapper and implement the
*     deletion for finer-grained control and advanced API
*   o Expose the Messaging object to the end developer
*   o Adding delete from Outbox into module inbox might be the easiest
*     way to delete an ongoing send request if an ongoing send does not
*     panic the client
*   o Unify and merge inbox and messaging?
* ====================================================================
*/

#include "messaging.h"

/* Dummy destructor for this module */
extern "C" {
  static void mes_dealloc(MES_object *meso)
  {
  }
}


class TMessagingStaticData {
public:
  MES_object *mes_object;
};

/* If DLL TLS is set then fetch and return the value or else create the 
 * messaging object, set the DLL TLS, register the thread exit function and
 * return the value. */
TMessagingStaticData* get_mes_object()
{
  if (Dll::Tls()) {
    return static_cast<TMessagingStaticData*>(Dll::Tls());
  }
  else {
    TInt error = KErrNone;
    TMessagingStaticData* sd = NULL;
    TRAP(error, {
      sd = new (ELeave) TMessagingStaticData;
    });
    if(error!=KErrNone){
      SPyErr_SetFromSymbianOSErr(error);
      return NULL;
    }
    sd->mes_object = PyObject_New(MES_object, MES_type);
    if (sd->mes_object == NULL){
      PyErr_NoMemory();
      return NULL;
    }
    error = Dll::SetTls(sd);
    if(error!=KErrNone){
      delete sd->mes_object;
      delete sd;
      SPyErr_SetFromSymbianOSErr(error);
      return NULL;
    }
    /* Register the thread exit function for mes_object cleanup */
    PyThread_AtExit(delete_mes_object);
    return static_cast<TMessagingStaticData*>(Dll::Tls());
  }
}


extern "C"
void delete_mes_object(void)
{
  TMessagingStaticData* sd = NULL;
  sd = get_mes_object();

  if(sd == NULL)
    return;

  if (sd->mes_object->smsObserver) {
    delete sd->mes_object->smsObserver;
    sd->mes_object->smsObserver = NULL;
  }
  if (sd->mes_object->messaging) {
    delete sd->mes_object->messaging;
    sd->mes_object->messaging = NULL;
  }    
  if (sd->mes_object->callBackSet) {
    Py_XDECREF(sd->mes_object->myCallBack.iCb);
  }
  PyObject_Del(sd->mes_object);
  delete static_cast<TMessagingStaticData *>(Dll::Tls());
  Dll::SetTls(NULL);
}

/*
 * Allocate and send the message.
 */
extern "C" PyObject *
new_mes_object(PyObject* /*self*/, PyObject* args)
{
  char* number;
  char* message;
  char* reciname;

  int number_l;
  int message_l;
  int reciname_l;

  int encoding = TSmsDataCodingScheme::ESmsAlphabet7Bit;
  PyObject* c;
  TInt error = KErrNone;
  MES_object *meso;
  TMessagingStaticData *sd = get_mes_object();
  if (sd == NULL)
    return NULL;
  meso = sd->mes_object;

  if (!PyArg_ParseTuple(args, "s#u#u#iO", &number, &number_l, &message, &message_l, &reciname, &reciname_l, &encoding, &c))  
    return NULL;

  // callback preparations
  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyObject_Del(meso);
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  Py_XINCREF(c);
  
  meso->myCallBack.iCb = c;
  meso->callBackSet = ETrue;

  SmsObserver* sender = new SmsObserver();
  if (!sender) {
    PyObject_Del(meso);
    return PyErr_NoMemory();
  }
  sender->SetCallBack(meso->myCallBack);
  meso->smsObserver = sender;
  // end callback
  
  if ((number_l > MaxTelephoneNumberLength) || (message_l > MaxMessageLength) || (reciname_l > MaxNameLength)) {
    PyObject_Del(meso);
    PyErr_BadArgument();
    return NULL;
  }

  TBuf<MaxTelephoneNumberLength> tel_number;
  tel_number.FillZ(MaxTelephoneNumberLength);
  tel_number.Copy(TPtrC8((TUint8*)number, number_l));
  
  TPtrC msg_body((TUint16*)message, message_l);
  
  TPtrC reci_name((TUint16*)reciname, reciname_l);
  
  Py_BEGIN_ALLOW_THREADS
  TRAP(error, meso->messaging = CSmsSendHandler::NewL(*(meso->smsObserver), tel_number, msg_body, reci_name, encoding));
  Py_END_ALLOW_THREADS

  if (meso->messaging == NULL) {
    PyObject_Del(meso);
    return PyErr_NoMemory();
  }

  if(error != KErrNone){
    PyObject_Del(meso);
    return SPyErr_SetFromSymbianOSErr(error);
  }

  return (PyObject*) meso;
}

/*
 * The following are functions for mms and sms sending in SOS 9.x.
 * 
 * The latter is not in use as the convenience class in S60 does not
 * offer encoding change in sending options. Therefore we have to 
 * revert to the use of older classes.
 *
 */
#ifdef EKA2

#include <rsendas.h> 
#include <rsendasmessage.h>
#include <csendasmessagetypes.h>
#include <senduiconsts.h>

extern "C" PyObject *
messaging_mms_send(PyObject* /*self*/, PyObject* args)
{
  TInt err = KErrNone;
  RSendAs sendServ;
  RSendAsMessage message;
  TRequestStatus status;
  
  PyObject* number = NULL;
  PyObject* content = NULL;
  PyObject* filename = NULL;
  HBufC* attachmentBuf = NULL;
  
  if (!PyArg_ParseTuple(args, "UU|U", &number, &content, &filename)){  
    return NULL;
  }
  
  TPtrC contentPtr((TUint16*) PyUnicode_AsUnicode(content), PyUnicode_GetSize(content));
  TPtrC numberPtr((TUint16*) PyUnicode_AsUnicode(number), PyUnicode_GetSize(number));
  if(NULL!=filename){
    TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename),PyUnicode_GetSize(filename));
    TRAP(err,attachmentBuf = HBufC::NewL(filenamePtr.Length()));
    if(KErrNone != err){
      return SPyErr_SetFromSymbianOSErr(err);
    }
    attachmentBuf->Des().Append(filenamePtr);
  }
  err = sendServ.Connect();
  if(KErrNone != err){
    delete attachmentBuf;
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  TRAP(err,message.CreateL(sendServ,KSenduiMtmMmsUid));
  if(KErrNone != err){
    sendServ.Close();
    delete attachmentBuf;
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  if(NULL!=attachmentBuf){
    message.AddAttachment(attachmentBuf->Des(),status);
    User::WaitForRequest(status);
    TRAP(err,User::LeaveIfError(status.Int()));
    if(KErrNone != err){
      sendServ.Close();
      delete attachmentBuf;
      return SPyErr_SetFromSymbianOSErr(err);
    }
  } 
    
  TRAP(err,{
    message.AddRecipientL(numberPtr, RSendAsMessage::ESendAsRecipientTo); 
    message.SetSubjectL(contentPtr);
    message.SendMessageAndCloseL();
  });
  sendServ.Close();
  delete attachmentBuf;
  if(KErrNone != err){
    return SPyErr_SetFromSymbianOSErr(err);
  }
    
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
messaging_sms_rsend(PyObject* /*self*/, PyObject* args)
{
  TInt err = KErrNone;
  RSendAs sendServ;
  RSendAsMessage message;
  
  PyObject* number = NULL;
  PyObject* content = NULL;
  
  if (!PyArg_ParseTuple(args, "UU", &number, &content)){  
    return NULL;
  }
  
  TPtrC contentPtr((TUint16*) PyUnicode_AsUnicode(content), PyUnicode_GetSize(content));
  TPtrC numberPtr((TUint16*) PyUnicode_AsUnicode(number), PyUnicode_GetSize(number));
    
  err = sendServ.Connect();
  if(KErrNone != err){
    return SPyErr_SetFromSymbianOSErr(err);
  }

  TRAP(err,{
    message.CreateL(sendServ,KSenduiMtmSmsUid);
    message.AddRecipientL(numberPtr,RSendAsMessage::ESendAsRecipientTo); 
    message.SetBodyTextL(contentPtr);
    message.SendMessageAndCloseL();
  });
  sendServ.Close();
  if(KErrNone != err){
    return SPyErr_SetFromSymbianOSErr(err);
  }
    
  Py_INCREF(Py_None);
  return Py_None;
}
#endif /* EKA2 */

//////////////TYPE SET////////////////////////

extern "C" {

  static const PyMethodDef mes_methods[] = {
    //{"sms_send", (PyCFunction)mes_sms_send, METH_VARARGS}, // no methods needed currently
    {NULL,              NULL}           /* sentinel */
  };

  static PyObject *
  mes_getattr(MES_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)mes_methods, (PyObject *)op, name);
  }

  static const PyTypeObject c_mes_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_messaging.Messaging",                   /*tp_name*/
    sizeof(MES_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)mes_dealloc,                  /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)mes_getattr,                 /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };

} /* extern "C" */

//////////////INIT////////////////////////////

extern "C" {

  static const PyMethodDef messaging_methods[] = {
    {"Messaging", (PyCFunction)new_mes_object, METH_VARARGS, NULL},
#ifdef EKA2
    {"mms_send", (PyCFunction)messaging_mms_send, METH_VARARGS, NULL},
    {"sms_rsend", (PyCFunction)messaging_sms_rsend, METH_VARARGS, NULL},
#endif
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initmessaging(void)
  {
    PyTypeObject* mes_type = PyObject_New(PyTypeObject, &PyType_Type);
    *mes_type = c_mes_type;
    mes_type->ob_type = &PyType_Type;

    SPyAddGlobalString("MESType", (PyObject*)mes_type);
  
    PyObject *m, *d;

    m = Py_InitModule("_messaging", (PyMethodDef*)messaging_methods);
    d = PyModule_GetDict(m);
    
    PyDict_SetItemString(d,"EEncoding7bit", PyInt_FromLong(TSmsDataCodingScheme::ESmsAlphabet7Bit));
    PyDict_SetItemString(d,"EEncoding8bit", PyInt_FromLong(TSmsDataCodingScheme::ESmsAlphabet8Bit));
    PyDict_SetItemString(d,"EEncodingUCS2", PyInt_FromLong(TSmsDataCodingScheme::ESmsAlphabetUCS2));

    // states in send
    PyDict_SetItemString(d,"ECreated", PyInt_FromLong(MMsvObserver::ECreated));
    PyDict_SetItemString(d,"EMovedToOutBox", PyInt_FromLong(MMsvObserver::EMovedToOutBox));
    PyDict_SetItemString(d,"EScheduledForSend", PyInt_FromLong(MMsvObserver::EScheduledForSend));
    PyDict_SetItemString(d,"ESent", PyInt_FromLong(MMsvObserver::ESent));
    PyDict_SetItemString(d,"EDeleted", PyInt_FromLong(MMsvObserver::EDeleted));
    
    // errors
    PyDict_SetItemString(d,"EScheduleFailed", PyInt_FromLong(MMsvObserver::EScheduleFailed));
    PyDict_SetItemString(d,"ESendFailed", PyInt_FromLong(MMsvObserver::ESendFailed));
    PyDict_SetItemString(d,"ENoServiceCentre", PyInt_FromLong(MMsvObserver::ENoServiceCentre));
    PyDict_SetItemString(d,"EFatalServerError", PyInt_FromLong(MMsvObserver::EFatalServerError));

  }
  
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif
