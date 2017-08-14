/*
 * ====================================================================
 *  appuifw_callbacks.h
 *
 *  Callback class definitions
 *
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

#ifndef __APPUIFW_CALLBACKS_H
#define __APPUIFW_CALLBACKS_H

#include "appuifwmodule.h"

#ifndef EKA2
class CAppuifwCallback : public CAmarettoCallback
#else
NONSHARABLE_CLASS(CAppuifwCallback) : public CAmarettoCallback
#endif
{
public:
  CAppuifwCallback(Application_object* aApp, CAmarettoAppUi* aAppUi):
    CAmarettoCallback(aAppUi),iApp(aApp) {;}

protected:
  Application_object* iApp;

private:
  virtual TInt CallImpl_Impl(void* aArg)=0;
  virtual TInt CallImpl(void* aArg);
};

TInt CAppuifwCallback::CallImpl(void* aArg)
{
  PyGILState_STATE gstate = PyGILState_Ensure();
  TInt error = CallImpl_Impl(aArg);
  PyGILState_Release(gstate);
  
  return error;
}

class CAppuifwEventCallback : public CAppuifwCallback
{
public:
  CAppuifwEventCallback(Application_object* aApp,
                        CAmarettoAppUi* aAppUi):
    CAppuifwCallback(aApp, aAppUi) {;}

private:
  virtual TInt CallImpl_Impl(void* aArg) {
    CAppuifwEventBindingArray* ebs =
      ((_control_object*)AppuifwControl_AsControl(iApp->ob_body))->ob_event_bindings;

    if (ebs)
      return ebs->Callback(*((SAmarettoEventInfo*)aArg));
    else
      return KErrNone;
  }
};

class CAppuifwCommandCallback : public CAppuifwCallback
{
public:
  CAppuifwCommandCallback(Application_object* aApp,
                          CAmarettoAppUi* aAppUi):
    CAppuifwCallback(aApp, aAppUi) {;}

private:
  virtual TInt CallImpl_Impl(void* aArg) {

    PyObject* cb = NULL;
    int totalIdCounter = 0;
    TInt cmdIndex = (TInt)aArg-EPythonMenuExtensionBase;

    int sz = PyList_Size(iApp->ob_menu); 

    if (cmdIndex < sz) {
      cb = PyTuple_GetItem(PyList_GetItem(iApp->ob_menu, cmdIndex), 1);
    } else {

      for (int i=0; i< sz; i++) {
	PyObject* second = PyTuple_GetItem(PyList_GetItem(iApp->ob_menu, i), 1); 
	if (PyTuple_Check(second)) {
	  int subSize = PyTuple_Size(second);
	  for (int j=0; j < subSize; j++) {
	    if (cmdIndex == sz+totalIdCounter+j) {
	      cb = PyTuple_GetItem(PyTuple_GetItem(second, j), 1);
	      break;
	    }
	  }
	  totalIdCounter += subSize;
	}
      }
    }

    return app_callback_handler(cb);
  }
};


class CAppuifwMenuCallback : public CAppuifwCallback
{
public:
  CAppuifwMenuCallback(Application_object* aApp, 
                       CAmarettoAppUi* aAppUi):
    CAppuifwCallback(aApp, aAppUi) { iAppUi->CleanSubMenuArray(); }

private:

  virtual TInt CallImpl_Impl(void* aStruct) {

    CAmarettoAppUi::TAmarettoMenuDynInitParams *param = (CAmarettoAppUi::TAmarettoMenuDynInitParams*)aStruct;
    TInt error = KErrNone;
    int subMenuCounter = 0;    
    int sz = PyList_Size(iApp->ob_menu); 

    for (int i = 0; i < sz; i++) {

      CEikMenuPaneItem::SData item;
      PyObject* t = PyTuple_GetItem(PyList_GetItem(iApp->ob_menu, i), 0);
      if (!t || !PyUnicode_Check(t))
	item.iText = _L("!ERROR: BAD DATA!");
      else {
	item.iText.Copy(PyUnicode_AsUnicode(t),
			Min(PyUnicode_GetSize(t),
			    CEikMenuPaneItem::SData::ENominalTextLength));
	
	if ( param->iMenuId < R_PYTHON_SUB_MENU_00) {  /* first level menu */

	  PyObject* second = PyTuple_GetItem(PyList_GetItem(iApp->ob_menu, i), 1); 

	  if (PyTuple_Check(second)) { /* submenu item owner */
	  
	    item.iCommandId = EPythonMenuExtensionBase+i;
	    item.iCascadeId = R_PYTHON_SUB_MENU_00+subMenuCounter;
	    item.iFlags = 0;
	    item.iExtraText = _L("");

	    TRAP(error, (param->iMenuPane->AddMenuItemL(item) ));
	    if (error != KErrNone)
	      break;

	    iAppUi->subMenuIndex[i] = R_PYTHON_SUB_MENU_00+subMenuCounter;
	    subMenuCounter++;

	  } else { /* normal item */
	    item.iCommandId = EPythonMenuExtensionBase+i;
	    item.iCascadeId = 0;
	    item.iFlags = 0;
	    item.iExtraText = _L("");
	    TRAP(error, ( (param->iMenuPane)->AddMenuItemL(item)));
	    if (error != KErrNone)
	      break;
	    iAppUi->subMenuIndex[i] = 0;
	  } 

	} else if (param->iMenuId >= R_PYTHON_SUB_MENU_00) { /* submenu */

	  int subIdCounter = 0;
	  int itemIndex = 0;
	  for (int i=0; i<sz; i++) {
	    if (iAppUi->subMenuIndex[i] != 0) {
	      if (iAppUi->subMenuIndex[i] == param->iMenuId) {
		itemIndex = i;
		break;
	      }
	      else {
		PyObject* second = PyTuple_GetItem(PyList_GetItem(iApp->ob_menu, i), 1); 
		if (PyTuple_Check(second))
		  subIdCounter += PyTuple_Size(second);
	      }
	    }
	  }

	  PyObject* second = PyTuple_GetItem(PyList_GetItem(iApp->ob_menu, itemIndex), 1); 
	  int subsz = 0;
	  if (PyTuple_Check(second))
	    subsz = PyTuple_Size(second);
	  else {
	    PyErr_SetString(PyExc_TypeError, "tuple expected");
	    return -1;
	  }

	  iAppUi->aSubPane = param->iMenuPane;
	  if (error != KErrNone)
	    break;

	  for (int j=0; j<subsz; j++) {
	    CEikMenuPaneItem::SData subItem;
	    PyObject* submenu = PyTuple_GetItem(second, j);
	    PyObject* subTxt = PyTuple_GetItem(submenu, 0);
	    subItem.iText.Copy(PyUnicode_AsUnicode(subTxt),
			       Min(PyUnicode_GetSize(subTxt),
				   CEikMenuPaneItem::SData::ENominalTextLength));

	    subItem.iCommandId = EPythonMenuExtensionBase+sz+subIdCounter+j;
	    subItem.iCascadeId = 0;  
	    subItem.iFlags = 0;
	    subItem.iExtraText = _L("");
	    TRAP(error, ( (iAppUi->aSubPane)->AddMenuItemL(subItem)));
	    if (error != KErrNone)
	      break; 
	  }
	  break;
	} 
      }
    } 

    return error;
  }
};

class CAppuifwFocusCallback : public CAppuifwCallback
{
public:
  CAppuifwFocusCallback(Application_object* aApp,
                          CAmarettoAppUi* aAppUi):
    CAppuifwCallback(aApp, aAppUi) {
    iFunc = Py_None;
    Py_INCREF(Py_None);
  }
  
  ~CAppuifwFocusCallback() {
    Py_DECREF(iFunc);
  }
  
  int Set(PyObject* aFunc) {
    if ((aFunc != Py_None) && (!PyCallable_Check(aFunc))) {
      PyErr_SetString(PyExc_TypeError, "callable expected");
      return -1;
    }
    Py_DECREF(iFunc);
    iFunc = aFunc;
    Py_INCREF(iFunc);
    iAppUi->SetFocusFunc(((aFunc == Py_None) ? NULL : this));
    return 0;
  }
  
  PyObject* Get() {
    Py_INCREF(iFunc);
    return iFunc;
  }

private:
  virtual TInt CallImpl_Impl(void* aArg) {
    TInt error = KErrNone;
    if (iFunc) {
      PyObject *arg = Py_BuildValue("(N)", PyInt_FromLong((long)aArg));
      if (!arg)
        error = KErrNoMemory;
      else {
        error = app_callback_handler(iFunc, arg);
        Py_DECREF(arg);
      }
    }
    return error; 
  }

  PyObject* iFunc;
};

class CAppuifwExitKeyCallback : public CAppuifwCallback
{
public:
  CAppuifwExitKeyCallback(Application_object* aApp,
                          CAmarettoAppUi* aAppUi):
    CAppuifwCallback(aApp, aAppUi) {
    iFunc = Py_None;
    Py_INCREF(Py_None);
  }
  
  ~CAppuifwExitKeyCallback() {
    Py_DECREF(iFunc);
  }
  
  int Set(PyObject* aFunc) {
    if ((aFunc != Py_None) && (!PyCallable_Check(aFunc))) {
      PyErr_SetString(PyExc_TypeError, "callable expected");
      return -1;
    }
    Py_DECREF(iFunc);
    iFunc = aFunc;
    Py_INCREF(iFunc);
    iAppUi->SetExitFunc(((aFunc == Py_None) ? NULL : this));
    return 0;
  }
  
  PyObject* Get() {
    Py_INCREF(iFunc);
    return iFunc;
  }

private:
  virtual TInt CallImpl_Impl(void* /*aArg*/) {
    return ((iFunc != Py_None) ? app_callback_handler(iFunc):KErrNone);
  }

  PyObject* iFunc;
};

class CAppuifwTabCallback : public CAppuifwCallback
{
public:
  CAppuifwTabCallback(Application_object* aApp, CAmarettoAppUi* aAppUi):
    CAppuifwCallback(aApp, aAppUi), iFunc(NULL) {;}

  ~CAppuifwTabCallback() {
    Py_XDECREF(iFunc);
  }

  TInt Set(PyObject* aFunc) { // FIXME no check that aFunc is
			      // callable? ok, so this _is_ checked in
			      // appuifwmodule side, but consistency
			      // would be good.
    Py_XDECREF(iFunc);
    iFunc = aFunc;
    Py_XINCREF(iFunc);
    return KErrNone;
  }

private:
  virtual TInt CallImpl_Impl(void* aArg) {
    TInt error = KErrNone;
    if (iFunc) {
      PyObject *arg = Py_BuildValue("(N)", PyInt_FromLong((long)aArg));
      if (!arg)
        error = KErrNoMemory;
      else {
        error = app_callback_handler(iFunc, arg);
        Py_DECREF(arg);
      }
    }
    return error;
  }

  PyObject* iFunc;
};

#endif /* __APPUIFW_CALLBACKS_H */
