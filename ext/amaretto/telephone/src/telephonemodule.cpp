/**
 * ====================================================================
 * telephonemodule.cpp
 *
 * Python API to telephone, modified from the "dialer" example 
 * available from the Series 60 SDK
 *
 * Implements currently following Python type:
 * 
 * Phone
 *
 *    open()
 *      opens the line. MUST be called prior dial().  
 *    
 *    close()
 * 
 *    set_number(str_number)
 *      sets the number to call.
 *
 *    dial()
 *      dials the number set.
 *      
 *    hang_up()
 *      hangs up if call in process. "SymbianError: KErrNotReady" returned
 *      if this call is already finished.
 *
 *    The following functions are provided for S60 3rdEd:
 *
 *    incoming_call(cb)
 *      bind callable cb to receive _a_ notification of an incoming call. The
 *      incoming call number in Unicode is given as parameter for callable cb.
 *      The function needs to be called for each notification request.
 *
 *    answer()
 *      answer to phone call. There needs to be a call to 'incoming_call'
 *      to be able to call this function succesfully.
 *
 * If this extension is used in emulator nothing happens. Notice that since
 * the user of the device can also hang-up the phone explicitly s/he might 
 * affect the current status of the call.
 *
 * Calling close() should be safe even if there is a call ongoing.
 *
 * If there is a phone call already going on prior to calling dial() from
 * Python then the earlier call is put on on hold and the new call established.
 *
 * Calling multiple times dial() where e.g. the first call is answered and a
 * line is established results in subsequent calls doing nothing.
 *
 * TODO support for not answering a call
 * TODO focus does not return to Python automatically when hangup occurs
 *      in the middle of phone call
 * TODO speaker on/off could be given as call parameters
 *      
 * Copyright (c) 2005-2007 Nokia Corporation
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

#include "telephone.h"

//////////////TYPE METHODS

/*
 * Deallocate phone.
 */
extern "C" {

  static void phone_dealloc(PHO_object *phoo)
  {
    if (phoo->phone) {
      delete phoo->phone;
      phoo->phone = NULL;
    }
#ifdef EKA2
    if (phoo->callBackSet) {
      Py_XDECREF(phoo->myCallBack.iCb);
    }
#endif /*EKA2*/
    PyObject_Del(phoo);
  }
  
}

/*
 * Allocate phone.
 */
extern "C" PyObject *
new_phone_object(PyObject* /*self*/, PyObject /**args*/)
{
  PHO_object *phoo = PyObject_New(PHO_object, PHO_type);
  if (phoo == NULL)
    return PyErr_NoMemory();

  TRAPD(error, phoo->phone = CPhoneCall::NewL());
  if (phoo->phone == NULL) {
    PyObject_Del(phoo);
    return PyErr_NoMemory();
  }

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

#ifdef EKA2
  phoo->callBackSet = EFalse;
#endif /*EKA2*/

  return (PyObject*) phoo;
}

/*
 * Dial.
 */
extern "C" PyObject *
phone_dial(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;
  
  Py_BEGIN_ALLOW_THREADS
  error = self->phone->Dial();
  Py_END_ALLOW_THREADS

  if(error != KErrNone){
    char* err_string;
    switch (error) {
      case CPhoneCall::ENumberNotSet:
        err_string = "number not set"; 
        break;
      case CPhoneCall::ENotInitialised:
        err_string = "open() not called";
        break;
      case CPhoneCall::EAlreadyCalling:
        err_string = "call in progress, hang up first";
        break;
      default:
        return SPyErr_SetFromSymbianOSErr(error);
    }  
    PyErr_SetString(PyExc_RuntimeError, err_string);
    return NULL;  
  }
    
  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Set number.
 */
extern "C" PyObject *
phone_set_number(PHO_object* self, PyObject *args)
{
  //Parse the passed telephone number (modified from "messagingmodule.cpp"):
  char* number;
  int number_l;

  if (!PyArg_ParseTuple(args, "s#", &number, &number_l))  
    return NULL;
  
  if ((number_l > MaxTelephoneNumberLength)) {
    PyErr_BadArgument();
    return NULL;
  }

  TBuf<MaxTelephoneNumberLength> tel_number;
  tel_number.FillZ(MaxTelephoneNumberLength);
  tel_number.Copy(TPtrC8((TUint8*)number, number_l));

  self->phone->SetNumber(tel_number);

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Initialise.
 */
extern "C" PyObject *
phone_open(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;
  
  Py_BEGIN_ALLOW_THREADS
  error = self->phone->Initialise();
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    if (error == CPhoneCall::EInitialiseCalledAlready) {
      PyErr_SetString(PyExc_RuntimeError, "open() already called");
      return NULL;  
    }
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Close.
 */
extern "C" PyObject *
phone_close(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;

  Py_BEGIN_ALLOW_THREADS
  error = self->phone->UnInitialise();
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    char* err_string;
    switch (error) {
      case CPhoneCall::ENotInitialised:
        err_string = "open() not called"; 
        break;
      case CPhoneCall::EAlreadyCalling:
        err_string = "call in progress, hang up first";
        break;
      default:
        return SPyErr_SetFromSymbianOSErr(error);
    }  
    PyErr_SetString(PyExc_RuntimeError, err_string);
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Hang up.
 */
extern "C" PyObject *
phone_hang_up(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;

  Py_BEGIN_ALLOW_THREADS
  error = self->phone->HangUp();
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    char* err_string;
    switch (error) {
      case CPhoneCall::ENotCallInProgress:
        err_string = "no call to hang up"; 
        break;
      default:
        return SPyErr_SetFromSymbianOSErr(error);
    }  
    PyErr_SetString(PyExc_RuntimeError, err_string);
    return NULL;  
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Bind the call back for incoming call notifications.
 */
#ifdef EKA2
extern "C" PyObject *
phone_incoming_call(PHO_object* self, PyObject *args)
{
  PyObject* c;
  
  if (!PyArg_ParseTuple(args, "O:set_callback", &c))
    return NULL;
  
  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  Py_XINCREF(c);
  
  /* Delete the old call back if this was set */
  if (self->callBackSet) {
      self->phone->Cancel();
      Py_XDECREF(self->myCallBack.iCb);
  }
  
  self->myCallBack.iCb = c;
  self->callBackSet = ETrue;

  Py_BEGIN_ALLOW_THREADS
  self->phone->SetIncomingCallBack(self->myCallBack);
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Answer phone call.
 */
extern "C" PyObject *
phone_answer(PHO_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;

  Py_BEGIN_ALLOW_THREADS
  error = self->phone->Answer();
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    char* err_string;
    switch (error) {
      case CPhoneCall::ENotCallInProgress:
        err_string = "no call to answer"; 
        break;
      default:
        return SPyErr_SetFromSymbianOSErr(error);
    }  
    PyErr_SetString(PyExc_RuntimeError, err_string);
    return NULL;  
  }

  Py_INCREF(Py_None);
  return Py_None;
}
#endif /*EKA2*/

/*
 * Cancel all requests. Note that this should be re-engineered to every
 * operation separately (e.g. CActiveSchedulerWait).
 */
extern "C" PyObject *
phone_cancel(PHO_object* self, PyObject /**args*/)
{
  self->phone->Cancel();
  
  Py_INCREF(Py_None);
  return Py_None;
}

//////////////TYPE SET

extern "C" {

  const static PyMethodDef phone_methods[] = {
    {"dial", (PyCFunction)phone_dial, METH_NOARGS},
    {"set_number", (PyCFunction)phone_set_number, METH_VARARGS},
    {"open", (PyCFunction)phone_open, METH_NOARGS},
    {"close", (PyCFunction)phone_close, METH_NOARGS},
    {"hang_up", (PyCFunction)phone_hang_up, METH_NOARGS},
#ifdef EKA2
    {"incoming_call", (PyCFunction)phone_incoming_call, METH_VARARGS},
    {"answer", (PyCFunction)phone_answer, METH_NOARGS},
#endif
    {"cancel", (PyCFunction)phone_cancel, METH_NOARGS},
    {NULL, NULL}  
  };

  static PyObject *
  phone_getattr(PHO_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)phone_methods, (PyObject *)op, name);
  }

  static const PyTypeObject c_pho_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_telephone.Phone",                        /*tp_name*/
    sizeof(PHO_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)phone_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)phone_getattr,               /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };
} /* extern "C" */ 

//////////////INIT

extern "C" {
  
  static const PyMethodDef telephone_methods[] = {
    {"Phone", (PyCFunction)new_phone_object, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) inittelephone(void)
  {
    PyTypeObject* pho_type = PyObject_New(PyTypeObject, &PyType_Type);
    *pho_type = c_pho_type;
    pho_type->ob_type = &PyType_Type;

    SPyAddGlobalString("PHOType", (PyObject*)pho_type);

    PyObject *m, *d;

    m = Py_InitModule("_telephone", (PyMethodDef*)telephone_methods);
    d = PyModule_GetDict(m);

#ifdef EKA2  
    PyDict_SetItemString(d, "EStatusUnknown", PyInt_FromLong(CTelephony::EStatusUnknown));
    PyDict_SetItemString(d, "EStatusIdle", PyInt_FromLong(CTelephony::EStatusIdle));
    PyDict_SetItemString(d, "EStatusDialling", PyInt_FromLong(CTelephony::EStatusDialling));
    PyDict_SetItemString(d, "EStatusRinging", PyInt_FromLong(CTelephony::EStatusRinging));
    PyDict_SetItemString(d, "EStatusAnswering", PyInt_FromLong(CTelephony::EStatusAnswering));
    PyDict_SetItemString(d, "EStatusConnecting", PyInt_FromLong(CTelephony::EStatusConnecting));
    PyDict_SetItemString(d, "EStatusConnected", PyInt_FromLong(CTelephony::EStatusConnected));
    PyDict_SetItemString(d, "EStatusReconnectPending", PyInt_FromLong(CTelephony::EStatusReconnectPending));
    PyDict_SetItemString(d, "EStatusDisconnecting", PyInt_FromLong(CTelephony::EStatusDisconnecting));
    PyDict_SetItemString(d, "EStatusHold", PyInt_FromLong(CTelephony::EStatusHold));
    PyDict_SetItemString(d, "EStatusTransferring", PyInt_FromLong(CTelephony::EStatusTransferring));
    PyDict_SetItemString(d, "EStatusTransferAlerting", PyInt_FromLong(CTelephony::EStatusTransferAlerting));
#endif /*EKA2*/

  }
} /* extern "C" */  
  
#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif /*EKA2*/
