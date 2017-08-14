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
#ifndef __GLCANVAS_MODULE_H
#define __GLCANVAS_MODULE_H

#include "Python.h"

#include "symbian_python_ext_util.h"
#include <stdlib.h>

#include <GLES/gl.h>
#include <GLES/egl.h>
#include "coecntrl.h"
#include <aknappui.h> 
#include "CAppuifwEventBindingArray.h"
#include "Python_appui.h"

#define GLCANVAS_DEFAULT_DEPTH_SIZE (16)

// UI Control api name. If this ever changes in appui/appuifw/appuifwmodule.cpp, it must be changed here also!
#define UICONTROLAPI_NAME "_uicontrolapi"
#define MY_APPUI ((get_app())->ob_data->appui)

// comment this out to disable debug messages
#define DEBUG_GLCANVAS

#define DEBUGMSG(msg) do { RDebug::Print(_L(msg)); } while(0)
#define DEBUGMSG1(msg,arg1) do { RDebug::Print(_L(msg),arg1); } while(0)
#define DEBUGMSG2(msg,arg1,arg2) do { RDebug::Print(_L(msg),arg1,arg2); } while(0)
#define DEBUGMSG3(msg,arg1,arg2,arg3) do { RDebug::Print(_L(msg),arg1,arg2,arg3); } while(0)
#define DEBUGMSG4(msg,arg1,arg2,arg3,arg4) do { RDebug::Print(_L(msg),arg1,arg2,arg3,arg4); } while(0)

#define RETURN_PYNONE do { Py_INCREF(Py_None); return Py_None; } while(0)

#ifndef EKA2
class CGLCanvas : public CCoeControl
#else
NONSHARABLE_CLASS(CGLCanvas) : public CCoeControl
#endif
{
public:
  CGLCanvas(PyObject *, PyObject *, PyObject *);
  int CreateContext(EGLint *attrib_list);
  virtual ~CGLCanvas();
  virtual void ConstructL(const TRect& aRect,
                          const CCoeControl* aParent, CAmarettoAppUi* aAppui);
private: //data
  // The display where the graphics are drawn
  EGLDisplay  iEglDisplay;
  
  // The rendering context
  EGLContext  iEglContext;
  
  // The window where the graphics are drawn
  EGLSurface  iEglSurface;
  
public:  //data
  // Frame counter variable
  int iFrame;
  void swapBuffers();
  void makeCurrent();
  int iOpenGLInitialized;
  void redraw() const;
  
  EGLint *GenerateEglAttributes(PyObject *);
  PyObject *GetRedrawCallBack();
  PyObject *GetEventCallBack();
  PyObject *GetResizeCallBack();
  
  void SetRedrawCallBack(PyObject *);
  void SetEventCallBack(PyObject *);
  void SetResizeCallBack(PyObject *);
  
protected:
  virtual void SizeChanged();
private:
  virtual void Draw(const TRect& aRect) const;
  virtual TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, 
				      TEventCode aType);
  
  int GetBufferSize();
  virtual void HandlePointerEventL(const TPointerEvent& aPointerEvent);
private:
  PyObject *iDrawCallback;
  PyObject *iEventCallback;
  PyObject *iResizeCallback;
  const CCoeControl *myParent;
  CAmarettoAppUi* myAppui;
};

// Forward declaration
class CAppuifwEventBindingArray;

struct GLCanvas_object {
  PyObject_VAR_HEAD
  CGLCanvas *ob_control;
  CAppuifwEventBindingArray *ob_event_bindings;
  ControlName control_name;
  // NOTE: fields above this line must match struct _control_object!
  PyObject *ob_drawcallback;
  PyObject *ob_eventcallback;
  PyObject *ob_resizecallback;
};

/** Borrowed from appuifwmodule.cpp
 * appuifw.Application object type representation
 */

struct Application_data;

struct Application_data {
  Application_data(CAmarettoAppUi* aAppUi, Application_object* /*aOp*/):
    appui(aAppUi)/*, ob_exit_key_cb(aOp,aAppUi), ob_event_cb(aOp,aAppUi),
    ob_command_cb(aOp,aAppUi), ob_menu_cb(aOp,aAppUi),
    ob_tab_cb(aOp,aAppUi), ob_focus_cb(aOp,aAppUi)*/
  {
    rsc_offset = (-1);
    //appui->SetMenuDynInitFunc(&(ob_menu_cb));
    //appui->SetMenuCommandFunc(&(ob_command_cb));
  }
  ~Application_data() {
    if (rsc_offset != (-1))
      (CEikonEnv::Static())->DeleteResourceFile(rsc_offset);
    /*appui->SetMenuDynInitFunc(NULL);
    appui->SetMenuCommandFunc(NULL);
    appui->SetExitFunc(NULL);
    appui->SetHostedControl(NULL, NULL);
    appui->SetFocusFunc(NULL);*/
  }

  CAmarettoAppUi* appui;
  TInt rsc_offset;
  /*CAmarettoCallback ob_exit_key_cb;
  CAmarettoCallback ob_event_cb;
  CAmarettoCallback ob_command_cb;
  CAmarettoCallback ob_menu_cb;
  CAmarettoCallback ob_tab_cb;
  CAmarettoCallback ob_focus_cb;*/
};


// Prototypes for functions in glcanvas_util.cpp
Application_object* get_app();
TInt app_callback_handler(void *func, void *arg);
void *glcanvas_alloc(size_t size);
void glcanvas_free(void *ptr);
void _uicontrolapi_decref(void *control_obj);



#endif // __GLCANVAS_MODULE_H
