/**
 * ====================================================================
 * keycapturemodule.cpp
 * Copyright (c) 2006 Nokia Corporation
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
 
 
#include "Python.h"
#include "symbian_python_ext_util.h"

#include <e32base.h>

#include "capturer.h"

#define Capturer_type ((PyTypeObject*)SPyGetGlobalString("CapturerType"))
struct Capturer_object{
  PyObject_VAR_HEAD
  CCapturer* capturer;
};


/*
 * Create new Capturer object.
 */
extern "C" PyObject *
new_Capturer_object(PyObject* callback)
{
  TInt err = KErrNone;
  CCapturer* capt = NULL;
  
  TRAP(err, {
    capt = CCapturer::NewL(callback);
  });
  if(err != KErrNone){
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  Capturer_object* capturer = 
    PyObject_New(Capturer_object, Capturer_type);
  if (capturer == NULL){
    delete capt;
    return PyErr_NoMemory();
  }
  capturer->capturer = capt;
   
  return (PyObject*)capturer;
}


/*
 * Create new key Capturer.
 */
extern "C" PyObject *
keycapture_capturer(PyObject* /*self*/, PyObject *args)
{
  PyObject* callback = NULL;
  if (!PyArg_ParseTuple(args, "|O", &callback))
    return NULL;
  
  if (callback && !PyCallable_Check(callback)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }
  return new_Capturer_object(callback);
}


/*
 * Deallocate the key Capturer object.
 */
extern "C" {
  static void Capturer_dealloc(Capturer_object *capturer)
  {
    delete capturer->capturer;
    PyObject_Del(capturer);
  }
}


/*
 * Start capturing defined keys.
 */
extern "C" PyObject *
Capturer_start(Capturer_object *self)
{
  Py_BEGIN_ALLOW_THREADS
  self->capturer->StartCapturing();
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Stop capturing.
 */
extern "C" PyObject *
Capturer_stop(Capturer_object *self)
{
  self->capturer->Cancel();
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Set key to be captured (key code).
 */
extern "C" PyObject *
Capturer_set_key(Capturer_object *self, PyObject* args)
{
  TInt32 keyCode;
  TInt32 keyId;
  if (!PyArg_ParseTuple(args, "i", &keyCode)){ 
    return NULL;
  }
  TRAPD(err, keyId = self->capturer->SetKeyToBeCapturedL(keyCode));
  if(err!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(err);
  }
  return Py_BuildValue("i", keyId);
}


/*
 * Remove key (do not capture this key anymore).
 */
extern "C" PyObject *
Capturer_remove_key(Capturer_object *self, PyObject* args)
{
  TInt32 keyId;
  if (!PyArg_ParseTuple(args, "i", &keyId)){ 
    return NULL;
  }
  self->capturer->RemoveKey(keyId);
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Get last captured key.
 */
extern "C" PyObject *
Capturer_last_captured_key(Capturer_object *self)
{
  return Py_BuildValue("i", self->capturer->LastCapturedKey());
}


/*
 * Set key forwarding.
 */
extern "C" PyObject *
Capturer_set_forwarding(Capturer_object *self, PyObject* args)
{
  TInt32 forwarding = 0;
  if (!PyArg_ParseTuple(args, "i", &forwarding)){ 
    return NULL;
  }
  
  self->capturer->SetForwarding(forwarding);
    
  Py_INCREF(Py_None);
  return Py_None; 
}



extern "C" {
  
  const static PyMethodDef Capturer_methods[] = {
    {"start", (PyCFunction)Capturer_start, METH_NOARGS},
    {"stop", (PyCFunction)Capturer_stop, METH_NOARGS},
    {"key", (PyCFunction)Capturer_set_key, METH_VARARGS}, 
    {"remove_key", (PyCFunction)Capturer_remove_key, METH_VARARGS}, 
    {"last_key", (PyCFunction)Capturer_last_captured_key, METH_NOARGS}, 
    {"set_forwarding", (PyCFunction)Capturer_set_forwarding, METH_VARARGS}, 
    {NULL, NULL}  
  };
  
  static PyObject *
  Capturer_getattr(Capturer_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)Capturer_methods, (PyObject *)op, name);
  }
  
  static const PyTypeObject c_Capturer_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_keycapture.Capturer",                      /*tp_name*/
    sizeof(Capturer_object),                  /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)Capturer_dealloc,             /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)Capturer_getattr,            /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    0,                                        /*tp_iter */
  };
  

  static const PyMethodDef keycapture_methods[] = {
    {"capturer", (PyCFunction)keycapture_capturer, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initkeycapture(void)
  {
    PyTypeObject* capturer_type = PyObject_New(PyTypeObject, &PyType_Type);
    *capturer_type = c_Capturer_type;
    capturer_type->ob_type = &PyType_Type;
    SPyAddGlobalString("CapturerType", (PyObject*)capturer_type);    
    
    Py_InitModule("_keycapture", (PyMethodDef*)keycapture_methods);
  }
} /* extern "C" */


#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif
