/**
 * ====================================================================
 * topwindowmodule.cpp
 * 
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

#include "topwindow.h"



#define TopWindow_type ((PyTypeObject*)SPyGetGlobalString("TopWindowType"))
struct TopWindow_object {
  PyObject_VAR_HEAD
  CTopWindow* topWindow;
};


/*
 * Create new TopWindow object.
 */ 
extern "C" PyObject *
new_TopWindow_object()
{
  TInt err = KErrNone;
  CTopWindow* topWin = NULL;
  
  TRAP(err, {
    topWin = CTopWindow::NewL();
  });
  if(err != KErrNone){
    delete topWin;
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  TopWindow_object* topWindow = 
    PyObject_New(TopWindow_object, TopWindow_type);
  if (topWindow == NULL){
    delete topWin;
    return PyErr_NoMemory();
  }
  topWindow->topWindow = topWin;
     
  return (PyObject*)topWindow;
}


/*
 * Deallocate the key TopWindow object.
 */
extern "C" {
  static void TopWindow_dealloc(TopWindow_object *topWindow)
  {
    delete topWindow->topWindow;
    PyObject_Del(topWindow);
  }
}


/*
 * Create new Window.
 */
extern "C" PyObject *
topwindow_window(PyObject* /*self*/, PyObject* /*args*/)
{
  return new_TopWindow_object();
}


/*
 * Show the window.
 */
extern "C" PyObject *
TopWindow_show(TopWindow_object *self, PyObject* /*args*/)
{    
  TInt err = KErrNone;
  
  TRAP(err, {
    self->topWindow->ShowL();
  });
  if(err != KErrNone){
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Hide the window.
 */
extern "C" PyObject *
TopWindow_hide(TopWindow_object *self, PyObject* /*args*/)
{   
  TInt err = KErrNone;
  
  TRAP(err, { 
    self->topWindow->HideL();
  });
  if(err != KErrNone){
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Set size of the window.
 */
extern "C" PyObject *
TopWindow_set_size(TopWindow_object *self, PyObject* args)
{    
  TInt width = 0;
  TInt height = 0;
   if (!PyArg_ParseTuple(args, "ii", &width, &height)){ 
    return NULL;
  }
    
  TSize size(width, height);
  
  self->topWindow->SetSize(size);
  
  Py_INCREF(Py_None);
  return Py_None; 
}

/*
 * Size of the window.
 */
extern "C" PyObject *
TopWindow_size(TopWindow_object *self, PyObject* /*args*/)
{   
  TSize size = self->topWindow->Size();
  return Py_BuildValue("(ii)", size.iWidth, size.iHeight);
}


/*
 * Maximum size of the window.
 */
extern "C" PyObject *
TopWindow_max_size(TopWindow_object *self, PyObject* /*args*/)
{   
  TSize size = self->topWindow->MaxSize();
  return Py_BuildValue("(ii)", size.iWidth, size.iHeight);
}


/*
 * Set position of the window.
 */
extern "C" PyObject *
TopWindow_set_position(TopWindow_object *self, PyObject* args)
{    
  TInt x = 0;
  TInt y = 0;
   if (!PyArg_ParseTuple(args, "ii", &x, &y)){ 
    return NULL;
  }
  
  
  TPoint pos(x, y);
  
  self->topWindow->SetPosition(pos);
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Set shadow for the window.
 */
extern "C" PyObject *
TopWindow_set_shadow(TopWindow_object *self, PyObject* args)
{    
  TInt shadow = 0;
   if (!PyArg_ParseTuple(args, "i", &shadow)){ 
    return NULL;
  }
 
  self->topWindow->SetShadow(shadow);
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Set corner type for the window.
 */
extern "C" PyObject *
TopWindow_set_corner_type(TopWindow_object *self, PyObject* args)
{    
  TInt cornerType = 0;
   if (!PyArg_ParseTuple(args, "i", &cornerType)){ 
    return NULL;
  }
 
  self->topWindow->SetCornerType(cornerType);
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Insert image into the window.
 */ 
extern "C" PyObject *
TopWindow_put_image(TopWindow_object *self, PyObject* args)
{
  TInt err = KErrNone;
  PyObject* bitmapObj = NULL;
  TUint key = 0;
  TInt x = 0;
  TInt y = 0;
  TInt width = 0;
  TInt height = 0;
  
  if (!PyArg_ParseTuple(args, "Oiiii", &bitmapObj, &x, &y, &width, &height)){
    return NULL;
  }
  TRect rect(x,y,x+width,y+height);
  
  TRAP(err, {
    key = self->topWindow->PutImageL(bitmapObj, rect);
  });
  if(err != KErrNone){
    return SPyErr_SetFromSymbianOSErr(err);
  }

  return Py_BuildValue("i", key);
}


/*
 * Remove image from the window.
 */
extern "C" PyObject *
TopWindow_remove_image(TopWindow_object *self, PyObject* args)
{
  TUint key = 0;
  TInt err = KErrNone;
  
  if (!PyArg_ParseTuple(args, "i", &key)){ 
    return NULL;
  }
 
  err = self->topWindow->RemoveImage(key);
  if(err == KErrNotFound){
    PyErr_SetString(PyExc_IndexError, 
		  "no such image");
		return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}



/*
 * Set background Color.
 */
extern "C" PyObject *
TopWindow_set_bg_color(TopWindow_object *self, PyObject* args)
{    
  TUint32 color = 0;
  
  if (!PyArg_ParseTuple(args, "i", &color)){ 
    return NULL;
  }
  
  TRgb rgb(color); 
  self->topWindow->SetBgColor(rgb);
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Set fading.
 */
extern "C" PyObject *
TopWindow_set_fading(TopWindow_object *self, PyObject* args)
{    
  TInt fading = 0;
  
  if (!PyArg_ParseTuple(args, "i", &fading)){ 
    return NULL;
  }
  
  self->topWindow->SetFading(fading);
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Sends all pending commands in the buffer to the window server.
 */
extern "C" PyObject *
TopWindow_flush(TopWindow_object *self, PyObject* /*args*/)
{
  self->topWindow->Flush();

  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Causes redraw (of the specified area if an area is specified).
 */
extern "C" PyObject *
TopWindow_redraw(TopWindow_object *self, PyObject* args)
{
  TInt x = -1;
  TInt y = -1;
  TInt width = -1;
  TInt height = -1;
  if (!PyArg_ParseTuple(args, "|(iiii)", &x, &y, &width, &height)){ 
    return NULL;
  }
  
  if(x == -1){
    self->topWindow->DoRedraw();
  }else{
    TRect rect(x, y, width, height);
    self->topWindow->DoRedraw(rect);
  }
  
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Pointer to the RWindow.
 */
extern "C" PyObject *
TopWindow_RWindow_pointer(TopWindow_object *self, PyObject* /*args*/)
{
  return PyCObject_FromVoidPtr(self->topWindow->RWindowPtr(), NULL);
}


/*
 * Pointer to the RWsSession.
 */
extern "C" PyObject *
TopWindow_RWsSession_pointer(TopWindow_object *self, PyObject* /*args*/)
{
  return PyCObject_FromVoidPtr(self->topWindow->RWsSessionPtr(), NULL);
}


/*
 * Pointer to the RWindowGroup.
 */
extern "C" PyObject *
TopWindow_RWindowGroup_pointer(TopWindow_object *self, PyObject* /*args*/)
{
  return PyCObject_FromVoidPtr(self->topWindow->RWindowGroupPtr(), NULL);
}


/*
 * Pointer to the CWsScreenDevice.
 */
extern "C" PyObject *
TopWindow_CWsScreenDevice_pointer(TopWindow_object *self, PyObject* /*args*/)
{
  return PyCObject_FromVoidPtr(self->topWindow->ScreenDevicePtr(), NULL);
}


extern "C" { 
  const static PyMethodDef TopWindow_methods[] = {
    {"show", (PyCFunction)TopWindow_show, METH_NOARGS},
    {"hide", (PyCFunction)TopWindow_hide, METH_NOARGS},
    {"set_size", (PyCFunction)TopWindow_set_size, METH_VARARGS}, 
    {"size", (PyCFunction)TopWindow_size, METH_NOARGS}, 
    {"max_size", (PyCFunction)TopWindow_max_size, METH_NOARGS}, 
    {"set_position", (PyCFunction)TopWindow_set_position, METH_VARARGS},
    {"set_shadow", (PyCFunction)TopWindow_set_shadow, METH_VARARGS},
    {"set_corner_type", (PyCFunction)TopWindow_set_corner_type, METH_VARARGS},
    {"put_image", (PyCFunction)TopWindow_put_image, METH_VARARGS},  
    {"remove_image", (PyCFunction)TopWindow_remove_image, METH_VARARGS},  
    {"bg_color", (PyCFunction)TopWindow_set_bg_color, METH_VARARGS},  
    {"fading", (PyCFunction)TopWindow_set_fading, METH_VARARGS},  
    {"flush", (PyCFunction)TopWindow_flush, METH_NOARGS}, 
    {"redraw", (PyCFunction)TopWindow_redraw, METH_VARARGS}, 
    {"_RWindow_pointer", (PyCFunction)TopWindow_RWindow_pointer, METH_VARARGS}, 
    {"_RWsSession_pointer", (PyCFunction)TopWindow_RWsSession_pointer, METH_VARARGS}, 
    {"_RWindowGroup_pointer", (PyCFunction)TopWindow_RWindowGroup_pointer, METH_VARARGS}, 
    {"_CWsScreenDevice_pointer", (PyCFunction)TopWindow_CWsScreenDevice_pointer, METH_VARARGS}, 
    {NULL, NULL}  
  };
  
  static PyObject *
  TopWindow_getattr(TopWindow_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)TopWindow_methods, (PyObject *)op, name);
  }
  
  static const PyTypeObject c_TopWindow_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_topwindow.TopWindow",                   /*tp_name*/
    sizeof(TopWindow_object),                 /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)TopWindow_dealloc,            /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)TopWindow_getattr,           /*tp_getattr*/
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
  


  static const PyMethodDef topwindow_methods[] = {
    {"window", (PyCFunction)topwindow_window, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) inittopwindow(void)
  {
    PyTypeObject* top_window_type = PyObject_New(PyTypeObject, &PyType_Type);
    *top_window_type = c_TopWindow_type;
    top_window_type->ob_type = &PyType_Type;
    SPyAddGlobalString("TopWindowType", (PyObject*)top_window_type);    
    
    PyObject *m, *d;

    m = Py_InitModule("_topwindow", (PyMethodDef*)topwindow_methods);
    d = PyModule_GetDict(m);
        
    PyDict_SetItemString(d,"corner_type_square", PyInt_FromLong(EWindowCornerSquare));
    PyDict_SetItemString(d,"corner_type_corner1", PyInt_FromLong(EWindowCorner1));
    PyDict_SetItemString(d,"corner_type_corner2", PyInt_FromLong(EWindowCorner2));
    PyDict_SetItemString(d,"corner_type_corner3", PyInt_FromLong(EWindowCorner3));
    PyDict_SetItemString(d,"corner_type_corner5", PyInt_FromLong(EWindowCorner5));
  }
} /* extern "C" */


#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif // EKA2



