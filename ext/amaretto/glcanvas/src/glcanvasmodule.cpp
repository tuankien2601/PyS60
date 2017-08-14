/*
 * Copyright (c) 2006-2009 Nokia Corporation
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
 */

#include "glcanvas.h"

/* EGL 1.1 enumeration */
#ifndef EGL_VERSION_1_1
#define EGL_CONTEXT_LOST	       0x300E
#define EGL_BIND_TO_TEXTURE_RGB        0x3039
#define EGL_BIND_TO_TEXTURE_RGBA       0x303A
#define EGL_MIN_SWAP_INTERVAL	       0x303B
#define EGL_MAX_SWAP_INTERVAL	       0x303C
#define EGL_BACK_BUFFER		       0x3084
#endif

/**
 * Implementation of GLCanvas
 *
 * A big part of the implementation has been copied from
 * the original Canvas object implementation in
 * appui/appuifw/appuifwmodule.cpp
 *
 */

CGLCanvas::CGLCanvas(PyObject *aDrawCallback, PyObject *aEventCallback, PyObject *aResizeCallback) {
    iDrawCallback  = aDrawCallback;
    iEventCallback = aEventCallback;
    iResizeCallback = aResizeCallback;
    iOpenGLInitialized = 0;
}

void CGLCanvas::ConstructL(const TRect& aRect, const CCoeControl* aParent, CAmarettoAppUi* aAppui) {
  CreateWindowL(aParent);
  if(TouchEnabled()){
    EnableDragEvents();
  }
  myParent = aParent;
  myAppui = aAppui;
  SetRect(aRect);
  ActivateL();
#ifdef DEBUG_GLCANVAS
  DEBUGMSG2("aRect Size: %d x %d", aRect.Size().iWidth, aRect.Size().iHeight);
  DEBUGMSG2("aRect PosX: %d PosY: %d", aRect.iTl.iX, aRect.iTl.iY);
#endif
}

void CGLCanvas::HandlePointerEventL(const TPointerEvent& aPointerEvent){
PyObject *arg=NULL, *ret=NULL;

  if( !TouchEnabled() ) {
    return;
  }
  /* All basic touch events cause this callback function to be hit before the
   * container's HandlePointerEventL function. This is because the GLCanvas is
   * above the Container and GLCanvas is a window-owning control as it derives
   * from CCoeControl and is created using CreateWindow. To call any callback
   * functions registered by the user using the bind() API, we have to call
   * the Container's callback function and this is done explicitly using 
   * myParent below. 
   * For advanced touch events like EDrag, EnableDragEvents is called and
   * as a result of this the CCoeControl::HandlePointerEventL base class call
   * in CAmarettoContainer::HandlePointerEventL in turn calls this function. 
   * The application crashes as a result of the inifinte loop. 
   * To get around this we disable and enable the CCoeControl base class call
   * in the Container class. */
  myAppui->DisablePointerForwarding(ETrue);
  CONST_CAST(CCoeControl*, myParent)->HandlePointerEventL(aPointerEvent);
  myAppui->DisablePointerForwarding(EFalse);

  if (!iEventCallback)
    return;
  arg = Py_BuildValue("({s:i, s:(ii), s:i})", "type", aPointerEvent.iType
        + 0x101, "pos", aPointerEvent.iPosition.iX,
        aPointerEvent.iPosition.iY, "modifiers", aPointerEvent.iModifiers);
  if (!arg) {
    PyErr_Print();
    return;
  }
  PyGILState_STATE gstate = PyGILState_Ensure();
  ret = PyEval_CallObject(iEventCallback, arg);
  Py_DECREF(arg);
  if (!ret)
    PyErr_Print();
  else
    Py_DECREF(ret);
  PyGILState_Release(gstate);
}

CGLCanvas::~CGLCanvas() {
#ifdef DEBUG_GLCANVAS
  DEBUGMSG("CGLCanvas destructor");
#endif
  eglMakeCurrent( iEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT );
  eglDestroySurface( iEglDisplay, iEglSurface );
  eglDestroyContext( iEglDisplay, iEglContext );
  // Release resources associated
  // with EGL and OpenGL ES
  eglTerminate( iEglDisplay );
}

TKeyResponse CGLCanvas::OfferKeyEventL(const TKeyEvent& aKeyEvent, 
				      TEventCode aType) {
  PyObject *arg=NULL, *ret=NULL;
  CCoeControl::OfferKeyEventL(aKeyEvent,aType);
  if (!iEventCallback) 
    return EKeyWasNotConsumed;
  PyGILState_STATE gstate = PyGILState_Ensure();
  arg=Py_BuildValue("({s:i,s:i,s:i,s:i})",
        "type",aType,
        "keycode",aKeyEvent.iCode,
        "scancode", aKeyEvent.iScanCode, 
        "modifiers", aKeyEvent.iModifiers);
  if (!arg) 
    goto error;
  ret=PyEval_CallObject(iEventCallback,arg);
  Py_DECREF(arg);
  if (!ret) 
    goto error;
  Py_DECREF(ret);
  PyGILState_Release(gstate);
  return EKeyWasNotConsumed;
error:
  PyErr_Print();
  PyGILState_Release(gstate);
  return EKeyWasNotConsumed;
}

/**
 * Creates a new EGL rendering context on screen. Sets a Python exception if
 * the operation fails.
 *
 * @param attrib_list An array of desired attributes.
 * @return 1 on success, 0 on failure.
 */
int CGLCanvas::CreateContext(EGLint *attrib_list) {
#ifdef DEBUG_GLCANVAS
  DEBUGMSG("CGLCanvas::CreateContext\n");
#endif
  assert(attrib_list != NULL);
  
  // Initialize frame counter
  iFrame = 0;
  
  // Describes the format, type and 
  // size of the color buffers and
  // ancillary buffers for EGLSurface
  EGLConfig Config;                        
  
  // Get the display for drawing graphics
  iEglDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );
  if ( iEglDisplay == NULL )
      {
      PyErr_Format(PyExc_RuntimeError, "eglGetDisplay failed [Errno %d]", eglGetError());
      return NULL;
      }

  // Initialize display 
  if ( eglInitialize( iEglDisplay, NULL, NULL ) == EGL_FALSE )
      {
      PyErr_Format(PyExc_RuntimeError, "eglInitialize [Errno %d, disp: %x]", eglGetError(), iEglDisplay);
      return NULL;
      }
  
  // Pointer for EGLConfigs
  EGLConfig *configList = NULL;
  EGLint numOfConfigs = 0;
  EGLint configSize   = 0;
  // Get the number of possible EGLConfigs
  if ( eglGetConfigs( iEglDisplay, configList, configSize, &numOfConfigs ) 
      == EGL_FALSE )
      {
      PyErr_Format(PyExc_ValueError, "eglGetConfigs failed [Errno %d]", eglGetError());
      return NULL;
      }
  configSize = numOfConfigs;
  
  // Allocate memory for the configList
  configList = (EGLConfig*) glcanvas_alloc( sizeof(EGLConfig)*configSize );
  if ( configList == NULL )
      {
      PyErr_SetString(PyExc_ValueError, "Config alloc failed");
      return NULL;
      }

  /* Define properties for the wanted EGLSurface.
     To get the best possible performance, choose
     an EGLConfig with a buffersize matching
     the current window's display mode*/
  
  // Choose an EGLConfig that best matches to the properties in attrib_list 
  if ( eglChooseConfig( iEglDisplay, attrib_list, configList, configSize, 
                       &numOfConfigs ) == EGL_FALSE )
      {
      glcanvas_free(configList);
      PyErr_Format(PyExc_TypeError, "No matching configuration found. [Errno %d]", eglGetError());
      return NULL;
      }
  
  // Choose the best EGLConfig. EGLConfigs 
  // returned by eglChooseConfig are sorted so 
  // that the best matching EGLConfig is first in
  // the list.
  Config = configList[0];
  
  // Free configList, not used anymore.
  glcanvas_free( configList );
  
  // Create a window where the graphics are blitted 
  iEglSurface = eglCreateWindowSurface( iEglDisplay, Config, &Window(), NULL );
  if ( iEglSurface == NULL )
      {
      PyErr_Format(PyExc_RuntimeError, "eglCreateWindowSurface failed [Errno %d]", eglGetError());
      return NULL;
      }
  
  // Create a rendering context 
  iEglContext = eglCreateContext( iEglDisplay, Config, EGL_NO_CONTEXT, NULL );
  if ( iEglContext == NULL )
      {
      PyErr_Format(PyExc_RuntimeError, "eglCreateContext failed [Errno %d]", eglGetError());
      return NULL;
      }
  /* Make the context current. Binds context to the current rendering thread
     and surface.*/
  if ( eglMakeCurrent( iEglDisplay, iEglSurface, iEglSurface, iEglContext ) 
      == EGL_FALSE )
      {
      PyErr_Format(PyExc_RuntimeError, "eglMakeCurrent failed [Errno %d]", eglGetError());
      return NULL;
      }
  iOpenGLInitialized = 1;
  return 1;
}

void CGLCanvas::makeCurrent() {
  /* Make the context current. Binds context to the current rendering thread
  and surface.*/
  if ( eglMakeCurrent( iEglDisplay, iEglSurface, iEglSurface, iEglContext ) 
      == EGL_FALSE )
      {
      PyErr_SetString(PyExc_RuntimeError, "eglMakeCurrent failed");
      return;
      }
  return;
}

PyObject *CGLCanvas::GetRedrawCallBack() {
  return iDrawCallback;
}

PyObject *CGLCanvas::GetEventCallBack() {
  return iEventCallback;
}

PyObject *CGLCanvas::GetResizeCallBack() {
  return iResizeCallback;
}

void CGLCanvas::SetRedrawCallBack(PyObject *cb) {
  if(PyCallable_Check(cb) ) {
    Py_XINCREF(cb);
  } else if(cb == Py_None) {
    cb = NULL;
  } else {
    PyErr_SetString(PyExc_TypeError, "Callable of None expected");
    return;
  }
  
  if(iDrawCallback) {
    Py_XDECREF(iDrawCallback);
  }
  iDrawCallback = cb;
  return;
}

void CGLCanvas::SetEventCallBack(PyObject *cb) {
  if(PyCallable_Check(cb) ) {
    Py_XINCREF(cb);
  } else if(cb == Py_None) {
    cb = NULL;
  } else {
    PyErr_SetString(PyExc_TypeError, "Callable of None expected");
    return;
  }
  
  if(iEventCallback) {
    Py_XDECREF(iEventCallback);
  }
  iEventCallback = cb;
  return;
}

void CGLCanvas::SetResizeCallBack(PyObject *cb) {
  if(PyCallable_Check(cb) ) {
    Py_XINCREF(cb);
  } else if(cb == Py_None) {
    cb = NULL;
  } else {
    PyErr_SetString(PyExc_TypeError, "Callable of None expected");
    return;
  }
  
  if(iResizeCallback) {
    Py_XDECREF(iResizeCallback);
  }
  iResizeCallback = cb;
  return;
}

/**
 * Returns the buffer size for current display mode
 *
 * @return Buffer size in bits
 */
int CGLCanvas::GetBufferSize() {
  TDisplayMode DMode = Window().DisplayMode();
  int BufferSize = 0;
#ifdef DEBUG_GLCANVAS
  DEBUGMSG1("DMode = %x", DMode);
#endif
  switch ( DMode ) {
    case(EColor4K):
      BufferSize = 12;
      break;
    case(EColor64K):
      BufferSize = 16;
      break;
    case(EColor16M):
      BufferSize = 24;
      break;
    #if SERIES60_VERSION >= 28
    case(EColor16MA):
      BufferSize = 32;
      break; 
    #endif
    case(EColor16MU):
      BufferSize = 32;
      break;
    default:
      return -1;
  }
  return BufferSize;
}
/* FIXME This method is declared const so it should not alter the
   state of the object, but arbitrary code execution is allowed in the
   Python callback. This could lead to trouble if the GLCanvas is
   altered there. Maybe we should switch to double buffering completely or 
   invoke the callback via a callgate? */
void CGLCanvas::Draw(const TRect& /*aRect*/) const {     
#ifdef DEBUG_GLCANVAS
  DEBUGMSG("GLCanvas::Draw");
  DEBUGMSG1("Frame: %d", iFrame);
#endif

// This fails on the emulator
#ifndef __WINS__
  redraw();
#endif


}

void CGLCanvas::redraw() const {
  if( !iOpenGLInitialized ) {
    return;
  }
  PyGILState_STATE gstate;
  if (iDrawCallback) {
    gstate = PyGILState_Ensure();
    PyObject *frame = Py_BuildValue("(i)", iFrame);
    app_callback_handler(iDrawCallback,frame);
    PyGILState_Release(gstate);
  }
  
  eglSwapBuffers( this->iEglDisplay, this->iEglSurface );
    // This keeps the backlight turned on when the rendering is continuous
  if ( !(iFrame%50) ) {
    User::ResetInactivityTime();
    User::After(0);
#ifdef DEBUG_GLCANVAS
    DEBUGMSG("User::ResetinactivityTime()");
    DEBUGMSG("User::After(0)");
#endif
  }
}

void CGLCanvas::SizeChanged() { 
#ifdef DEBUG_GLCANVAS
  DEBUGMSG("GLCanvas::SizeChanged()\n");
  TSize size = Size();
  TPoint position = Position();
  DEBUGMSG2("New size: %d x %d", size.iWidth, size.iHeight);
  DEBUGMSG2("New posX: %d posY: %d", position.iX, position.iY);
#endif
  if( !iOpenGLInitialized ) {
    return;
  }
  PyGILState_STATE gstate;
  if (iResizeCallback) {
    gstate = PyGILState_Ensure();
    app_callback_handler(iResizeCallback,NULL);
    PyGILState_Release(gstate);
  }
}

#define ADD_EGL_ATTRIB_VALUE(ptr,param,value) do { *ptr=param; ptr++; *ptr=value; ptr++; } while(0)
#define TERMINATE_EGL_ATTRIB_LIST(ptr) do { *ptr=EGL_NONE; } while(0)

EGLint *CGLCanvas::GenerateEglAttributes(PyObject *options) {
  PyObject *optlist = NULL;
  int optsize = 0;
  EGLint *attrib_list = NULL;
  EGLint *attrib_list_item = NULL;
  int i = 0;
#ifdef DEBUG_GLCANVAS
  DEBUGMSG("GLCanvas_egl_attrs()\n");
#endif
  
  if(!PyDict_Check(options) ) {
    PyErr_SetString(PyExc_TypeError, "Expecting a dict");
    return NULL;
  }
  
  // See if the user wants to specify a custom buffer size
  if(PyDict_GetItem(options, PyInt_FromLong(EGL_BUFFER_SIZE)) == NULL ) {
    PyDict_SetItem(options,Py_BuildValue("i", EGL_BUFFER_SIZE), Py_BuildValue("i", GetBufferSize()) );
  }
  
  // See if the user wants to specify a custom buffer size
  if(PyDict_GetItem(options, PyInt_FromLong(EGL_DEPTH_SIZE)) == NULL ) {
    PyDict_SetItem(options, Py_BuildValue("i", EGL_DEPTH_SIZE), Py_BuildValue("i", GLCANVAS_DEFAULT_DEPTH_SIZE));
  }
  
  // Build an array from the dictionary items
  optlist = PyDict_Items(options);
  optsize = PyList_Size(optlist);
  
  attrib_list = (EGLint*)glcanvas_alloc( (sizeof(EGLint) * ((optsize)*2)) + 1 );
  if(attrib_list == NULL) {
    return (EGLint*)PyErr_NoMemory();
  }
  
  // Take a copy of the array pointer, we'll be incrementing this while adding new values.
  attrib_list_item = attrib_list;
  
  for(i = 0; i < optsize; i++) {
    PyObject *temp_item = PyList_GetItem(optlist, i);
    int ycount;
    
    // temp_item should be a tuple of 2 values (as usual with items from dictionaries)
    assert(PyTuple_Check(temp_item));
    ycount = PyTuple_Size(temp_item);
    
    assert(ycount == 2);
    
    PyObject *item1 = PyTuple_GetItem(temp_item, 0);
    PyObject *item2 = PyTuple_GetItem(temp_item, 1);
    
    if( !PyInt_Check(item1) || !PyInt_Check(item2) ) {
      PyErr_SetString(PyExc_TypeError, "Expecting an integer");
      glcanvas_free(attrib_list);
      return NULL;
    }
    ADD_EGL_ATTRIB_VALUE(attrib_list_item,(EGLint)PyInt_AsLong(item1),(EGLint)PyInt_AsLong(item2));
  }
  
  // Add the terminator
  TERMINATE_EGL_ATTRIB_LIST(attrib_list_item);
  return attrib_list;
}

extern "C" PyObject *new_GLCanvas_object(PyObject */*self*/, PyObject *args, PyObject *keywds) {
  PyObject *draw_cb=NULL;
  PyObject *event_cb=NULL;
  PyObject *resize_cb=NULL;
  PyObject *egl_attrs=NULL;
  GLCanvas_object *op=NULL;
  PyTypeObject *GLCanvas_type=NULL;
  CGLCanvas *gc=NULL;
  EGLint *egl_attrs_tbl=NULL;
#ifdef DEBUG_GLCANVAS
  DEBUGMSG("new_GLCanvas_object\n");
#endif
  
  GLCanvas_type = ((PyTypeObject*)SPyGetGlobalString("GLCanvasType"));
  
  static char *kwlist[] = {"redraw_callback", "event_callback", "resize_callback", "attributes", NULL};
  
  if( !PyArg_ParseTupleAndKeywords(args, keywds, "O|OOO", kwlist, &draw_cb, &event_cb, &resize_cb, &egl_attrs)) {
    return NULL;
  }
  
  if ((draw_cb && !PyCallable_Check(draw_cb) && draw_cb != Py_None) ) {
    PyErr_SetString(PyExc_TypeError, "callable or None expected");
    return NULL;
  }
  
  if ((event_cb && !PyCallable_Check(event_cb) && event_cb != Py_None) ) {
    PyErr_SetString(PyExc_TypeError, "callable or None expected");
    return NULL;
  }
  
  if ((resize_cb && !PyCallable_Check(resize_cb) && resize_cb != Py_None) ) {
    PyErr_SetString(PyExc_TypeError, "callable or None expected");
    return NULL;
  }
  
  if (draw_cb == Py_None) {
    draw_cb=NULL;
  }
  
  if (event_cb == Py_None) {
    event_cb=NULL;
  }
  
  if (resize_cb == Py_None) {
    resize_cb=NULL;
  }
  
  // Create the Python object instance.
  op = PyObject_New(GLCanvas_object, GLCanvas_type);
  
  if (!op) {
    return PyErr_NoMemory();
  }
  
  CAmarettoAppUi* appui = MY_APPUI;
  
  op->ob_event_bindings = NULL;
  
  op->ob_drawcallback  = draw_cb;
  op->ob_eventcallback = event_cb;
  op->ob_resizecallback = resize_cb;
  Py_XINCREF(op->ob_drawcallback);
  Py_XINCREF(op->ob_eventcallback);
  Py_XINCREF(op->ob_resizecallback);
  op->control_name = EGLCanvas;
#ifdef DEBUG_GLCANVAS
  DEBUGMSG1("GLCanvas Draw Callback at %x", draw_cb);
  DEBUGMSG1("GLCanvas Event Callback at %x", event_cb);
  DEBUGMSG1("GLCanvas Resize Callback at %x", event_cb);
#endif
  gc = new CGLCanvas(op->ob_drawcallback, op->ob_eventcallback, op->ob_resizecallback);
  gc->ConstructL(appui->ClientRect(), appui->iContainer, appui);
  if(!gc) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }
  op->ob_control = gc;
  
  if (!op->ob_control) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }
  if(egl_attrs == NULL || egl_attrs == Py_None) {
    // Create an empty dictionary for egl_attrs
    // No need to incref since we won't be needing it after this function ends
    egl_attrs = PyDict_New();
  }
  
  egl_attrs_tbl = op->ob_control->GenerateEglAttributes(egl_attrs);
  if(egl_attrs_tbl == NULL) {
    assert(PyErr_Occurred() != NULL);
    return NULL;
  }
  // Create the rendering context
  op->ob_control->CreateContext(egl_attrs_tbl);
  glcanvas_free(egl_attrs_tbl);
  if(PyErr_Occurred() != NULL) {
    delete op->ob_control;
    op->ob_control = NULL;
    PyObject_Del(op);
    return NULL;
  }
  
  return (PyObject*) op;
}

extern "C" PyObject *GLCanvas_drawNow(GLCanvas_object *self, PyObject */*args*/) {
  CGLCanvas *op = self->ob_control;
#ifdef DEBUG_GLCANVAS
  DEBUGMSG("GLCanvas_drawNow()\n");
#endif
#ifdef __WINS__
  if (op->iFrame == 0) {
      /* Workaround for bug #1653058. Sleep here to give DSA a bit of time to settle and not crash 
       * when the rendering starts (affects at least SDK 3.0MR when starting in fullscreen mode) */
      PyRun_SimpleString("import e32;e32.ao_yield()");
  }
#endif
  op->redraw();

  // Increment the frame count. It must be done here since the Draw() method
  // is declared as const.
  op->iFrame++;
  if(PyErr_Occurred() != NULL) {
#ifdef DEBUG_GLCANVAS
    DEBUGMSG("Got an exception during callback!");
#endif
    return NULL;
  }
  RETURN_PYNONE;
}

extern "C" PyObject *GLCanvas_makeCurrent(GLCanvas_object *self, PyObject */*args*/) {
  CGLCanvas *op = self->ob_control;
  op->makeCurrent();
  RETURN_PYNONE;
}

extern "C" PyObject * GLCanvas_bind(GLCanvas_object *self, PyObject* args) {
  int key_code=0, x1=0, x2=0, y1=0, y2=0;
  PyObject* c;
  
  if (!PyArg_ParseTuple(args, "iO|((ii)(ii))", &key_code, &c, &x1, &y1, &x2, &y2))
    return NULL;
  
  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }
  
  if (!(self->ob_event_bindings))
    if (!(self->ob_event_bindings = new CAppuifwEventBindingArray))
      return PyErr_NoMemory();

  SAppuifwEventBinding bind_info;
  /* These are unique hex values assigned to the Pointer Events. These are
   * defined in key_codes.py */
  if (key_code >= 0x101 && key_code <= 0x10A) {
    bind_info.iType = SAmarettoEventInfo::EPointer;
    memset(&bind_info.iKeyEvent, 0, sizeof(bind_info.iKeyEvent));
    bind_info.iPointerEvent.iType = TPointerEvent::TType(key_code - 0x101);
    bind_info.iPointerEvent.iModifiers = 0;
    bind_info.iPointerRect.SetRect(x1, y1, x2, y2);
  }
  else {
    bind_info.iType = SAmarettoEventInfo::EKey;
    memset(&bind_info.iPointerEvent, 0, sizeof(bind_info.iPointerEvent));
    bind_info.iPointerEvent.iType = TPointerEvent::TType(-1);
    bind_info.iKeyEvent.iCode = key_code;
    bind_info.iKeyEvent.iModifiers = 0;
  }
  bind_info.iCb = c;
  Py_XINCREF(c);

  TRAPD(error, self->ob_event_bindings->InsertEventBindingL(bind_info));

  if (error != KErrNone)
    Py_XDECREF(c);

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" {
  static void GLCanvas_dealloc(GLCanvas_object *op) {
#ifdef DEBUG_GLCANVAS
    DEBUGMSG("GLCanvas_dealloc\n");
#endif
    if(op->ob_drawcallback) {
      Py_XDECREF(op->ob_drawcallback);
      op->ob_drawcallback=NULL;
    }
    
    if(op->ob_eventcallback) {
      Py_XDECREF(op->ob_eventcallback);
      op->ob_eventcallback=NULL;
    }
    
    if(op->ob_resizecallback) {
      Py_XDECREF(op->ob_resizecallback);
      op->ob_resizecallback=NULL;
    }
    
    delete op->ob_event_bindings;
    op->ob_event_bindings = NULL;
    
    if(op->ob_control) {
      delete op->ob_control;
      op->ob_control = NULL;
    }
    
    PyObject_Del(op);
  }
  
  const static PyMethodDef GLCanvas_methods[] = {
    {"bind", (PyCFunction)GLCanvas_bind, METH_VARARGS, NULL},
    {"drawNow", (PyCFunction)GLCanvas_drawNow, METH_NOARGS, NULL},
    {"makeCurrent", (PyCFunction)GLCanvas_makeCurrent, METH_NOARGS, NULL},
    
    {NULL,              NULL}           // sentinel
  };

  static PyObject * GLCanvas_getattr(GLCanvas_object *op, char *name) {
    CGLCanvas *glcanvas=op->ob_control;
    PyObject *ret=NULL;
    if (!strcmp(name,"size")) {
      TSize size=glcanvas->Size();      
      return Py_BuildValue("(ii)",size.iWidth,size.iHeight);
    }
    if (!strcmp(name, UICONTROLAPI_NAME)) {
      Py_INCREF(op);
      return PyCObject_FromVoidPtr(op,_uicontrolapi_decref);
    }
    
    if (!strcmp(name,"redraw_callback")) {
     ret = glcanvas->GetRedrawCallBack();
     if(ret == NULL) {
      RETURN_PYNONE;
     }
     Py_INCREF(ret);
     return ret;
    }
    
    if (!strcmp(name,"event_callback")) {
      ret = glcanvas->GetEventCallBack();
      if(ret == NULL) {
        RETURN_PYNONE;
      }
      Py_INCREF(ret);
      return ret;
    }
    
    if (!strcmp(name,"resize_callback")) {
      ret = glcanvas->GetResizeCallBack();
      if(ret == NULL) {
        RETURN_PYNONE;
      }
      Py_INCREF(ret);
      return ret;
    }
    
    return Py_FindMethod((PyMethodDef*)GLCanvas_methods,
                         (PyObject*)op, name);
  }
  
  static int GLCanvas_setattr(GLCanvas_object *op, char *name, PyObject *v) {
    CGLCanvas *glcanvas=op->ob_control;
    if (!strcmp(name, "redraw_callback")) {
      glcanvas->SetRedrawCallBack(v);
    } else if (!strcmp(name, "event_callback")) {
      glcanvas->SetEventCallBack(v);
    } else if (!strcmp(name, "resize_callback")) {
      glcanvas->SetResizeCallBack(v);
    } else {
      PyErr_SetString(PyExc_AttributeError, "no such attribute");
      return -1;
    }
    
    if (PyErr_Occurred() != NULL) {
      return -1;
    }
    return 0;
  }
  
  static const PyTypeObject c_GLCanvas_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "GLCanvas",                                /*tp_name*/
    sizeof(GLCanvas_object),                   /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)GLCanvas_dealloc,              /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)GLCanvas_getattr,             /*tp_getattr*/
    (setattrfunc)GLCanvas_setattr,             /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };
}       // extern "C"

//////////////INIT////////////////////////////

extern "C" {
  static const PyMethodDef _glcanvas_methods[] = {
    {"GLCanvas",      (PyCFunction)new_GLCanvas_object, METH_VARARGS|METH_KEYWORDS, NULL},
    
    {NULL,              NULL}           /* sentinel */
  };
  
  DL_EXPORT(void) zfinalizeglcanvas(void) {
#ifdef DEBUG_GLCANVAS
    DEBUGMSG("GLCanvas Finalized!");
#endif
  }
  
  DL_EXPORT(void) initglcanvas(void) {
    PyObject *m, *d;
    
    PyTypeObject* glcanvas_type;
    
    glcanvas_type = PyObject_New(PyTypeObject, &PyType_Type);
    *glcanvas_type = c_GLCanvas_type;
    glcanvas_type->ob_type = &PyType_Type;
    SPyAddGlobalString("GLCanvasType", (PyObject*)glcanvas_type);
    
    m = Py_InitModule("glcanvas", (PyMethodDef*)_glcanvas_methods);
    
    d = PyModule_GetDict(m);
    PyDict_SetItemString(d, "EGL_DEFAULT_DISPLAY", PyInt_FromLong(EGL_DEFAULT_DISPLAY));
    PyDict_SetItemString(d, "EGL_NO_CONTEXT", PyInt_FromLong(EGL_NO_CONTEXT));
    PyDict_SetItemString(d, "EGL_NO_DISPLAY", PyInt_FromLong(EGL_NO_DISPLAY));
    PyDict_SetItemString(d, "EGL_NO_SURFACE", PyInt_FromLong(EGL_NO_SURFACE));
//     PyDict_SetItemString(d, "EGL_VERSION_1_0", PyInt_FromLong(EGL_VERSION_1_0));
//     PyDict_SetItemString(d, "EGL_VERSION_1_1", PyInt_FromLong(EGL_VERSION_1_1));
    PyDict_SetItemString(d, "EGL_CONTEXT_LOST", PyInt_FromLong(EGL_CONTEXT_LOST));
    PyDict_SetItemString(d, "EGL_BIND_TO_TEXTURE_RGB", PyInt_FromLong(EGL_BIND_TO_TEXTURE_RGB));
    PyDict_SetItemString(d, "EGL_BIND_TO_TEXTURE_RGBA", PyInt_FromLong(EGL_BIND_TO_TEXTURE_RGBA));
    PyDict_SetItemString(d, "EGL_MIN_SWAP_INTERVAL", PyInt_FromLong(EGL_MIN_SWAP_INTERVAL));
    PyDict_SetItemString(d, "EGL_MAX_SWAP_INTERVAL", PyInt_FromLong(EGL_MAX_SWAP_INTERVAL));
    PyDict_SetItemString(d, "EGL_BACK_BUFFER", PyInt_FromLong(EGL_BACK_BUFFER));

    PyDict_SetItemString(d, "EGL_FALSE", PyInt_FromLong(EGL_FALSE));
    PyDict_SetItemString(d, "EGL_TRUE", PyInt_FromLong(EGL_TRUE));
    PyDict_SetItemString(d, "EGL_SUCCESS", PyInt_FromLong(EGL_SUCCESS));
    PyDict_SetItemString(d, "EGL_NOT_INITIALIZED", PyInt_FromLong(EGL_NOT_INITIALIZED));
    PyDict_SetItemString(d, "EGL_BAD_ACCESS", PyInt_FromLong(EGL_BAD_ACCESS));
    PyDict_SetItemString(d, "EGL_BAD_ALLOC", PyInt_FromLong(EGL_BAD_ALLOC));
    PyDict_SetItemString(d, "EGL_BAD_ATTRIBUTE", PyInt_FromLong(EGL_BAD_ATTRIBUTE));
    PyDict_SetItemString(d, "EGL_BAD_CONFIG", PyInt_FromLong(EGL_BAD_CONFIG));
    PyDict_SetItemString(d, "EGL_BAD_CONTEXT", PyInt_FromLong(EGL_BAD_CONTEXT));
    PyDict_SetItemString(d, "EGL_BAD_CURRENT_SURFACE", PyInt_FromLong(EGL_BAD_CURRENT_SURFACE));
    PyDict_SetItemString(d, "EGL_BAD_DISPLAY", PyInt_FromLong(EGL_BAD_DISPLAY));
    PyDict_SetItemString(d, "EGL_BAD_MATCH", PyInt_FromLong(EGL_BAD_MATCH));
    PyDict_SetItemString(d, "EGL_BAD_NATIVE_PIXMAP", PyInt_FromLong(EGL_BAD_NATIVE_PIXMAP));
    PyDict_SetItemString(d, "EGL_BAD_NATIVE_WINDOW", PyInt_FromLong(EGL_BAD_NATIVE_WINDOW));
    PyDict_SetItemString(d, "EGL_BAD_PARAMETER", PyInt_FromLong(EGL_BAD_PARAMETER));
    PyDict_SetItemString(d, "EGL_BAD_SURFACE", PyInt_FromLong(EGL_BAD_SURFACE));
    PyDict_SetItemString(d, "EGL_BUFFER_SIZE", PyInt_FromLong(EGL_BUFFER_SIZE));
    PyDict_SetItemString(d, "EGL_ALPHA_SIZE", PyInt_FromLong(EGL_ALPHA_SIZE));
    PyDict_SetItemString(d, "EGL_BLUE_SIZE", PyInt_FromLong(EGL_BLUE_SIZE));
    PyDict_SetItemString(d, "EGL_GREEN_SIZE", PyInt_FromLong(EGL_GREEN_SIZE));
    PyDict_SetItemString(d, "EGL_RED_SIZE", PyInt_FromLong(EGL_RED_SIZE));
    PyDict_SetItemString(d, "EGL_DEPTH_SIZE", PyInt_FromLong(EGL_DEPTH_SIZE));
    PyDict_SetItemString(d, "EGL_STENCIL_SIZE", PyInt_FromLong(EGL_STENCIL_SIZE));
    PyDict_SetItemString(d, "EGL_CONFIG_CAVEAT", PyInt_FromLong(EGL_CONFIG_CAVEAT));
    PyDict_SetItemString(d, "EGL_CONFIG_ID", PyInt_FromLong(EGL_CONFIG_ID));
    PyDict_SetItemString(d, "EGL_LEVEL", PyInt_FromLong(EGL_LEVEL));
    PyDict_SetItemString(d, "EGL_MAX_PBUFFER_HEIGHT", PyInt_FromLong(EGL_MAX_PBUFFER_HEIGHT));
    PyDict_SetItemString(d, "EGL_MAX_PBUFFER_PIXELS", PyInt_FromLong(EGL_MAX_PBUFFER_PIXELS));
    PyDict_SetItemString(d, "EGL_MAX_PBUFFER_WIDTH", PyInt_FromLong(EGL_MAX_PBUFFER_WIDTH));
    PyDict_SetItemString(d, "EGL_NATIVE_RENDERABLE", PyInt_FromLong(EGL_NATIVE_RENDERABLE));
    PyDict_SetItemString(d, "EGL_NATIVE_VISUAL_ID", PyInt_FromLong(EGL_NATIVE_VISUAL_ID));
    PyDict_SetItemString(d, "EGL_NATIVE_VISUAL_TYPE", PyInt_FromLong(EGL_NATIVE_VISUAL_TYPE));
    PyDict_SetItemString(d, "EGL_SAMPLES", PyInt_FromLong(EGL_SAMPLES));
    PyDict_SetItemString(d, "EGL_SAMPLE_BUFFERS", PyInt_FromLong(EGL_SAMPLE_BUFFERS));
    PyDict_SetItemString(d, "EGL_SURFACE_TYPE", PyInt_FromLong(EGL_SURFACE_TYPE));
    PyDict_SetItemString(d, "EGL_TRANSPARENT_TYPE", PyInt_FromLong(EGL_TRANSPARENT_TYPE));
    PyDict_SetItemString(d, "EGL_TRANSPARENT_BLUE_VALUE", PyInt_FromLong(EGL_TRANSPARENT_BLUE_VALUE));
    PyDict_SetItemString(d, "EGL_TRANSPARENT_GREEN_VALUE", PyInt_FromLong(EGL_TRANSPARENT_GREEN_VALUE));
    PyDict_SetItemString(d, "EGL_TRANSPARENT_RED_VALUE", PyInt_FromLong(EGL_TRANSPARENT_RED_VALUE));
    PyDict_SetItemString(d, "EGL_VENDOR", PyInt_FromLong(EGL_VENDOR));
    PyDict_SetItemString(d, "EGL_VERSION", PyInt_FromLong(EGL_VERSION));
    PyDict_SetItemString(d, "EGL_EXTENSIONS", PyInt_FromLong(EGL_EXTENSIONS));
    PyDict_SetItemString(d, "EGL_HEIGHT", PyInt_FromLong(EGL_HEIGHT));
    PyDict_SetItemString(d, "EGL_WIDTH", PyInt_FromLong(EGL_WIDTH));
    PyDict_SetItemString(d, "EGL_LARGEST_PBUFFER", PyInt_FromLong(EGL_LARGEST_PBUFFER));
    PyDict_SetItemString(d, "EGL_DRAW", PyInt_FromLong(EGL_DRAW));
    PyDict_SetItemString(d, "EGL_READ", PyInt_FromLong(EGL_READ));
    PyDict_SetItemString(d, "EGL_CORE_NATIVE_ENGINE", PyInt_FromLong(EGL_CORE_NATIVE_ENGINE));
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason) {
  return KErrNone;
}
#endif /*EKA2*/
