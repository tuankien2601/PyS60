/**
 * ====================================================================
 * glcanvas_util.cpp
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
 */
#include "glcanvas.h"

/** Borrowed from appuifwmodule.cpp
 * An utility for obtaining the Application object
 */

Application_object* get_app()
{
  PyInterpreterState *interp = PyThreadState_Get()->interp;
  PyObject* m = PyDict_GetItemString(interp->modules, "_appuifw");
  
  return (Application_object*)PyDict_GetItemString(PyModule_GetDict(m),
                                                   "app");
}

/**
 * Borrowed from appuifwmodule.cpp
 *
 */
void _uicontrolapi_decref(void *control_obj)
{
  Py_DECREF((PyObject *)control_obj);
}
/**
 * Utility function for calling a Python function/method from C++, borrowed from appuifwmodule.cpp
 *
 * @param func Python callable object
 * @param arg Python object containing arguments for func
 * @return Error code
 */
TInt app_callback_handler(void *func, void *arg)
{
  TInt error = KErrNone;
  
  PyObject *rval = PyEval_CallObject((PyObject*)func, (PyObject*)arg);
  
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
      // Note that PyErr_Fetch gives us ownership of these objects, so
      // we must remember to DECREF them.
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

  return error;
}

/**
 * Helper function for allocating memory
 *
 * @param size Size of desired memory block
 * @return A pointer to a newly allocated block of memory
 */
void *glcanvas_alloc(size_t size) {
  return User::Alloc(size);
}

/**
 * Helper function for freeing allocated memory
 *
 * @param ptr A pointer to a block of memory to be freed
 */
void glcanvas_free(void *ptr) {
  User::Free(ptr);
}
