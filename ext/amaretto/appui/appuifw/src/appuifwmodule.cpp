/*
* ====================================================================
* Copyright (c) 2005-2009 Nokia Corporation
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
*
*  appuifwmodule.cpp
*
*  Python API to Series 60 application UI framework facilities
*
*  Implements currently (16.03.05) following Python types and functions:
*
*  Application (an implicit instance "app" always present)
*    unicode full_name()
*    set_tabs(list<unicode>, callable)
*    activate_tab(int)
*    set_exit()
*    callable exit_key_handler
*    list<tuple<unicode, callable>> menu
*    ui_control body
*    unicode title
*    mode screen
*    callable(focus) focus
*    layout(int)
*
*  Listbox
*    Listbox(list<unicode OR tuple<unicode, unicode>>, callable)
*    int current()
*    set_list(list<unicode OR tuple<unicode, unicode>>, [int])
*    bind(event_code, callable)
*
*  Form
*    Form(list<tuple<unicode, string, unicode>>, int)
*    execute()
*    add_menu_item()
*    insert(), pop(), sequence methods: length, item, assignment
*
*  Text
*    unicode get()
*    set(unicode)
*    add(unicode)
*    int len()
*    clear()
*    set_pos(int)
*    int get_pos()
*    bind(event_code, callable)
*    bool focus
*  attributes:
*    focus
*    style
*    color
*    highlight_color
*    font
*
*  Canvas
*    line((x1,y1),(x2,y2),(r,g,b))
*     - draw line from (x1,y1) to (x2,y2) with color (r,g,b)
*    rectfill((x1,y1),(x2,y2),(r,g,b))
*     - fill rectangle with corners at (x1,y1) and (x2,y2) with color (r,g,b)
*    clear([(r,g,b)])
*     - clear entire canvas to white or to given color
*    text((x,y),unicode,(r,g,b))
*     - draw text to (x,y) with given color
*    bind(event_code, callable)
*    CObject _ccoecontrol()
*     - return the pointer to the canvas CCoeControl as CObject
*  attributes:
*    draw (read/write)
*     - called when refreshing the display is needed with parameters:
*       (Canvas,((x1,y1),(x2,y2)))
*    resize (read/write)
*     - called when the canvas is resized with Canvas as parameter
*
*  Content_handler
*    Content_handler([callable])
*    open(unicode OR string)
*    open_standalone(unicode OR string)
*
*  int selection_list(list<unicode> [, int])
*
*  tuple<int> multi_selection_list(list<unicode> [, string, int])
*
*  <query_result_type> query(unicode, string [,<initial_value>])
*
*  note(unicode, string)
*
*  (unicode, unicode)
*  multi_query(unicode, unicode)
*
*  int popup_menu(list<unicode OR tuple<unicode, unicode>>, [unicode])
*
*  list<unicode> available_fonts()
*/

#include "appuifwmodule.h"
#include "appuifw_callbacks.h"
#include "colorspec.cpp"
// Yuck. This is ugly but will do for now until a better solution is implemented
// for shared functionality between appuifw and graphics modules. Perhaps they
// should just be merged?
#include "../../../graphics/src/fontspec.cpp"

#if SERIES60_VERSION>=28
#include "akniconutils.h"
#include "GULICON.H"
#define SCALABLE_UI
#endif /* SERIES60_VERSION */

#define UICONTROLAPI_NAME "_uicontrolapi"

/* This macro is defined both in stdapis\stdefs.h and in structmember.h.
 * This file expects the definition from structmember.h but it gets that
 * from stdapis\stdefs.h. So, copying its definition from structmember.h.
 */
#define offsetof(type, member) ( (int) & ((type*)0) -> member )

struct Application_data {
  Application_data(CAmarettoAppUi* aAppUi, Application_object* aOp):
    appui(aAppUi), ob_exit_key_cb(aOp,aAppUi), ob_event_cb(aOp,aAppUi),
    ob_command_cb(aOp,aAppUi), ob_menu_cb(aOp,aAppUi),
    ob_tab_cb(aOp,aAppUi), ob_focus_cb(aOp,aAppUi)
  {
    rsc_offset = (-1);
    appui->SetMenuDynInitFunc(&(ob_menu_cb));
    appui->SetMenuCommandFunc(&(ob_command_cb));
  }
  ~Application_data() {
    if (rsc_offset != (-1))
      (CEikonEnv::Static())->DeleteResourceFile(rsc_offset);
    appui->SetMenuDynInitFunc(NULL);
    appui->SetMenuCommandFunc(NULL);
    appui->SetExitFunc(NULL);
    appui->SetHostedControl(NULL, NULL, -1);
    appui->SetFocusFunc(NULL);
  }

  CAmarettoAppUi* appui;
  TInt rsc_offset;
  CAppuifwExitKeyCallback ob_exit_key_cb;
  CAppuifwEventCallback ob_event_cb;
  CAppuifwCommandCallback ob_command_cb;
  CAppuifwMenuCallback ob_menu_cb;
  CAppuifwTabCallback ob_tab_cb;
  CAppuifwFocusCallback ob_focus_cb;
};

/*
 * These are moved from PYTHON_GLOBALS
 */
PyTypeObject Application_type;
PyTypeObject Listbox_type;
PyTypeObject Content_handler_type;
PyTypeObject Form_type;
PyTypeObject Text_type;
PyTypeObject Canvas_type;
PyTypeObject Icon_type;
#if SERIES60_VERSION>=30
PyTypeObject InfoPopup_type;
#endif /* SERIES60_VERSION */

TBool touch_enabled_flag;
TBool directional_pad_flag;

EXPORT_C TBool TouchEnabled()
{
    return touch_enabled_flag;
}

/*
 * An utility for obtaining the Application object
 */

static Application_object* get_app()
{
  PyInterpreterState *interp = PyThreadState_Get()->interp;
  PyObject* m = PyDict_GetItemString(interp->modules, "_appuifw");

  return (Application_object*)PyDict_GetItemString(PyModule_GetDict(m),
                                                   "app");
}

/* A function that is used with CAsyncCallBack.
 * It is called to destroy the object after
 * UI processing is done.
 */

TInt decrefObject(TAny* aObj)
{
  PyGILState_STATE gstate = PyGILState_Ensure();
  Py_DECREF((PyObject*)aObj);
  PyGILState_Release(gstate);
  return 1;
}

/*
 * A helper function for the implementation of callbacks
 * from C/C++ code to Python callables
 */

TInt app_callback_handler(TAny* func)
{
  // Strictly speaking this violates the Python/C API spec.
  // We should pass an empty tuple instead of NULL.
  return app_callback_handler(func, NULL);
}

TInt app_callback_handler(TAny* func, TAny* arg)
{
  TInt error = KErrNone;

  PyObject* rval = PyEval_CallObject((PyObject*)func, (PyObject*)arg);
  // This is the only way to refresh the Navi keys every time it loses focus
  // The Canvas Draw is not called by the framework if there is nothing to redraw
  CAmarettoAppUi *myAppui = MY_APPUI;
  myAppui->RefreshDirectionalPad();
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

TInt
AppuifwControl_Check(PyObject *obj)
{
  return (AppuifwControl_AsControl(obj))?1:0;
}

struct _control_object *
AppuifwControl_AsControl(PyObject *obj)
{
  PyObject *controlapi_cobject=NULL;
  if (!(controlapi_cobject=PyObject_GetAttrString(obj, UICONTROLAPI_NAME)) ||
      !(PyCObject_Check(controlapi_cobject))) {
    Py_XDECREF(controlapi_cobject);
    return NULL;
  }
  struct _control_object *control_object=
    (struct _control_object *)PyCObject_AsVoidPtr(controlapi_cobject);
  Py_DECREF(controlapi_cobject);
  return control_object;
}

static void _uicontrolapi_decref(void *control_obj)
{
  Py_DECREF((PyObject *)control_obj);
}


/*
 * Implementation of appuifw.Application (an implicit instance
 * of this type with name "app" comes with this module, new
 * instances can not be created from Python)
 */

extern "C" {static void application_dealloc(Application_object *op);}

/*
 * SPy_S60app_New() -- C API only!
 */

extern "C" PyObject *
SPy_S60app_New()
{
  Application_object *op = PyObject_New(Application_object,
                                        &Application_type);
  if (op == NULL)
    return PyErr_NoMemory();

  op->ob_dict_attr = NULL;
  op->ob_menu = NULL;
  op->ob_body = Py_None;
  Py_INCREF(Py_None);
  op->ob_title = NULL;
  op->ob_directional_pad = Py_BuildValue("O", Py_False);
  op->ob_track_allocations = Py_BuildValue("O", Py_True);
  op->ob_screen = Py_BuildValue("s", "normal");
  op->ob_data = NULL;

#ifdef EKA2
  op->ob_orientation = Py_BuildValue("s", "automatic");
#endif
  CEikonEnv* env = CEikonEnv::Static();
  CAmarettoAppUi* appui = STATIC_CAST(CAmarettoAppUi*, env->EikAppUi());

  RLibrary avkonDll;
  touch_enabled_flag = EFalse;

  /* PenEnabled() is not present in 3rdEd SDK, so we try to load avkon and
   * find the ordinal number for that function */
  if ( avkonDll.Load( _L( "avkon.dll" ) ) == KErrNone ) {
    #ifdef __WINS__
    TLibraryFunction pen_enabled = avkonDll.Lookup( PEN_ENABLED_ORDINAL_WINS );
    #else
    TLibraryFunction pen_enabled = avkonDll.Lookup( PEN_ENABLED_ORDINAL_ARM );
    #endif
    if (pen_enabled != NULL && pen_enabled())
        touch_enabled_flag = ETrue;
    avkonDll.Close();
  }
  // Virtual D-pad is enabled for touch devices by default
  if (touch_enabled_flag)
      directional_pad_flag = ETrue;
  else
      directional_pad_flag = EFalse;
  TInt error;
  const TDesC* title;
  TRAP(error,
       (title =
        ((CAknTitlePane*)
         ((env->AppUiFactory()->StatusPane())->
          ControlL(TUid::Uid(EEikStatusPaneUidTitle))))->Text())
       );

  if (error != KErrNone) {
    application_dealloc(op);
    return SPyErr_SetFromSymbianOSErr(error);
  }

  if (!(op->ob_title =
        PyUnicode_FromUnicode(title->Ptr(), title->Length())) ||
      (!(op->ob_data = new Application_data(appui, op))) ||
      (!(op->ob_menu = PyList_New(0)))) {
    application_dealloc(op);
    return NULL;
  }

  TParse f;
  TFileName fn = op->ob_data->appui->Application()->AppFullName();
  f.Set(KAppuiFwRscFile, &fn, NULL);
  TRAP(error,
       (op->ob_data->rsc_offset = env->AddResourceFileL(f.FullName())));

  if (error != KErrNone) {
    application_dealloc(op);
    return SPyErr_SetFromSymbianOSErr(error);
  }

  return (PyObject *) op;
}

extern "C" PyObject *
app_full_name(Application_object *self)
{
  TFileName n = ((self->ob_data)->appui)->Application()->AppFullName();

  return Py_BuildValue("u#", n.Ptr(), n.Length());
}

extern "C" PyObject *
app_uid(Application_object *self)
{
  TUid uid = ((self->ob_data)->appui)->Application()->AppDllUid();
  TBuf<KMaxUidName> uidName(uid.Name());

  uidName.Delete(KMaxUidName-1,1);
  uidName.Delete(0,1);

  return Py_BuildValue("u#", uidName.Ptr(), uidName.Length());
}

extern "C" PyObject *
app_set_tabs(Application_object *self, PyObject *args)
{
  TInt error = KErrNone;
  PyObject* list;
  PyObject* cb;

  if (!PyArg_ParseTuple(args, "O!O", &PyList_Type, &list, &cb))
    return NULL;

  int sz = PyList_Size(list);

  if ((sz > 1) && !PyCallable_Check(cb)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  CDesCArray *tab_text_list = NULL;
  if (sz > 1) {
    if (!(tab_text_list = new CDesCArrayFlat(sz)))
      return PyErr_NoMemory();

    for (int i = 0; i < sz; i++) {
      PyObject* s = PyList_GetItem(list, i);
      if (!PyUnicode_Check(s))
        error = KErrArgument;
      else {
        TPtr buf(PyUnicode_AsUnicode(s), PyUnicode_GetSize(s),
                 PyUnicode_GetSize(s));
        TRAP(error, tab_text_list->AppendL(buf));
      }
      if (error != KErrNone)
        break;
    }
  }
  else
    cb = Py_None;

  if (error == KErrNone) {
    self->ob_data->ob_tab_cb.Set(cb);
    Py_BEGIN_ALLOW_THREADS
    error = self->ob_data->appui->EnableTabs(tab_text_list,
                                             &self->ob_data->ob_tab_cb);
    Py_END_ALLOW_THREADS
  }

  delete tab_text_list;

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
app_activate_tab(Application_object *self, PyObject *args)
{
  TInt index;

  if (!PyArg_ParseTuple(args, "i", &index))
    return NULL;

  Py_BEGIN_ALLOW_THREADS
  self->ob_data->appui->SetActiveTab(index);
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}

#if SERIES60_VERSION>=28
extern "C" PyObject *
app_layout(Application_object *self, PyObject *args)
{
  TInt layout = 0;
  TRect rect;
  TBool available = EFalse;

  if (!PyArg_ParseTuple(args, "i", &layout))
    return NULL;

  available = AknLayoutUtils::LayoutMetricsRect((AknLayoutUtils::TAknLayoutMetrics)layout, rect);

  if(available) {
    // size, position:
    return Py_BuildValue("((ii),(ii))", rect.Width(),
                                        rect.Height(),
                                        rect.iTl.iX,
                                        rect.iTl.iY
                                        );
  } else {
    PyErr_SetString(PyExc_ValueError, "unknown layout");
    return NULL;
  }
}
#endif /* SERIES60_VERSION */

extern "C" PyObject *
app_set_exit(Application_object *self, PyObject* /*args*/)
{
  self->ob_data->appui->SetExitFlag();

  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" {

  static void
  application_dealloc(Application_object *op)
  {
    delete op->ob_data;
    op->ob_data = NULL;
    Py_XDECREF(op->ob_dict_attr);
    Py_XDECREF(op->ob_menu);
    Py_XDECREF(op->ob_body);
    Py_XDECREF(op->ob_title);
    Py_XDECREF(op->ob_screen);
    Py_XDECREF(op->ob_directional_pad);
    Py_XDECREF(op->ob_track_allocations);
#ifdef EKA2
    Py_XDECREF(op->ob_orientation);
#endif
    PyObject_Del(op);
  }

  static const PyMethodDef application_methods[] = {
    {"set_exit", (PyCFunction)app_set_exit, METH_NOARGS},
    {"full_name", (PyCFunction)app_full_name, METH_NOARGS},
    {"uid", (PyCFunction)app_uid, METH_NOARGS},
    {"set_tabs", (PyCFunction)app_set_tabs, METH_VARARGS},
    {"activate_tab", (PyCFunction)app_activate_tab, METH_VARARGS},
#if SERIES60_VERSION>=28
    {"layout", (PyCFunction)app_layout, METH_VARARGS},
#endif /* SERIES60_VERSION */
    {NULL,              NULL}           /* sentinel */
  };

  static PyObject *
  application_getattr(Application_object *op, char *name)
  {
    if (!strcmp(name, "title")) {
      Py_INCREF(op->ob_title);
      return op->ob_title;
    }

    if (!strcmp(name, "menu")) {
      Py_INCREF(op->ob_menu);
      return op->ob_menu;
    }

    if (!strcmp(name, "body")) {
      Py_INCREF(op->ob_body);
      return op->ob_body;
    }

    if (!strcmp(name, "screen")) {
      Py_INCREF(op->ob_screen);
      return op->ob_screen;
    }

    if (!strcmp(name, "directional_pad")) {
          Py_INCREF(op->ob_directional_pad);
          return op->ob_directional_pad;
    }

    if (!strcmp(name, "track_allocations")) {
      Py_INCREF(op->ob_track_allocations);
      return op->ob_track_allocations;
    }

#ifdef EKA2
    if (!strcmp(name, "orientation")) {
      Py_INCREF(op->ob_orientation);
      return op->ob_orientation;
    }
#endif
    if (!strcmp(name, "exit_key_handler"))
      return op->ob_data->ob_exit_key_cb.Get();

    if (!strcmp(name, "focus"))
      return op->ob_data->ob_focus_cb.Get();

    if (op->ob_dict_attr != NULL) {
      PyObject *v = PyDict_GetItemString(op->ob_dict_attr, name);
      if (v != NULL) {
        Py_INCREF(v);
        return v;
      }
    }

    return Py_FindMethod((PyMethodDef*)application_methods,
                         (PyObject *)op, name);
  }


  static int
  application_setattr(Application_object *op, char *name, PyObject *v)
  {
    /*
     * title
     */

    if (!strcmp(name, "title")) {
      if (!PyUnicode_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "unicode string expected");
        return -1;
      }

      TPtr buf(PyUnicode_AsUnicode(v), PyUnicode_GetSize(v),
               PyUnicode_GetSize(v));

      TRAPD(error, {
        CAknTitlePane* tp = (CAknTitlePane*)
          (((CEikonEnv::Static())->AppUiFactory()->StatusPane())
           ->ControlL(TUid::Uid(EEikStatusPaneUidTitle)));
        tp->SetTextL(buf);
      });

      if (error == KErrNone) {
        Py_DECREF(op->ob_title);
        op->ob_title = v;
        Py_INCREF(v);
        return 0;
      }
      else {
        SPyErr_SetFromSymbianOSErr(error);
        return -1;
      }
    }

    /*
     * menu
     */

    if (!strcmp(name, "menu")) {
      const static char menu_type_msg[] =
        "list of (unicode, callable) or (unicode, (unicode, callable)...) expected";

      if (!PyList_Check(v))  {
        PyErr_SetString(PyExc_TypeError, menu_type_msg);
        return -1;
      }

      int sz = PyList_Size(v);
      if (sz > KMaxPythonMenuExtensions) {
        PyErr_SetString(PyExc_ValueError, "too many menu items");
        return -1;
      }

      for (int i = 0; i < sz; i++) {
        PyObject* t = PyList_GetItem(v, i);
        if ((!PyTuple_Check(t)) ||
        (!PyUnicode_Check(PyTuple_GetItem(t, 0))) ) {
          PyErr_SetString(PyExc_TypeError, menu_type_msg);
          return -1;
        }

    PyObject* secondObj = PyTuple_GetItem(t, 1);

    if (!PyCallable_Check(secondObj))
      if (PyTuple_Check(secondObj)) { /* submenu checking */

        int subsz = PyTuple_Size(secondObj);
        if (subsz > KMaxPythonMenuExtensions) {
          PyErr_SetString(PyExc_ValueError, "too many submenu items");
          return -1;
        }

        for (int j=0; j < subsz; j++) {
          PyObject* submenu = PyTuple_GetItem(secondObj, j);
          if ((!PyTuple_Check(submenu)) ||
          (!PyUnicode_Check(PyTuple_GetItem(submenu, 0))) ||
          (!PyCallable_Check(PyTuple_GetItem(submenu, 1)))) {
        PyErr_SetString(PyExc_TypeError, menu_type_msg);
        return -1;
          }
        }

      }
      else {
        PyErr_SetString(PyExc_TypeError, menu_type_msg);
        return -1;
      }
      }

      Py_XDECREF(op->ob_menu);
      op->ob_menu = v;
      Py_XINCREF(v);
      return 0;
    }

    /*
     * body
     */

    if (!strcmp(name, "body")) {
      if((v != Py_None) && !AppuifwControl_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "UI control expected");
        return -1;
      }

      TInt error;

      if (v == Py_None) {
        Py_BEGIN_ALLOW_THREADS
        error = op->ob_data->appui->SetHostedControl(0, 0, -1);
        Py_END_ALLOW_THREADS
      }
      else {
        Py_BEGIN_ALLOW_THREADS
        error =
          op->ob_data->appui->
          SetHostedControl(((_control_object*)AppuifwControl_AsControl(v))->ob_control,
                           &(op->ob_data->ob_event_cb), ((_control_object*)AppuifwControl_AsControl(v))->control_name);
        Py_END_ALLOW_THREADS
      }

      if (error != KErrNone) {
        SPyErr_SetFromSymbianOSErr(error);
        return -1;
      }

      Py_XDECREF(op->ob_body);
      op->ob_body = v;
      Py_XINCREF(v);
      return 0;
    }


    /*
     * screen mode
     */

    if (!strcmp(name, "screen")) {
      if (!PyString_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "invalid screen setting: string expected");
        return -1;
      }
      char *screen_mode=PyString_AsString(v);
      if (!strcmp(screen_mode,"normal")) { // normal screen mode
        op->ob_data->appui->SetScreenmode(ENormal);
      } else if (!strcmp(screen_mode,"large")) { // fullscreen with soft_keys
             op->ob_data->appui->SetScreenmode(ELarge);
      } else if (!strcmp(screen_mode,"full")) { // full screen
          op->ob_data->appui->SetScreenmode(EFull);
      }
      else {
        PyErr_SetString(PyExc_TypeError, "invalid screen mode: must be one of normal, large, full");
        return -1;
      }
      Py_DECREF(op->ob_screen);
      Py_INCREF(v);
      op->ob_screen=v;
      return 0;
    }

    /*
     * directional_pad
     */
   if (!strcmp(name, "directional_pad")) {
     if (v == Py_True && touch_enabled_flag) {
         directional_pad_flag = ETrue;
     } else if (v == Py_False) {
         directional_pad_flag = EFalse;
     } else {
         PyErr_SetString(PyExc_TypeError, "bool value expected");
         return -1;
     }
      Py_DECREF(op->ob_directional_pad);
      Py_INCREF(v);
      op->ob_directional_pad = v;
      return 0;
    }

   /*
    * track allocations
    */
  if (!strcmp(name, "track_allocations")) {
    if (v == Py_True) {
        track_allocations(true);
    } else if (v == Py_False) {
        track_allocations(false);
    } else {
        PyErr_SetString(PyExc_TypeError, "bool value expected");
        return -1;
    }
     Py_DECREF(op->ob_track_allocations);
     Py_INCREF(v);
     op->ob_track_allocations = v;
     return 0;
   }

#ifdef EKA2
    /*
     * orientation
     */

    if (!strcmp(name, "orientation")) {
      if (!PyString_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "invalid orientation setting: string expected");
        return -1;
      }
      char *orientation_mode = PyString_AsString(v);
      CAknAppUiBase::TAppUiOrientation orientation = CAknAppUiBase::EAppUiOrientationAutomatic;

      if (!strcmp(orientation_mode,"portrait")) {
        orientation = CAknAppUiBase::EAppUiOrientationPortrait;
      } else if (!strcmp(orientation_mode,"landscape")) {
        orientation = CAknAppUiBase::EAppUiOrientationLandscape;
      } else if (!strcmp(orientation_mode,"automatic")) {
        orientation = CAknAppUiBase::EAppUiOrientationAutomatic;
      } else {
        PyErr_SetString(PyExc_TypeError, "invalid orientation mode: must be one of portrait, landscape or automatic");
        return -1;
      }

      TRAPD(error, op->ob_data->appui->SetOrientationL(orientation));
      if (error != KErrNone) {
          SPyErr_SetFromSymbianOSErr(error);
          return -1;
      }

      Py_DECREF(op->ob_orientation);
      Py_INCREF(v);
      op->ob_orientation=v;
      return 0;
    }
#endif
    /*
     * exit_key_handler
     */

    if (!strcmp(name, "exit_key_handler"))
      return op->ob_data->ob_exit_key_cb.Set(v);

    /*
     * focus_handler
     */

    if (!strcmp(name, "focus"))
      return op->ob_data->ob_focus_cb.Set(v);

    /*
     * other
     */

    if (!(op->ob_dict_attr) && !(op->ob_dict_attr = PyDict_New()))
      return -1;

    if (v == NULL) {
      int rv = PyDict_DelItemString(op->ob_dict_attr, name);
      if (rv < 0)
        PyErr_SetString(PyExc_AttributeError,
                        "delete non-existing app attribute");
      return rv;
    }
    else
      return PyDict_SetItemString(op->ob_dict_attr, name, v);
  }

  static const PyTypeObject c_Application_type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "Application",
    sizeof(Application_object),
    0,
    /* methods */
    (destructor)application_dealloc,    /* tp_dealloc */
    0,                                  /* tp_print */
    (getattrfunc)application_getattr,   /* tp_getattr */
    (setattrfunc)application_setattr,   /* tp_setattr */
    0,                                  /* tp_compare */
    0,                                  /* tp_repr */
    0,                                  /* tp_as _number*/
    0,                                  /* tp_as _sequence*/
    0,                                  /* tp_as _mapping*/
    0,                                  /* tp_hash */
  };
} /* extern "C" */

/*
 *
 * Implementation of appuifw.selection_list()
 *
 */

extern "C" PyObject *
selection_list(PyObject* /*self*/, PyObject* args,  PyObject *kwds)
{
  TInt error = KErrNone;
  TInt resp = 0;
  PyObject* list;
  int search = 0;

  static const char *const kwlist[] = {"choices", "search_field", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|i", (char**)kwlist,
                   &PyList_Type, &list, &search))
    return NULL;

  if ( (search<0) || (search>1)) {
    PyErr_SetString(PyExc_ValueError, "search field can be 0 or 1");
    return NULL;
  }

  CDesCArray *items_list = new CDesCArrayFlat(5);
  if (items_list == NULL)
    return PyErr_NoMemory();

  TBuf<KMaxFileName+1> temp;
  int sz = PyList_Size(list);
  for (int i = 0; i < sz; i++) {
    PyObject* s = PyList_GetItem(list, i);
    if (!PyUnicode_Check(s))
      error = KErrArgument;
    else {
      temp.Copy(KSeparatorTab);
      temp.Append(PyUnicode_AsUnicode(s),
                  Min(PyUnicode_GetSize(s), KMaxFileName));
      TRAP(error, items_list->AppendL(temp));
    }

    if (error != KErrNone)
      break;
  }

  TInt index = (-1);
  CAknSelectionListDialog *dlg = NULL;

  if (error == KErrNone) {
    Py_BEGIN_ALLOW_THREADS
    TRAP(error, {
      dlg = CAknSelectionListDialog::NewL(index, items_list,
                                          R_AVKON_DIALOG_EMPTY_MENUBAR);
      if (search==1)
    resp = dlg->ExecuteLD(R_APPUIFW_SEL_LIST_QUERY);
      else
    resp = dlg->ExecuteLD(R_APPUIFW_SEL_LIST);
    });
    Py_END_ALLOW_THREADS
  }

  delete items_list;

  if ((error == KErrNone) && resp)
    return Py_BuildValue("i", index);
  else
    RETURN_ERROR_OR_PYNONE(error);
}

/*
 *
 * Implementation of appuifw.multi_selection_list()
 *
 */
#if SERIES60_VERSION==28
_LIT(KIconsFile, "Z:\\system\\data\\avkon2.mif");
#elif SERIES60_VERSION>28
_LIT(KIconsFile, "Z:\\resource\\apps\\avkon2.mif");
#else
_LIT(KIconsFile, "Z:\\system\\data\\avkon.mbm");
#endif /* SERIES60_VERSION */

extern "C" PyObject *
multi_selection_list(PyObject* /*self*/, PyObject *args, PyObject *kwds)
{
  TInt error = KErrNone;
  PyObject* list;
  char *type = NULL;
  int l_type = 0;
  int search = 0;
  TBool isChkbx = EFalse;

  static const char *const kwlist[] = {"choices", "style", "search_field", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|s#i", (char**)kwlist,
                   &PyList_Type, &list, &type, &l_type, &search))
    return NULL;

  TPtrC8 type_style((TUint8*)type, l_type);

  if (!type || (type_style.Compare(KCheckboxStyle) == 0))
    isChkbx = ETrue;
  else if (type && (type_style.Compare(KCheckmarkStyle) != 0)) {
    PyErr_SetString(PyExc_ValueError, "unknown style type");
    return NULL;
  }

  if ( (search<0) || (search>1) ){
    PyErr_SetString(PyExc_ValueError, "search field can be 0 or 1");
    return NULL;
  }

  PyObject* selected = NULL;
  int sz = PyList_Size(list);

  CDesCArray *items_list = new CDesCArrayFlat(5);
  if (items_list == NULL)
    return PyErr_NoMemory();

  CArrayFixFlat<TInt> *selected_items = NULL;
  TRAP(error,
       selected_items = new(ELeave) CArrayFixFlat<TInt>(sz));

  if (error != KErrNone) {
    delete items_list;
    items_list = NULL;
    selected_items = NULL;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  TBuf<KMaxFileName+1> temp;

  for (int i = 0; i < sz; i++) {
    PyObject* s = PyList_GetItem(list, i);
    if (!PyUnicode_Check(s))
      error = KErrArgument;
    else {
      if (isChkbx) {
        temp.Copy(_L("1\t"));
      }
      else {
        temp.Copy(_L("\t"));
      }
      temp.Append(PyUnicode_AsUnicode(s),
          Min(PyUnicode_GetSize(s), KMaxFileName));
      TRAP(error, items_list->AppendL(temp));
    }

    if (error != KErrNone)
      break;
  }

  CArrayPtr<CGulIcon>* icons = NULL;
  TRAP(error,
       icons = new(ELeave) CArrayPtrFlat<CGulIcon>(2));

  if (error != KErrNone) {
    delete items_list;
    items_list = NULL;
    delete selected_items;
    selected_items = NULL;
    icons = NULL;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  if (isChkbx) {
#if SERIES60_VERSION>=28
    TRAP(error,
    {
    CFbsBitmap* bitMapOn = NULL;
      CFbsBitmap* maskOn = NULL;
      AknIconUtils::CreateIconL(bitMapOn,
                                maskOn,
                                KIconsFile,
                                EMbmAvkonQgn_prop_checkbox_on,
                                EMbmAvkonQgn_prop_checkbox_on_mask);
      icons->AppendL(CGulIcon::NewL(bitMapOn, maskOn));

    CFbsBitmap* bitMapOff = NULL;
      CFbsBitmap* maskOff = NULL;
      AknIconUtils::CreateIconL(bitMapOff,
                                maskOff,
                                KIconsFile,
                                EMbmAvkonQgn_prop_checkbox_off,
                                EMbmAvkonQgn_prop_checkbox_off_mask);
      icons->AppendL(CGulIcon::NewL(bitMapOff, maskOff));
      });
  } else
    TRAP(error,
      {
      CFbsBitmap* bitMap = NULL;
        CFbsBitmap* mask = NULL;
        AknIconUtils::CreateIconL(bitMap,
                                mask,
                                KIconsFile,
                                EMbmAvkonQgn_indi_marked_add,
                                EMbmAvkonQgn_indi_marked_add_mask);
        icons->AppendL(CGulIcon::NewL(bitMap, mask));
      });
#else
    CEikonEnv* env = CEikonEnv::Static();
    TRAP(error, {
    icons->AppendL(env->CreateIconL(KIconsFile, EMbmAvkonQgn_prop_checkbox_on,
                                    EMbmAvkonQgn_prop_checkbox_on_mask));
    icons->AppendL(env->CreateIconL(KIconsFile, EMbmAvkonQgn_prop_checkbox_off,
                                    EMbmAvkonQgn_prop_checkbox_off_mask));
    });
  } else {
      CEikonEnv* env = CEikonEnv::Static();
      TRAP(error, icons->AppendL(env->CreateIconL(KIconsFile,
          EMbmAvkonQgn_indi_marked_add, EMbmAvkonQgn_indi_marked_add_mask)));
    }
#endif /* SERIES60_VERSION */

  if (error != KErrNone) {
    delete items_list;
    items_list = NULL;
    delete selected_items;
    selected_items = NULL;
    delete icons;
    icons = NULL;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  TInt index = (-1);
  CAknMarkableListDialog *dlg = NULL;

  if (error == KErrNone) {
    Py_BEGIN_ALLOW_THREADS
      TRAP(error, {
      dlg = CAknMarkableListDialog::NewL(index, selected_items, items_list,
                         R_APPUIFW_MENU_FOR_MULTISEL_LIST);

      if (isChkbx && (search == 0))
        dlg->PrepareLC(R_APPUIFW_MULTI_SEL_LIST);
      else if (isChkbx && (search == 1))
        dlg->PrepareLC(R_APPUIFW_MULTI_SEL_LIST_QUERY);
      else if (!isChkbx && (search == 0))
        dlg->PrepareLC(R_APPUIFW_MARKABLE_SEL_LIST);
      else if (!isChkbx && (search == 1))
        dlg->PrepareLC(R_APPUIFW_MARKABLE_SEL_LIST_QUERY);

      dlg->SetIconArrayL(icons);
      dlg->RunLD();
    });

    Py_END_ALLOW_THREADS

    if (error != KErrNone) {
      delete items_list;
      items_list = NULL;
      delete selected_items;
      selected_items = NULL;
      return SPyErr_SetFromSymbianOSErr(error);
    }

    TInt selItemNum = selected_items->Count();
    selected = PyTuple_New(selItemNum);

    for (int i=0; i < selItemNum; i++)
      error = PyTuple_SetItem(selected, i, Py_BuildValue(
                                        "i", selected_items->operator[](i)) );

  }

  delete items_list;
  items_list = NULL;
  delete selected_items;
  selected_items = NULL;

  if (error == KErrNone)
    return selected;
  else
    RETURN_ERROR_OR_PYNONE(error);
}

/*
 *
 * Implementation of appuifw.query()
 *
 */


static TReal epoch_local_as_TReal()
{
#ifndef EKA2
  TLocale loc;
  return (epoch_as_TReal()+
          ((1000.0*1000.0)*
           TInt64(loc.UniversalTimeOffset().Int()).GetTReal()));
#else
  TLocale loc;
  return (epoch_as_TReal()+
          ((1000.0*1000.0)*
           TReal64(TInt64(loc.UniversalTimeOffset().Int()))));
#endif /*EKA2*/
}


static TReal datetime_as_TReal(const TTime& aTime)
{
#ifndef EKA2
  TDateTime dt = aTime.DateTime();
  return TInt64((dt.Hour()*3600)+(dt.Minute()*60)).GetTReal();
#else
  TDateTime dt = aTime.DateTime();
  return TReal64(TInt64((dt.Hour()*3600)+(dt.Minute()*60)));
#endif /*EKA2*/
}

static TReal datetime_with_secs_as_TReal(const TTime& aTime)
{
#ifndef EKA2
  TDateTime dt = aTime.DateTime();
  return TInt64((dt.Hour()*3600)+(dt.Minute()*60)+(dt.Second())).GetTReal();
#else
  TDateTime dt = aTime.DateTime();
  return TReal64(TInt64((dt.Hour()*3600)+(dt.Minute()*60)+(dt.Second())));
#endif /*EKA2*/
}

static TInt rid_by_type(const TDesC8& aName)
{
  if (aName.Compare(KTextFieldType) == 0)
    return R_APPUIFW_DATA_QUERY;

  if (aName.Compare(KNumberFieldType) == 0)
    return R_APPUIFW_NUMBER_QUERY;

  if (aName.Compare(KFloatFieldType) == 0)
    return R_APPUIFW_FLOAT_QUERY;

  if (aName.Compare(KCodeFieldType) == 0)
    return R_APPUIFW_CODE_QUERY;

  if (aName.Compare(KDateFieldType) == 0)
    return R_APPUIFW_DATE_QUERY;

  if (aName.Compare(KTimeFieldType) == 0)
    return R_APPUIFW_TIME_QUERY;

  if (aName.Compare(KQueryFieldType) == 0)
    return R_APPUIFW_CONFIRMATION_QUERY;

  return -1;
}

extern "C" PyObject *
query(PyObject* /*self*/, PyObject *args)
{
  int l_label, l_type;
  char *b_label, *b_type;
  PyObject* inival = NULL;
  PyObject* retval = NULL;

  if (!PyArg_ParseTuple(args, "u#s#|O", &b_label, &l_label,
                        &b_type, &l_type, &inival))
    return NULL;

  TInt error = KErrNone;
  CAknQueryDialog* dlg = NULL;
  TPtr buf(NULL, 0);
  TInt num = 0;
  TReal num_real = 0;
  TTime time;
  TInt rid = rid_by_type(TPtrC8((TUint8*)b_type, l_type));

  switch(rid) {
  case R_APPUIFW_DATA_QUERY:
    if (retval = PyUnicode_FromUnicode(NULL, KAppuifwMaxTextField)) {
      buf.Set(PyUnicode_AsUnicode(retval), 0, KAppuifwMaxTextField);
      if (inival && PyUnicode_Check(inival))
        buf.Copy(PyUnicode_AsUnicode(inival), PyUnicode_GetSize(inival));
      TRAP(error, {
        dlg = CAknTextQueryDialog::NewL(buf, CAknQueryDialog::ENoTone);
        ((CAknTextQueryDialog*)dlg)->SetMaxLength(KAppuifwMaxTextField);
      });
      if (error == KErrNone)
        dlg->SetPredictiveTextInputPermitted(ETrue);
    }
    else
      error = KErrPython;
    break;
  case R_APPUIFW_NUMBER_QUERY:
    if (inival && PyInt_Check(inival))
      num = PyLong_AsLong(inival);
    TRAP(error, {
      dlg = CAknNumberQueryDialog::NewL(num, CAknQueryDialog::ENoTone);
    });
    break;
  case R_APPUIFW_FLOAT_QUERY:
    if (inival && PyFloat_Check(inival))
      num_real = PyFloat_AsDouble(inival);
    TRAP(error, {
      dlg = CAknFloatingPointQueryDialog::NewL(num_real, CAknQueryDialog::ENoTone);
    });
    break;
  case R_APPUIFW_CODE_QUERY:
    if (retval = PyUnicode_FromUnicode(NULL, KAppuifwMaxTextField)) {
      buf.Set(PyUnicode_AsUnicode(retval), 0, KAppuifwMaxTextField);
      TRAP(error, {
        dlg = CAknTextQueryDialog::NewL(buf, CAknQueryDialog::ENoTone);
        ((CAknTextQueryDialog*)dlg)->SetMaxLength(KAppuifwMaxTextField);
      });
    }
    else
      error = KErrPython;
    break;
  case R_APPUIFW_DATE_QUERY:
  case R_APPUIFW_TIME_QUERY:
    time = (rid == R_APPUIFW_DATE_QUERY) ? epoch_local_as_TReal() : 0.0;
    if (inival && PyFloat_Check(inival))
      time += TTimeIntervalSeconds(PyFloat_AsDouble(inival));
    TRAP(error, {
      dlg = CAknTimeQueryDialog::NewL(time, CAknQueryDialog::ENoTone);
    });
    break;
  case R_APPUIFW_CONFIRMATION_QUERY:
    TRAP(error, (dlg = CAknQueryDialog::NewL()));
    break;
  default:
    PyErr_SetString(PyExc_ValueError, "unknown query type");
    return NULL;
  }

  TInt user_response = 0;

  if (error == KErrNone) {
    TRAP(error, {
    dlg->SetPromptL(TPtrC((TUint16 *)b_label, l_label));
    Py_BEGIN_ALLOW_THREADS
      user_response = dlg->ExecuteLD(rid);
    Py_END_ALLOW_THREADS
      });
  }

  if (error != KErrNone) {
    delete dlg;
    Py_XDECREF(retval);
    return SPyErr_SetFromSymbianOSErr(error);
  }
  else if (!user_response) {
    Py_XDECREF(retval);
    Py_INCREF(Py_None);
    return Py_None;
  }
  else {
    switch(rid) {
    case R_APPUIFW_DATA_QUERY:
    case R_APPUIFW_CODE_QUERY:
      PyUnicode_Resize(&retval, buf.Length());
      break;
    case R_APPUIFW_NUMBER_QUERY:
      retval = Py_BuildValue("i", num);
      break;
    case R_APPUIFW_FLOAT_QUERY:
      retval = Py_BuildValue("d", num_real);
      break;
    case R_APPUIFW_DATE_QUERY:
      retval = Py_BuildValue("d", time_as_UTC_TReal(time));
      break;
    case R_APPUIFW_TIME_QUERY:
      retval = Py_BuildValue("d", datetime_as_TReal(time));
      break;
    case R_APPUIFW_CONFIRMATION_QUERY:
    default:
      retval = Py_True;
      Py_INCREF(Py_True);
      break;
    }
    return retval;
  }
}


/*
 *
 * Implementation of appuifw.Icon
 *
 */

struct icon_data {
  TBuf<KMaxFileName> ob_file;
  int ob_bmpId;
  int ob_maskId;
};

struct Icon_object {
  PyObject_VAR_HEAD
  icon_data* icon;
};


extern "C" PyObject *
new_Icon_object(PyObject* /*self*/, PyObject* args)
{
  char *b_file;
  int l_file, bitmapId, maskId;

  if (!PyArg_ParseTuple(args, "u#ii", &b_file, &l_file, &bitmapId, &maskId))
    return NULL;

  if (l_file <=0 || l_file > KMaxFileName) {
    PyErr_SetString(PyExc_TypeError,
            "expected valid file name");
    return NULL;
  }

  TBuf<KMaxFileName> fileName;
  fileName.FillZ(KMaxFileName);
  fileName.Copy((TUint16*)b_file, l_file);
  TParsePtrC p(fileName);
#if SERIES60_VERSION>=28
  if (!((p.Ext().CompareF(_L(".mbm")) == 0) ||
      (p.Ext().CompareF(_L(".mif")) == 0))) {
#else
  if (!(p.Ext().CompareF(_L(".mbm")) == 0)) {
#endif /* SERIES60_VERSION */
        PyErr_SetString(PyExc_TypeError,
            "expected valid icon file");
    return NULL;
  }

  if (bitmapId <0 || maskId <0) {
    PyErr_SetString(PyExc_TypeError,
            "expected valid icon and icon mask indexes");
    return NULL;
  }

  /* Panic occurs e.g. in AknIconUtils if the indeces are bigger than KMaxTInt16 */
  if (bitmapId > KMaxTInt16 || maskId > KMaxTInt16) {
    PyErr_SetString(PyExc_TypeError,
            "expected valid icon and icon mask indexes");
    return NULL;
  }

  Icon_object *io = PyObject_New(Icon_object, &Icon_type);
  if (io == NULL)
    return PyErr_NoMemory();

  io->icon = new icon_data();
  if (io->icon == NULL) {
    PyObject_Del(io);
    return PyErr_NoMemory();
  }

  io->icon->ob_file.FillZ(KMaxFileName);
  io->icon->ob_file.Copy((TUint16*)b_file, l_file);
  io->icon->ob_bmpId = bitmapId;
  io->icon->ob_maskId = maskId;

  return (PyObject *) io;
}


extern "C" {
  static void
  Icon_dealloc(Icon_object *op)
  {
    delete op->icon;
    op->icon = NULL;
    PyObject_Del(op);
  }

  const static PyMethodDef Icon_methods[] = {
    {NULL,              NULL}           // sentinel
  };

  static PyObject *
  Icon_getattr(Icon_object *op, char *name)
  {
    if (!strcmp(name, UICONTROLAPI_NAME)) {
      Py_INCREF(op);
      return PyCObject_FromVoidPtr(op,_uicontrolapi_decref);
    }

    return Py_FindMethod((PyMethodDef*)Icon_methods,
                         (PyObject *)op, name);
  }

  static int
  Icon_setattr(Icon_object * /*op*/, char * /*name*/, PyObject * /*v*/)
  {
    PyErr_SetString(PyExc_AttributeError, "no such attribute");
    return -1;
  }

  static const PyTypeObject c_Icon_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "appuifw.Icon",                            /*tp_name*/
    sizeof(Icon_object),                       /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)Icon_dealloc,                  /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)Icon_getattr,                 /*tp_getattr*/
    (setattrfunc)Icon_setattr,                 /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };
}       // extern "C"



CListBoxCallback::CListBoxCallback(PyObject *aCallback, PyObject *aListBox) {
  iListBox=aListBox;
  iCallback=aCallback;
  Py_XINCREF(iCallback);
}
CListBoxCallback::~CListBoxCallback() {
  Py_XDECREF(iCallback);
  delete iAsyncCallback;
}

void CListBoxCallback::HandleListBoxEventL(CEikListBox* aListBox , TListBoxEvent aEventType) {
  if (iCallback && (aEventType == EEventEnterKeyPressed ||
          aEventType == EEventItemDoubleClicked)) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    /*Increment of reference count of iListbox. It is used not to delete iListBox
      prematurely. Possibly, iCallback is such a
      function (i.e. function from Python script that would change app.body from
      current listbox to something else) that would
      cause deletion of iListBox object.
      Asynchronous callback is later used to call decrefObject() which would
      decrease the reference to iListBox once when HandleListBoxEventL() returns, and
      UI processing is done. This fixes bug 1481966*/
    Py_INCREF(iListBox);
    app_callback_handler(iCallback);
    if (iListBox->ob_refcnt == 1){
      TCallBack cb(&decrefObject, iListBox);
      iAsyncCallback = new (ELeave) CAsyncCallBack(cb, CActive::EPriorityHigh);
      iAsyncCallback->CallBack();
    }
    else {
      Py_DECREF(iListBox);
    }
    PyGILState_Release(gstate);
  }
//  else if (aEventType == EEventItemDraggingActioned) {
//    aListBox->View()->Draw(&aListBox->View()->ViewRect());
//  }
}


struct Listbox_object {
  PyObject_VAR_HEAD
  CEikListBox *ob_control;
  CAppuifwEventBindingArray* ob_event_bindings;
  ControlName control_name;
  ListboxType ob_lb_type;
  CListBoxCallback *ob_listbox_callback;
  CArrayPtrFlat<CGulIcon>* ob_icons;
};

/* An internal helper function */
PyObject *RemoveTabs(PyObject *unicodeString )
{
  _LIT(KEmpty, " ");

  TBuf<(KMaxFileName)> temp;

  temp.Append(PyUnicode_AsUnicode(unicodeString),
                    Min(PyUnicode_GetSize(unicodeString), KMaxFileName));

  TInt res = KErrNotFound;
  res = temp.Find(KSeparatorTab);
  while (res!=KErrNotFound)
  {
    res = temp.Find(KSeparatorTab);
    if ((res != KErrNotFound) ) {
      temp.Replace(res, 1, KEmpty );
    }
  }
  PyObject *newUnicodeObject = Py_BuildValue("u#", temp.Ptr(), temp.Length());

  return newUnicodeObject;
}

/* An internal helper function */
static TInt Listbox_create_itemslist(ListboxType lb_type,
                                     PyObject* list,
                                     CDesCArray*& items_list,
                                     TBool is_popup_style = EFalse,
                     CArrayPtr<CGulIcon>* icons = NULL)
{
  Icon_object *io = NULL;
  TBool items_list_borrowed = ETrue;

  if (!items_list) {
    if (!(items_list = new CDesCArrayFlat(5)))
      return KErrNoMemory;
    else
      items_list_borrowed = EFalse;
  }

  TInt error = KErrNone;
  TBuf<((KMaxFileName+1)*2)> temp;
  int sz = PyList_Size(list);

  CEikonEnv* env = CEikonEnv::Static();

  for (int i = 0; i < sz; i++) {
    if (lb_type == ESingleListbox) {
      PyObject* s = PyList_GetItem(list, i);
      if (!PyUnicode_Check(s))
        error = KErrArgument;
      else {
        if (is_popup_style) {
          temp.Copy(KEmptyString);
        }
        else {
          temp.Copy(KSeparatorTab);
        }
        temp.Append(PyUnicode_AsUnicode(RemoveTabs(s)),
                    Min(PyUnicode_GetSize(s), KMaxFileName));
      }
    }
    else if (lb_type == EDoubleListbox) {
      PyObject* t = PyList_GetItem(list, i);
      if (!PyTuple_Check(t))
        error = KErrArgument;
      else {
        PyObject* s1 = PyTuple_GetItem(t, 0);
        PyObject* s2 = PyTuple_GetItem(t, 1);
        if ((!PyUnicode_Check(s1)) || (!PyUnicode_Check(s2))) {
          error = KErrArgument;
        }
        else {
          if (is_popup_style) {
            temp.Copy(KEmptyString);
          }
          else {
            temp.Copy(KSeparatorTab);
          }

          temp.Append(PyUnicode_AsUnicode(RemoveTabs(s1)),
                      Min(PyUnicode_GetSize(s1), KMaxFileName));
          temp.Append(KSeparatorTab);
          temp.Append(PyUnicode_AsUnicode(RemoveTabs(s2)),
                      Min(PyUnicode_GetSize(s2), KMaxFileName));

        }
      }
    }

    else if (lb_type == ESingleGraphicListbox) {
      PyObject* t = PyList_GetItem(list, i);
      if (!PyTuple_Check(t)) {
        error = KErrArgument;
      }
      else {
        PyObject* s1; // = PyTuple_GetItem(t, 0);
        if (!PyArg_ParseTuple(t, "OO!", &s1, &Icon_type, &io)) {
          error = KErrArgument;
        }
        else {
          if (is_popup_style) {
            temp.Copy(KEmptyString);
          }
          else {
            temp.Copy(KEmptyString);
            temp.AppendNum(i);
            temp.Append(KSeparatorTab);
           }
        temp.Append(PyUnicode_AsUnicode(RemoveTabs(s1)), Min(PyUnicode_GetSize(s1), KMaxFileName));
        }

        // error in argument:
        if (error != KErrNone) {
          SPyErr_SetFromSymbianOSErr(error);
          delete items_list;
          items_list = NULL;
          return error;
        }

        CGulIcon* iIcon = NULL;

#if SERIES60_VERSION>=28
        CFbsBitmap* bitMap = NULL;
        CFbsBitmap* mask = NULL;

        TRAP(error, {
        AknIconUtils::CreateIconL(bitMap, mask, io->icon->ob_file, io->icon->ob_bmpId, io->icon->ob_maskId);
        iIcon = CGulIcon::NewL(bitMap, mask);
        });
#else
        TRAP(error,
        iIcon = env->CreateIconL(io->icon->ob_file, io->icon->ob_bmpId, io->icon->ob_maskId) );
#endif /* SERIES60_VERSION */
        if (error != KErrNone) {
          SPyErr_SetFromSymbianOSErr(error);
          delete items_list;
          items_list = NULL;
          return error;
        }
        TRAP(error, icons->AppendL(iIcon) );
        if (error != KErrNone) {
          SPyErr_SetFromSymbianOSErr(error);
          delete items_list;
          items_list = NULL;
          return error;
        }

      }
    }
    else if (lb_type == EDoubleGraphicListbox) {
      PyObject* t = PyList_GetItem(list, i);
      if (!PyTuple_Check(t)) {
        error = KErrArgument;
      }
      else {
        PyObject* s1; // = PyTuple_GetItem(t, 0);
        PyObject* s2; // = PyTuple_GetItem(t, 1);

        if (!PyArg_ParseTuple(t, "OOO!", &s1, &s2, &Icon_type, &io)) {
          error = KErrArgument;
        }
        else {
          if (is_popup_style) {
            temp.Copy(KEmptyString);
          }
          else {
            temp.Copy(KEmptyString);
            temp.AppendNum(i);
            temp.Append(KSeparatorTab);
          }
          temp.Append(PyUnicode_AsUnicode(RemoveTabs(s1)), Min(PyUnicode_GetSize(s1), KMaxFileName));
          temp.Append(KSeparatorTab);
          temp.Append(PyUnicode_AsUnicode(RemoveTabs(s2)), Min(PyUnicode_GetSize(s2), KMaxFileName));

        }

        // error in argument:
        if (error != KErrNone) {
          SPyErr_SetFromSymbianOSErr(error);
          delete items_list;
          items_list = NULL;
          return error;
        }

        CGulIcon* iIcon = NULL;

#if SERIES60_VERSION>=28
        CFbsBitmap* bitMap = NULL;
        CFbsBitmap* mask = NULL;

        TRAP(error, { AknIconUtils::CreateIconL(
        bitMap, mask, io->icon->ob_file, io->icon->ob_bmpId, io->icon->ob_maskId);
        iIcon = CGulIcon::NewL(bitMap, mask);
        });
#else
        TRAP(error,
        iIcon = env->CreateIconL(io->icon->ob_file, io->icon->ob_bmpId, io->icon->ob_maskId) );
#endif /* SERIES60_VERSION */
        if (error != KErrNone) {
          SPyErr_SetFromSymbianOSErr(error);
          delete items_list;
          items_list = NULL;
          return error;
        }

        TRAP(error, icons->AppendL(iIcon) );
        if (error != KErrNone) {
          SPyErr_SetFromSymbianOSErr(error);
          delete items_list;
          items_list = NULL;
          return error;
        }
      }
    }

    if (error == KErrNone) {
      TRAP(error, items_list->AppendL(temp));
    }

    if ((error != KErrNone) && (!items_list_borrowed)) {
      delete items_list;
      items_list = NULL;
      break;
    }
  }

  return error;
}

void cleanup_listbox(Listbox_object *op)
{
  delete op->ob_listbox_callback;
  delete op->ob_event_bindings;
  delete op->ob_control;
  PyObject_Del(op);
}

extern "C" PyObject *
new_Listbox_object(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  PyObject* list;
  PyObject* cb = NULL;

  if (!PyArg_ParseTuple(args, "O!|O", &PyList_Type, &list, &cb))
    return NULL;

  if (cb && !PyCallable_Check(cb)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  if (PyList_Size(list) <= 0) {
    PyErr_SetString(PyExc_ValueError, "non-empty list expected");
    return NULL;
  }

  Listbox_object *op = PyObject_New(Listbox_object, &Listbox_type);
  if (op == NULL)
    return PyErr_NoMemory();
  op->ob_listbox_callback = NULL;
  op->ob_event_bindings = NULL;
  op->ob_control = NULL;
  op->control_name = EListbox;
  if (!(op->ob_event_bindings = new CAppuifwEventBindingArray())) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }

  if (PyTuple_Check(PyList_GetItem(list, 0))) {

    switch (PyTuple_Size(PyList_GetItem(list, 0))) {
      case 2: {
        PyObject* secondObj = PyTuple_GetItem(PyList_GetItem(list, 0), 1);
        if (PyUnicode_Check(secondObj) )
          op->ob_lb_type = EDoubleListbox;
        else
          op->ob_lb_type = ESingleGraphicListbox;
        break;
      }
      case 3:
        op->ob_lb_type = EDoubleGraphicListbox;
        break;
      default:
        PyErr_SetString(PyExc_ValueError, "tuple must include 2 or 3 elements");
        return NULL;
    }
  }
  else {
    op->ob_lb_type = ESingleListbox;
  }


  if (error == KErrNone) {
    switch(op->ob_lb_type) {
    case ESingleListbox:
      op->ob_control = new CAknSingleStyleListBox();
      break;
    case EDoubleListbox:
      op->ob_control = new CAknDoubleStyleListBox();
      break;
    case ESingleGraphicListbox:
      op->ob_control = new CAknSingleGraphicStyleListBox();
      break;
    case EDoubleGraphicListbox:
      op->ob_control = new CAknDoubleLargeStyleListBox();
      break;
    }

    if (op->ob_control == NULL) {
      PyObject_Del(op);
      return PyErr_NoMemory();
    }

    op->ob_icons = NULL;
  }

  CAmarettoAppUi* pyappui = MY_APPUI;
  CCoeControl* cont = pyappui->iContainer;

  if (error == KErrNone) {
    TRAP(error, {
      op->ob_control->SetContainerWindowL(*cont);
      if (op->ob_lb_type == ESingleListbox)
        ((CAknSingleStyleListBox*)op->ob_control)->
          ConstructL(cont, EAknListBoxSelectionList);
      else
        if (op->ob_lb_type == EDoubleListbox)
          ((CAknDoubleStyleListBox*)op->ob_control)->
            ConstructL(cont, EAknListBoxSelectionList);
      else
        if (op->ob_lb_type == ESingleGraphicListbox)
          ((CAknSingleGraphicStyleListBox*)op->ob_control)->
            ConstructL(cont, EAknListBoxSelectionList);
      else
        if (op->ob_lb_type == EDoubleGraphicListbox)
          ((CAknDoubleLargeStyleListBox*)op->ob_control)->
            ConstructL(cont, EAknListBoxSelectionList);
      });
  }

  CDesCArray *items_list = NULL;

  if ( (op->ob_lb_type==ESingleGraphicListbox) || (op->ob_lb_type==EDoubleGraphicListbox) )
    TRAP(error,
     op->ob_icons = new(ELeave) CArrayPtrFlat<CGulIcon>(5));

  if (error == KErrNone)
    error = Listbox_create_itemslist(op->ob_lb_type, list, items_list, NULL, op->ob_icons);

  if (error == KErrNone) {
    if (op->ob_lb_type == ESingleListbox)
      ((CAknSingleStyleListBox*)op->ob_control)->
        Model()->SetItemTextArray(items_list); /* ownership transfer */
    else
      if (op->ob_lb_type == EDoubleListbox)
        ((CAknDoubleStyleListBox*)op->ob_control)->
          Model()->SetItemTextArray(items_list);
    else
      if (op->ob_lb_type == ESingleGraphicListbox) {
          if (op->ob_icons != NULL)
            ((CAknColumnListBox*)op->ob_control)->ItemDrawer()->ColumnData()->SetIconArray(op->ob_icons);
          ((CAknSingleGraphicStyleListBox*)op->ob_control)->Model()->SetItemTextArray(items_list);
      }
    else
      if (op->ob_lb_type == EDoubleGraphicListbox) {
          if (op->ob_icons != NULL)
            ((CEikFormattedCellListBox*)op->ob_control)->ItemDrawer()->
            ColumnData()->SetIconArray(op->ob_icons);
        ((CAknDoubleLargeStyleListBox*)op->ob_control)->Model()->SetItemTextArray(items_list);
      }

    TRAP(error, op->ob_listbox_callback = new (ELeave) CListBoxCallback(cb, (PyObject *)op));
    if (error != KErrNone) {
      cleanup_listbox(op);
      return SPyErr_SetFromSymbianOSErr(error);
    }
    else {
      ((CEikListBox*)op->ob_control)->SetListBoxObserver(op->ob_listbox_callback);
      TRAP(error, {
        ((CEikListBox*)op->ob_control)->CreateScrollBarFrameL(ETrue);
        ((CEikListBox*)op->ob_control)->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,
                                                                CEikScrollBarFrame::EAuto);
      });
      if (error != KErrNone) {
        cleanup_listbox(op);
        return SPyErr_SetFromSymbianOSErr(error);
      }
      return (PyObject *) op;
    }
  } else {
      Py_DECREF(op);
      return SPyErr_SetFromSymbianOSErr(error);
  }
}

extern "C" PyObject*
Listbox_index(Listbox_object *self)
{
  return Py_BuildValue("i", self->ob_control->CurrentItemIndex());
}

extern "C" PyObject*
Listbox_set_list(Listbox_object *self, PyObject *args)
{
  TInt error = KErrNone;
  int current = 0;
  PyObject* list;

  if (!PyArg_ParseTuple(args, "O!|i", &PyList_Type, &list, &current))
    return NULL;

  if (PyList_Size(list) <= 0) {
    PyErr_SetString(PyExc_ValueError, "non-empty list expected");
    return NULL;
  }

  if (PyTuple_Check(PyList_GetItem(list, 0))) {
    if ( (self->ob_lb_type != EDoubleListbox) && (self->ob_lb_type != ESingleGraphicListbox) && (self->ob_lb_type != EDoubleGraphicListbox) ) {
      PyErr_SetString(PyExc_ValueError, "Listbox type mismatch");
      return NULL;
    }
  }
  else {
    if (self->ob_lb_type != ESingleListbox) {
      PyErr_SetString(PyExc_ValueError, "Listbox type mismatch");
      return NULL;
    }
  }

  if (current) {
    int sz = PyList_Size(list);
    if (current >= sz)
      current = sz-1;
  }

  CDesCArray *items_list = NULL;
  self->ob_icons = NULL;

  if ( (self->ob_lb_type==ESingleGraphicListbox) || (self->ob_lb_type==EDoubleGraphicListbox) )
    TRAP(error,
     self->ob_icons = new(ELeave) CArrayPtrFlat<CGulIcon>(5));

  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);

  error = Listbox_create_itemslist(self->ob_lb_type, list, items_list, NULL, self->ob_icons);

  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
  else {
    if (self->ob_lb_type == ESingleListbox)
      ((CAknSingleStyleListBox*)self->ob_control)->
        Model()->SetItemTextArray(items_list);
    else
      if (self->ob_lb_type == EDoubleListbox)
        ((CAknDoubleStyleListBox*)self->ob_control)->
          Model()->SetItemTextArray(items_list);
      else
        if (self->ob_lb_type == ESingleGraphicListbox) {
      if (self->ob_icons != NULL)
        ((CAknColumnListBox*)self->ob_control)->ItemDrawer()->ColumnData()->SetIconArray(self->ob_icons);
          ((CAknSingleGraphicStyleListBox*)self->ob_control)->Model()->SetItemTextArray(items_list);
    }
    else
      if (self->ob_lb_type == EDoubleGraphicListbox) {
      CTextListBoxModel* model;
        model = ((CAknDoubleLargeStyleListBox*)self->ob_control)->Model();
        TInt noOfItems = (model->NumberOfItems());
        MDesCArray* textArray = ((CAknDoubleLargeStyleListBox*)self->ob_control)->Model()->ItemTextArray();
        CDesCArray* itemList = static_cast<CDesCArray*>(textArray);
        itemList->Delete(0, noOfItems);
        ((CEikFormattedCellListBox*)self->ob_control)->ItemDrawer()->ColumnData()->IconArray()->ResetAndDestroy();
        delete ((CEikFormattedCellListBox*)self->ob_control)->ItemDrawer()->ColumnData()->IconArray();
        ((CAknDoubleLargeStyleListBox*)self->ob_control)->Model()->SetItemTextArray(items_list);
        if (self->ob_icons != NULL)
                ((CEikFormattedCellListBox*)self->ob_control)->ItemDrawer()->ColumnData()->SetIconArray(self->ob_icons);
      }

    self->ob_control->Reset();
    if (current)
      self->ob_control->SetCurrentItemIndex(current);
    MY_APPUI->RefreshHostedControl();

    Py_INCREF(Py_None);
    return Py_None;
  }
}

extern "C" PyObject *
Listbox_bind(Listbox_object *self, PyObject* args)
{
  int key_code;
  PyObject* c;

  if (!PyArg_ParseTuple(args, "iO", &key_code, &c))
    return NULL;

  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  SAppuifwEventBinding bind_info;

  bind_info.iType = SAmarettoEventInfo::EKey;
  bind_info.iKeyEvent.iCode = key_code;
  bind_info.iKeyEvent.iModifiers = 0;
  bind_info.iCb = c;
  Py_XINCREF(c);

  TRAPD(error, self->ob_event_bindings->InsertEventBindingL(bind_info));

  if (error != KErrNone)
    Py_XDECREF(c);

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" {
  static void
  Listbox_dealloc(Listbox_object *op)
  {
    delete op->ob_listbox_callback;
    op->ob_listbox_callback = NULL;

    delete op->ob_event_bindings;
    op->ob_event_bindings = NULL;
    delete op->ob_control;
    op->ob_control = NULL;
    op->ob_icons = NULL;

    PyObject_Del(op);
  }

  const static PyMethodDef Listbox_methods[] = {
    {"current", (PyCFunction)Listbox_index, METH_NOARGS},
    {"set_list", (PyCFunction)Listbox_set_list, METH_VARARGS},
    {"bind", (PyCFunction)Listbox_bind, METH_VARARGS},
    {NULL,              NULL}           // sentinel
  };

  static PyObject *
  Listbox_getattr(Listbox_object *op, char *name)
  {
    if (!strcmp(name, UICONTROLAPI_NAME)) {
      Py_INCREF(op);
      return PyCObject_FromVoidPtr(op,_uicontrolapi_decref);
    }
#if S60_VERSION>=30
    if (!strcmp(name, "size")) {
      TRect rect;
      rect = op->ob_control->HighlightRect();
      return Py_BuildValue("(ii)", rect.Width(),
                                   rect.Height());
    }

    if (!strcmp(name, "position")) {
      TRect rect;
      rect = op->ob_control->HighlightRect();
      return Py_BuildValue("(ii)",rect.iTl.iX,
                                   rect.iTl.iY);
    }
#endif
    return Py_FindMethod((PyMethodDef*)Listbox_methods,
                         (PyObject *)op, name);
  }


  static const PyTypeObject c_Listbox_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "appuifw.Listbox",                         /*tp_name*/
    sizeof(Listbox_object),                    /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)Listbox_dealloc,               /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)Listbox_getattr,              /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };
}       // extern "C"

/*
 *
 * Implementation of appuifw.Content_handler
 *
 */

class Content_handler_data;

struct Content_handler_object {
  PyObject_VAR_HEAD
  Content_handler_data *ob_data;
  PyObject* ob_cb;
};

class CContent_handler_undertaker : public CActive
{
 public:
  CContent_handler_undertaker(PyObject* aOb):
    CActive(EPriorityStandard), iOb(aOb) {
    CActiveScheduler::Add(this);
  }

  void Start() {
    iStatus = KRequestPending;
    SetActive();
    //iStatus = KErrNone; // Doing this will panic the 3.0 emulator with E32USER-CBase 46 "Stray signal".
    TRequestStatus* pstatus = &iStatus;
    RThread().RequestComplete(pstatus, 0);
  }

 private:
  void DoCancel() {;}
  void RunL() {
    PyGILState_STATE gstate = PyGILState_Ensure();
    Py_DECREF(iOb);
    PyGILState_Release(gstate);
  }
  PyObject* iOb;
};

#ifdef EKA2
NONSHARABLE_CLASS(Content_handler_data) : public MAknServerAppExitObserver
#else
class Content_handler_data : public MApaEmbeddedDocObserver
#endif /*EKA2*/
{
public:
  static Content_handler_data* New(Content_handler_object* aOwner);
  Content_handler_data(Content_handler_object* aOwner):iOwner(aOwner){;}
  virtual ~Content_handler_data();
  void ConstructL();
#ifdef EKA2
  void HandleServerAppExit(TInt aReason);
#else
  void NotifyExit(TExitMode aMode);
#endif /*EKA2*/
  CDocumentHandler* iDocHandler;
  CContent_handler_undertaker* iUndertaker;
  Content_handler_object* iOwner;
};

Content_handler_data*
Content_handler_data::New(Content_handler_object* aOwner)
{
  Content_handler_data* self = new Content_handler_data(aOwner);
  if (self != NULL) {
    TRAPD(error, self->ConstructL());
    if (error != KErrNone) {
      delete self;
      self = NULL;
    }
  }
  return self;
}

void Content_handler_data::ConstructL()
{
  iUndertaker =
    new (ELeave) CContent_handler_undertaker((PyObject*)iOwner);

#ifdef EKA2
  iDocHandler = CDocumentHandler::NewL();
  iDocHandler->SetExitObserver((MAknServerAppExitObserver*)this);
#else
  iDocHandler =
    CDocumentHandler::NewL((CEikProcess*)
                           ((CEikonEnv::Static())->
                            EikAppUi()->Application()->Process()));
  iDocHandler->SetExitObserver(this);
#endif /*EKA*/
}

#ifdef EKA2
void Content_handler_data::HandleServerAppExit(TInt /* aReason */)
{
  if (iOwner->ob_cb) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject* tmp_r = NULL;
    tmp_r = PyEval_CallObject(iOwner->ob_cb, NULL);
    Py_XDECREF(tmp_r);
    PyGILState_Release(gstate);
  }
  iUndertaker->Start();
  return;
}
#else
void Content_handler_data::NotifyExit(TExitMode /* aMode */)
{
  if (iOwner->ob_cb) {
    PyGILState_STATE gstate = PyGILState_Ensure();
    PyObject* tmp_r = NULL;
    tmp_r = PyEval_CallObject(iOwner->ob_cb, NULL);
    Py_XDECREF(tmp_r);
    PyGILState_Release(gstate);
  }
  iUndertaker->Start();
  return;
}
#endif /*EKA2*/

Content_handler_data::~Content_handler_data()
{
  delete iDocHandler;
  delete iUndertaker;
}

extern "C" PyObject *
new_Content_handler_object(PyObject* /*self*/, PyObject* args)
{
  PyObject *c = NULL;
  if (!PyArg_ParseTuple(args, "|O", &c))
      return NULL;
  if (c && !PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  Content_handler_object *op = PyObject_New(Content_handler_object,
                                            &Content_handler_type);
  if (op == NULL)
    return PyErr_NoMemory();

  op->ob_data = Content_handler_data::New(op);
  if (op->ob_data == NULL) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }
  op->ob_cb = c;
  Py_XINCREF(c);
  return (PyObject *) op;
}

static PyObject* Content_handler_open(Content_handler_object *self,
                                      PyObject* args,
                                      int f_embed)
{
  PyObject *it, *s;

  if (!PyArg_ParseTuple(args, "O", &it) ||
      !(s = PyUnicode_FromObject(it)))
    return NULL;

  TPtrC name(PyUnicode_AsUnicode(s), PyUnicode_GetSize(s));
  TParse p;
  p.Set(name, NULL, NULL);
  int is_py = (p.Ext() == _L(".py"));

  if (f_embed && is_py) {
    Py_DECREF(s);
    PyErr_SetString(PyExc_TypeError,
                    "cannot currently embed Python within Python");
    return NULL;
  }

  TDataType empty;
  TInt error;

  if (f_embed) {
    Py_BEGIN_ALLOW_THREADS
    TRAP(error,
         self->ob_data->iDocHandler->OpenFileEmbeddedL(name, empty));
    Py_END_ALLOW_THREADS
    if (error == KErrNone)
      Py_INCREF(self);
  }
  else {
#ifdef USE_LAUNCHER_FOR_APP
    if (is_py) {
#if defined(__WINS__)
      RThread proc;
      RLibrary lib;
      HBufC* pname = name.Alloc();
      error = lib.Load(_L("\\system\\programs\\Python_launcher.dll"));
      if (error == KErrNone) {
        TThreadFunction func = (TThreadFunction)(lib.Lookup(1));
        error =
          proc.Create(_L("Py"), func, 0x1000, (TAny*) pname, &lib,
                      RThread().Heap(), 0x1000, 0x100000, EOwnerProcess);
        lib.Close();
      }
      else
        delete pname;
#else
      TFileName app_name =
        CEikonEnv::Static()->EikAppUi()->Application()->AppFullName();
      p.Set(_L("\\system\\programs\\Python_launcher.exe"),
            &app_name, NULL);
      RProcess proc;
      error = proc.Create(p.FullName(), name);
#endif
      if (error == KErrNone) {
        proc.Resume();
        proc.Close();
      }
    }
    else {
#endif /* USE_LAUNCHER_FOR_APP */
      TRAP(error, self->ob_data->iDocHandler->OpenFileL(name, empty));
#ifdef USE_LAUNCHER_FOR_APP
    }
#endif
  }

  Py_DECREF(s);
  PyObject *r = NULL;

  switch(error) {
  case KErrNone:
    Py_INCREF(Py_None);
    r = Py_None;
    break;
  case KBadMimeType:
    PyErr_SetString(PyExc_TypeError, "bad mime type");
    break;
  case KMimeNotSupported:
    PyErr_SetString(PyExc_TypeError, "mime type not supported");
    break;
  case KNullContent:
    PyErr_SetString(PyExc_TypeError, "empty content");
    break;
  case KExecNotAllowed:
    PyErr_SetString(PyExc_TypeError, "executables not allowed");
    break;
  default:
    r = SPyErr_SetFromSymbianOSErr(error);
    break;
  }

  return r;
}

extern "C" PyObject *
Content_handler_open_emb(Content_handler_object *self, PyObject* args)
{
  return Content_handler_open(self, args, 1);
}

extern "C" PyObject *
Content_handler_open_proc(Content_handler_object *self, PyObject* args)
{
  return Content_handler_open(self, args, 0);
}

extern "C" {
  static void
  Content_handler_dealloc(Content_handler_object *op)
  {
    delete op->ob_data;
    op->ob_data = NULL;
    Py_XDECREF(op->ob_cb);
    PyObject_Del(op);
  }

  const static PyMethodDef Content_handler_methods[] = {
    {"open", (PyCFunction)Content_handler_open_emb, METH_VARARGS},
    {"open_standalone",
     (PyCFunction)Content_handler_open_proc, METH_VARARGS},
    {NULL,              NULL}           // sentinel
  };

  static PyObject *
  Content_handler_getattr(Content_handler_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)Content_handler_methods,
                         (PyObject *)op, name);
  }


  static const PyTypeObject c_Content_handler_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "appuifw.Content_handler",                 /*tp_name*/
    sizeof(Content_handler_object),            /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)Content_handler_dealloc,       /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)Content_handler_getattr,      /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };
}       // extern "C"

/*
 *
 * Implementation of appuifw.InfoPopup
 *
 */
#if SERIES60_VERSION>=30
struct InfoPopup_data {
  CAknInfoPopupNoteController* controller;
};

struct InfoPopup_object {
  PyObject_VAR_HEAD
  InfoPopup_data* info_popup;
};

extern "C" PyObject *
new_InfoPopup_object(PyObject* /*self*/, PyObject* args)
{
  InfoPopup_object *ip = PyObject_New(InfoPopup_object, &InfoPopup_type);
  if (ip == NULL)
    return PyErr_NoMemory();

  ip->info_popup = new InfoPopup_data();
  if (ip->info_popup == NULL) {
    PyObject_Del(ip);
    return PyErr_NoMemory();
  }

  TRAPD(error, ip->info_popup->controller = CAknInfoPopupNoteController::NewL());
  if (error != KErrNone) {
    PyObject_Del(ip);
    return SPyErr_SetFromSymbianOSErr(error);
  }

  return (PyObject *) ip;
}


extern "C" PyObject *
InfoPopup_show(InfoPopup_object *ip, PyObject *args)
{
  PyObject* text = NULL;
  TInt error = KErrNone;
  TInt xSize = 0;
  TInt ySize = 0;
  TInt timeShown = 5000; // 5 seconds default
  TInt timeBefore = 0; // 0 seconds default before shown
  TInt alignment = EHLeftVTop;

  if (!PyArg_ParseTuple(args, "U|(ii)iii", &text, &xSize, &ySize,
                                        &timeShown, &timeBefore, &alignment))
    return NULL;

  if (xSize<0 || ySize<0 || timeShown<0 || timeBefore<0){
    PyErr_SetString(PyExc_ValueError,
                    "Info pop-up, value must be positive or zero");
    return NULL;
  }

  TPtrC textPtr((TUint16*) PyUnicode_AsUnicode(text), PyUnicode_GetSize(text));

  TRAP(error, ip->info_popup->controller->SetTextL(textPtr));
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }

  ip->info_popup->controller->SetTimeDelayBeforeShow(timeBefore);
  ip->info_popup->controller->SetTimePopupInView(timeShown);
  ip->info_popup->controller->SetPositionAndAlignment(TPoint(xSize, ySize),
                                                     (TGulAlignmentValue)alignment);
  ip->info_popup->controller->ShowInfoPopupNote();

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
InfoPopup_hide(InfoPopup_object *ip, PyObject *args)
{
  ip->info_popup->controller->HideInfoPopupNote();
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" {
  static void
  InfoPopup_dealloc(InfoPopup_object *ip)
  {
    delete ip->info_popup->controller;
    ip->info_popup->controller = NULL;

    delete ip->info_popup;
    ip->info_popup = NULL;
    PyObject_Del(ip);
  }

  const static PyMethodDef InfoPopup_methods[] = {
    {"show", (PyCFunction)InfoPopup_show, METH_VARARGS},
    {"hide", (PyCFunction)InfoPopup_hide, METH_NOARGS},
    {NULL,              NULL}           // sentinel
  };

  static PyObject *
  InfoPopup_getattr(InfoPopup_object *op, char *name)
  {
    if (!strcmp(name, UICONTROLAPI_NAME)) {
      Py_INCREF(op);
      return PyCObject_FromVoidPtr(op,_uicontrolapi_decref);
    }

    return Py_FindMethod((PyMethodDef*)InfoPopup_methods,
                         (PyObject *)op, name);
  }

  static int
  InfoPopup_setattr(InfoPopup_object * /*op*/, char * /*name*/, PyObject * /*v*/)
  {
    PyErr_SetString(PyExc_AttributeError, "no such attribute");
    return -1;
  }

  static const PyTypeObject c_InfoPopup_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "appuifw.InfoPopup",                       /*tp_name*/
    sizeof(InfoPopup_object),                  /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)InfoPopup_dealloc,             /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)InfoPopup_getattr,            /*tp_getattr*/
    (setattrfunc)InfoPopup_setattr,            /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };
}       // extern "C"
#endif /* SERIES60_VERSION */

/*
 *
 * Implementation of appuifw.note()
 *
 */

extern "C" PyObject *
note(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  int l_text=0, l_type=4;
  char *b_text=NULL, *b_type="info";
  TInt global_note = 0;

  if (!PyArg_ParseTuple(args, "u#|s#i", &b_text, &l_text,
                        &b_type, &l_type, &global_note))
    return NULL;

  TPtrC8 type_string((TUint8*)b_type, l_type);
  TPtrC note_text((TUint16 *)b_text, l_text);

  if (global_note == 0) {

    CAknResourceNoteDialog* dlg = NULL;

    if (type_string.Compare(KErrorNoteType) == 0)
      dlg = new CAknErrorNote(ETrue);
    else if (type_string.Compare(KInformationNoteType) == 0)
      dlg = new CAknInformationNote(ETrue);
    else if (type_string.Compare(KConfirmationNoteType) == 0)
      dlg = new CAknConfirmationNote(ETrue);
    else {
      PyErr_SetString(PyExc_ValueError, "unknown note type");
      return NULL;
    }

    if (dlg == NULL)
      return PyErr_NoMemory();

    Py_BEGIN_ALLOW_THREADS
    TRAP(error, dlg->ExecuteLD(note_text));
    Py_END_ALLOW_THREADS

  } else {

    TBool error_note_type = EFalse;

    TRAP(error, {

      CAknGlobalNote* dlg = CAknGlobalNote::NewLC();

      if (type_string.Compare(KErrorNoteType) == 0) {
        Py_BEGIN_ALLOW_THREADS
        dlg->ShowNoteL(EAknGlobalErrorNote, note_text);
        Py_END_ALLOW_THREADS
      }
      else if (type_string.Compare(KInformationNoteType) == 0) {
        Py_BEGIN_ALLOW_THREADS
        dlg->ShowNoteL(EAknGlobalInformationNote, note_text);
        Py_END_ALLOW_THREADS
      }
      else if (type_string.Compare(KConfirmationNoteType) == 0) {
        Py_BEGIN_ALLOW_THREADS
        dlg->ShowNoteL(EAknGlobalConfirmationNote, note_text);
        Py_END_ALLOW_THREADS
      }
      else {
        error_note_type = ETrue;
      }

      CleanupStack::PopAndDestroy(dlg);

    });

    if (error_note_type) {
      PyErr_SetString(PyExc_ValueError, "unknown note type");
      return NULL;
    }
  }

  RETURN_ERROR_OR_PYNONE(error);
}

/*
 *
 * Implementation of appuifw.Multi_line_data_query_dialog
 *
 */

extern "C" PyObject *
Multi_line_data_query_dialog(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  TInt resp = 0;
  int l1, l2;
  char *b1, *b2;

  if (!PyArg_ParseTuple(args, "u#u#", &b1, &l1, &b2, &l2))
    return NULL;

  CAknMultiLineDataQueryDialog *dlg = NULL;
  TBuf<KAppuifwMaxTextField> text1;
  TBuf<KAppuifwMaxTextField> text2;

  TRAP(error, {
    dlg = CAknMultiLineDataQueryDialog::NewL(text1, text2,
                                             CAknQueryDialog::ENoTone);
    TPtrC prompt1((TUint16 *)b1, l1);
    TPtrC prompt2((TUint16 *)b2, l2);
    dlg->SetPromptL(prompt1, prompt2);
    Py_BEGIN_ALLOW_THREADS
    resp = dlg->ExecuteLD(R_APPUIFW_MULTI_LINE_DATA);
    dlg = NULL;
    Py_END_ALLOW_THREADS
  });

  if (error != KErrNone) {
    delete dlg;
    return SPyErr_SetFromSymbianOSErr(error);
  }
  else {
    if (resp)
      return Py_BuildValue("u#u#", text1.Ptr(), text1.Length(),
                           text2.Ptr(), text2.Length());
    else {
      Py_INCREF(Py_None);
      return Py_None;
    }
  }
}

/*
 *
 * Implementation of appuifw.popup_menu
 *
 */

extern "C" PyObject *
popup_menu(PyObject* /*self*/, PyObject *args)
{
  TInt error = KErrNone;
  PyObject* list;
  int l = 0;
  char *b = NULL;
  ListboxType lb_type;

  if (!PyArg_ParseTuple(args, "O!|u#", &PyList_Type, &list, &b, &l))
    return NULL;

  CEikTextListBox* lb = NULL;
  if (PyTuple_Check(PyList_GetItem(list, 0))) {
    lb_type = EDoubleListbox;
    lb = new CAknDoublePopupMenuStyleListBox;
  } else {
    lb_type = ESingleListbox;
    lb = new CAknSinglePopupMenuStyleListBox;
  }

  if (lb == NULL)
    return PyErr_NoMemory();

  CAknPopupList* popupList = NULL;

  TRAP(error, {
    popupList = CAknPopupList::NewL(lb, R_AVKON_SOFTKEYS_OK_BACK,
                                    AknPopupLayouts::EMenuWindow);
    lb->ConstructL(popupList, CEikListBox::ELeftDownInViewRect);
    lb->CreateScrollBarFrameL(ETrue);
    lb->ScrollBarFrame()->
      SetScrollBarVisibilityL(CEikScrollBarFrame::EOff,
                              CEikScrollBarFrame::EAuto);
    CDesCArray* a = (CDesCArray*)lb->Model()->ItemTextArray();
    User::LeaveIfError(Listbox_create_itemslist(lb_type, list,
                        a, ETrue));
    lb->HandleItemAdditionL();
    if (b)
      popupList->SetTitleL(TPtrC((TUint16 *)b, l));
  });

  PyObject* retval = NULL;

  if (error == KErrNone) {
    TInt resp = 0;
    Py_BEGIN_ALLOW_THREADS
    TRAP(error, (resp = popupList->ExecuteLD()));
    Py_END_ALLOW_THREADS
    if (error == KErrNone)
      retval =
        (resp ? Py_BuildValue("i", lb->CurrentItemIndex()) : Py_None);
  }

  if (error != KErrNone) {
    delete (CBase*)popupList;
    retval = SPyErr_SetFromSymbianOSErr(error);
  }

  delete lb;
  if (retval == Py_None) Py_INCREF(Py_None);
  return retval;
}


/*
 *
 * Implementation of appuifw.available_fonts()
 *
 */

extern "C" PyObject *
available_fonts(PyObject* /*self*/)
{
  TInt error = KErrNone;
  TTypefaceSupport iTypefaceSupport;
  CCoeEnv* iCoeEnv;

  CAmarettoAppUi* pyappui = MY_APPUI;
  CCoeControl* control = pyappui->iContainer;
  iCoeEnv = control->ControlEnv();

  TInt fontTotalNumber = iCoeEnv->ScreenDevice()->NumTypefaces();

  if (fontTotalNumber <= 0) {
    PyErr_SetString(PyExc_TypeError, "No fonts available on device");
    return NULL;
  }

  PyObject* list = PyList_New(fontTotalNumber);

  for (int i=0; i<fontTotalNumber; i++) {

    TBuf<KMaxTypefaceNameLength> fontName;
    fontName.FillZ(KMaxTypefaceNameLength);

    iCoeEnv->ScreenDevice()->TypefaceSupport(iTypefaceSupport, i);
    fontName = iTypefaceSupport.iTypeface.iName;

    if (fontName.Ptr() != NULL) {
      error = PyList_SetItem(list, i, (Py_BuildValue("u#", fontName.Ptr(), fontName.Length())) );

      if (error!=KErrNone) {
    SPyErr_SetFromSymbianOSErr(error);
    return NULL;
      }
    }
  }

  return list;
}

extern "C" PyObject *
touch_enabled(PyObject* /*self*/)
{
  PyObject *ret = Py_False;

  if (touch_enabled_flag)
      ret = Py_True;

  Py_INCREF(ret);
  return ret;
}

/*
 *
 * Implementation of appuifw.Text
 *
 */

#define KNORMAL     0
#define KBOLD       0x001
#define KUNDERLINE  0x002
#define KITALIC     0x004
#define KSTRIKETHROUGH  0x008

#define KHIGHLIGHTNONE      0
#define KHIGHLIGHTROUNDED   0x010
#define KHIGHLIGHTSHADOW    0x020
#define KHIGHLIGHTNORMAL    0x040

#define HL_MASK         0x070


//class CAppuifwText : public CEikGlobalTextEditor
#ifndef EKA2
class CAppuifwText : public CEikRichTextEditor
#else
NONSHARABLE_CLASS(CAppuifwText) : public CEikRichTextEditor
#endif
{
public:
  CAppuifwText() {}
  virtual void ConstructL(const TRect& aRect,
                          const CCoeControl* aParent);
  virtual ~CAppuifwText();

  void ReadL(PyObject*& aRval, TInt aPos = 0, TInt aLen = (-1));
  void WriteL(const TDesC& aText);
  void DelL(TInt aPos, TInt aLen);
  TInt Len() {
    return RichText()->DocumentLength();
  }
  void ClearL() {
    RichText()->Reset();
    HandleChangeL(0);
  }
  TInt SetPos(TInt aPos);
  TInt GetPos() {
    return CursorPos();
  }
  static TBool CheckRange(TInt& aPos, TInt& aLen, TInt aDocLen);

  void SetBold(TBool aOpt);
  void SetUnderline(TBool aOpt);
  void SetPosture(TBool aOpt);
  void SetStrikethrough(TBool aOpt);
  void SetHighlight(TInt mode);
  void SetStyle(TInt aStyle) { iCurrentStyle = aStyle; };
  TInt Style() { return iCurrentStyle; };
  void SetTextColor(TRgb color);
  TRgb TextColor() { return iCharFormat.iFontPresentation.iTextColor; };
  void SetHighlightColor(TRgb color);
  TRgb HighlightColor() { return iCharFormat.iFontPresentation.iHighlightColor; };

  void SetFont(TFontSpec&);
  TFontSpec& Font() { return iCharFormat.iFontSpec; }
  void SizeChanged();

private:
    void UpdateCharFormatLayerL();
    void HandleChangeL(TInt aNewCursorPos) {
    // TODO: Use SetAmountToFormat to improve performance?
    HandleTextChangedL();
    SetCursorPosL(aNewCursorPos, EFalse);
    //if (!IsVisible()) {   // Otherwise we occasionally pop up unwanted
    //  int in_interpreter=(_PyThreadState_Current == PYTHON_TLS->thread_state);
    //  if (!in_interpreter)
    //    PyEval_RestoreThread(PYTHON_TLS->thread_state);
    //  // The interpreter state must be valid to use the MY_APPUI macro.
    //  CAmarettoAppUi* pyappui = MY_APPUI;
    //  if (!in_interpreter)
    //    PyEval_SaveThread();
    //  pyappui->RefreshHostedControl();
    //}
  }

  TCharFormatMask iCharFormatMask;
  TCharFormat iCharFormat;
  TInt iCurrentStyle;
  CCharFormatLayer *iMyCharFormatLayer;
  CParaFormatLayer *iMyParaFormatLayer;
};


CAppuifwText::~CAppuifwText()
{
  //  iCoeEnv->ReleaseScreenFont(aFont); XXX necessary?
    delete iMyCharFormatLayer;
    delete iMyParaFormatLayer;
}

void CAppuifwText::ConstructL(const TRect& /* aRect */,
                              const CCoeControl* aParent)
{
  CEikRichTextEditor::ConstructL(aParent, 0, 0,
                 EEikEdwinInclusiveSizeFixed|
                 EEikEdwinNoAutoSelection|
                 EEikEdwinAlwaysShowSelection, 0, 0);

// If scrollbar is wanted to be used with the editor
// the scrollbar functionality must be linked to texteditor content.
/*
  CreateScrollBarFrameL();
  ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EAuto,
                                         CEikScrollBarFrame::EAuto);
*/
  ActivateL();

  iMyCharFormatLayer = CCharFormatLayer::NewL();
  iCurrentStyle = KNORMAL;
  iCharFormat.iFontPresentation.iTextColor = TRgb(0,128,0); // Default text color: dark green.
  iCharFormat.iFontPresentation.iHighlightColor = TRgb(180,180,180); // Default highlight color: gray.
  //iCharFormatMask.SetAll();
  iCharFormatMask.SetAttrib(EAttFontHeight);
  iCharFormatMask.SetAttrib(EAttFontPosture);
  iCharFormatMask.SetAttrib(EAttFontStrokeWeight);
  iCharFormatMask.SetAttrib(EAttFontPrintPos);
//  iCharFormatMask.SetAttrib(EAttFontTypeface);
//  iCharFormatMask.SetAttrib(EAttFontBitmapStyle);
  iCharFormatMask.SetAttrib(EAttColor);

  iCharFormatMask.SetAttrib(EAttFontHighlightColor);

  iMyCharFormatLayer->SetL(iCharFormat, iCharFormatMask);
  RichText()->SetGlobalCharFormat(iMyCharFormatLayer);


  /* Set the global paragraph format. This should be more effective than setting it on every insert. */
  iMyParaFormatLayer = CParaFormatLayer::NewL();
  CParaFormat paraFormat;
  TParaFormatMask paraFormatMask;
  paraFormatMask.SetAttrib(EAttLineSpacing);
  paraFormatMask.SetAttrib(EAttLineSpacingControl);
  paraFormat.iLineSpacingControl = CParaFormat::ELineSpacingAtLeastInTwips;
  paraFormat.iLineSpacingInTwips = 30;
  iMyParaFormatLayer->SetL(&paraFormat, paraFormatMask);
  RichText()->SetGlobalParaFormat(iMyParaFormatLayer);
}

void CAppuifwText::SizeChanged()
{
  CEikRichTextEditor::HandleSizeChangedL();
  TextLayout()->RestrictScrollToTopsOfLines(EFalse);
}

TBool CAppuifwText::CheckRange(TInt& aPos, TInt& aLen, TInt aDocLen)
{
  if (aPos > aDocLen)
    return EFalse;

  if ((aLen == (-1)) || ((aPos + aLen) > aDocLen))
    aLen = aDocLen - aPos;

  return ETrue;
}

void CAppuifwText::ReadL(PyObject*& aRval, TInt aPos, TInt aLen)
{
  if (CheckRange(aPos, aLen, RichText()->DocumentLength()) == EFalse)
    User::Leave(KErrArgument);

  if (aRval = PyUnicode_FromUnicode(NULL, aLen)) {
    TPtr buf(PyUnicode_AsUnicode(aRval), 0, aLen);
    RichText()->Extract(buf, aPos, aLen);
  }
}

void CAppuifwText::WriteL(const TDesC& aText)
{
  TInt pos = CursorPos();
  TInt len_written;

  RMemReadStream source(aText.Ptr(), aText.Size());

  RichText()->ImportTextL(pos, source,
              CPlainText::EOrganiseByParagraph,
              KMaxTInt, KMaxTInt, &len_written);
  RichText()->ApplyCharFormatL(iCharFormat, iCharFormatMask, pos, len_written);

  /*
  RichText()->GetParaFormatL(&paraFormat, paraFormatMask,
                 pos, len_written);
  paraFormatMask.ClearAll();
  RichText()->ApplyParaFormatL(&paraFormat, paraFormatMask,
                   pos, len_written);
  */

  // BENCHMARKS with N70:
  // without paraformat, 13s
  //HandleTextChangedL();
  //SetCursorPosL(pos+len_written, EFalse);

  // without paraformat, 22s
  //SetCursorPosL(pos+len_written, EFalse);
  //HandleTextChangedL();

  // without paraformat, 12s
  //SetCursorPosL(pos+len_written, EFalse);

  // without paraformat, 3.7s
  //iLayout->SetAmountToFormat(CTextLayout::EFFormatBand);
  //SetCursorPosL(pos+len_written, EFalse);

  // with paraformat, 13.8s
  //HandleTextChangedL();
  //SetCursorPosL(pos+len_written, EFalse);

  // with paraformat, 4.6s
  //iLayout->SetAmountToFormat(CTextLayout::EFFormatBand);
  //SetCursorPosL(pos+len_written, EFalse);

  // with paraformat, 3.9s
  iTextView->HandleInsertDeleteL(TCursorSelection(pos,pos+len_written),0,ETrue);
  SetCursorPosL(pos+len_written, EFalse);
}

void CAppuifwText::DelL(TInt aPos, TInt aLen)
{
  if (CheckRange(aPos, aLen, RichText()->DocumentLength()) == EFalse)
    User::Leave(KErrArgument);
  RichText()->DeleteL(aPos, aLen);
  iTextView->HandleInsertDeleteL(TCursorSelection(aPos,aPos),aLen,ETrue);
  SetCursorPosL(CursorPos()-aLen, EFalse);
  //HandleTextChangedL();
}

TInt CAppuifwText::SetPos(TInt aPos)
{
  if (aPos > RichText()->DocumentLength())
    aPos = RichText()->DocumentLength();
  TRAPD(error, SetCursorPosL(aPos, EFalse));
  return error;
}

void CAppuifwText::UpdateCharFormatLayerL(void)
{
    iMyCharFormatLayer->SetL(iCharFormat, iCharFormatMask);
}

void CAppuifwText::SetBold(TBool aOpt)
{
  iCharFormatMask.SetAttrib(EAttFontStrokeWeight);

  if (aOpt)
    iCharFormat.iFontSpec.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
  else
    iCharFormat.iFontSpec.iFontStyle.SetStrokeWeight(EStrokeWeightNormal);
  UpdateCharFormatLayerL();
}

void CAppuifwText::SetUnderline(TBool aOpt)
{
  iCharFormatMask.SetAttrib(EAttFontUnderline);
  if (aOpt)
    iCharFormat.iFontPresentation.iUnderline = EUnderlineOn;
  else
    iCharFormat.iFontPresentation.iUnderline = EUnderlineOff;
  UpdateCharFormatLayerL();
}

void CAppuifwText::SetPosture(TBool aOpt)
{
  iCharFormatMask.SetAttrib(EAttFontPosture);

  if (aOpt)
    iCharFormat.iFontSpec.iFontStyle.SetPosture(EPostureItalic);
  else
    iCharFormat.iFontSpec.iFontStyle.SetPosture(EPostureUpright);
  UpdateCharFormatLayerL();
}

void CAppuifwText::SetStrikethrough(TBool aOpt)
{
  iCharFormatMask.SetAttrib(EAttFontStrikethrough);

  if (aOpt)
    iCharFormat.iFontPresentation.iStrikethrough = EStrikethroughOn;
  else
    iCharFormat.iFontPresentation.iStrikethrough = EStrikethroughOff;
  UpdateCharFormatLayerL();
}

void CAppuifwText::SetHighlight(TInt mode)
{
  iCharFormatMask.SetAttrib(EAttFontHighlightStyle);

  switch (mode) {
  case KHIGHLIGHTNONE:
    iCharFormat.iFontPresentation.iHighlightStyle = TFontPresentation::EFontHighlightNone;
    break;
  case KHIGHLIGHTNORMAL:
    iCharFormat.iFontPresentation.iHighlightStyle = TFontPresentation::EFontHighlightNormal;
    break;
  case KHIGHLIGHTROUNDED:
    iCharFormat.iFontPresentation.iHighlightStyle = TFontPresentation::EFontHighlightRounded;
    break;
  case KHIGHLIGHTSHADOW:
    iCharFormat.iFontPresentation.iHighlightStyle = TFontPresentation::EFontHighlightShadow;
    break;
  }
  UpdateCharFormatLayerL();
}

void CAppuifwText::SetTextColor(TRgb color)
{
  iCharFormatMask.SetAttrib(EAttColor);
  iCharFormat.iFontPresentation.iTextColor = color;
  UpdateCharFormatLayerL();
}

void CAppuifwText::SetHighlightColor(TRgb color)
{
  iCharFormatMask.SetAttrib(EAttFontHighlightColor);
  iCharFormat.iFontPresentation.iHighlightColor = color;
  UpdateCharFormatLayerL();
}

void CAppuifwText::SetFont(TFontSpec& aFontSpec)
{
  /* XXX Necessary???
   *
   *   iCoeEnv = CEikRichTextEditor::ControlEnv();

  TRAPD(error,
    (aFont = iCoeEnv->CreateScreenFontL(*aFontSpec) ));

  if (error != KErrNone) {
    SPyErr_SetFromSymbianOSErr(error);
    return;
  }
*/
  iCharFormatMask.SetAttrib(EAttFontHeight);
  iCharFormatMask.SetAttrib(EAttFontPosture);
  iCharFormatMask.SetAttrib(EAttFontStrokeWeight);
  iCharFormatMask.SetAttrib(EAttFontPrintPos);
  iCharFormatMask.SetAttrib(EAttFontTypeface);
  //iCharFormatMask.SetAll();
  iCharFormat.iFontSpec = aFontSpec;

  UpdateCharFormatLayerL();
}

struct Text_object {
  PyObject_VAR_HEAD
  CAppuifwText* ob_control;
  CAppuifwEventBindingArray* ob_event_bindings;
  ControlName control_name;
};


extern "C" PyObject *
new_Text_object(PyObject* /*self*/, PyObject* args)
{
  int l;
  char* b = NULL;

  if (!PyArg_ParseTuple(args, "|u#", &b, &l))
    return NULL;

  Text_object *op = PyObject_New(Text_object, &Text_type);
  if (op == NULL)
    return PyErr_NoMemory();
  op->control_name = EText;
  op->ob_event_bindings = NULL;
  if (!(op->ob_control = new CAppuifwText())) {
    PyObject_Del(op);
    return PyErr_NoMemory();
  }
  CAmarettoAppUi* appui = MY_APPUI;
  TRAPD(error, {
    op->ob_control->ConstructL(appui->ClientRect(), appui->iContainer);
  });

  PyObject *default_font_object=Py_BuildValue("s", "dense");
  TFontSpec fontspec;
  TFontSpec_from_python_fontspec(default_font_object,
          fontspec,
          *CEikonEnv::Static()->ScreenDevice());
  op->ob_control->SetFont(fontspec);
  Py_DECREF(default_font_object);
  TRAP(error, {
    if (b) {
      op->ob_control->ClearL();
      op->ob_control->WriteL(TPtrC((TUint16 *)b, l));
    }
  });

  if (error != KErrNone) {
    delete op->ob_control;
    PyObject_Del(op);
    op = NULL;
    return SPyErr_SetFromSymbianOSErr(error);
  }
  else
    return (PyObject *) op;
}

extern "C" PyObject*
Text_clear(Text_object *self, PyObject* /*args*/)
{
  TRAPD(error, self->ob_control->ClearL());
  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject*
Text_get(Text_object *self, PyObject *args)
{
  TInt pos = 0;
  TInt len = (-1);

  if (!PyArg_ParseTuple(args, "|ii", &pos, &len))
    return NULL;

  PyObject* r = NULL;
  TRAPD(error, self->ob_control->ReadL(r, pos, len));

  return ((error != KErrNone) ? SPyErr_SetFromSymbianOSErr(error) : r);
}

extern "C" PyObject*
Text_set(Text_object *self, PyObject *args)
{
  int l;
  char* b = NULL;

  if (!PyArg_ParseTuple(args, "u#", &b, &l))
    return NULL;

  TRAPD(error, {
    self->ob_control->ClearL();
    self->ob_control->WriteL(TPtrC((TUint16 *)b, l));
  });

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject*
Text_add(Text_object *self, PyObject *args)
{
  int l;
  char* b;

  if (!PyArg_ParseTuple(args, "u#", &b, &l))
    return NULL;

  TRAPD(error, self->ob_control->WriteL(TPtrC((TUint16 *)b, l)));

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject*
Text_del(Text_object *self, PyObject *args)
{
  TInt pos = 0;
  TInt len = (-1);

  if (!PyArg_ParseTuple(args, "|ii", &pos, &len))
    return NULL;

  TRAPD(error, self->ob_control->DelL(pos, len));

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject*
Text_len(Text_object *self, PyObject* /*args*/)
{
  return Py_BuildValue("i", self->ob_control->Len());
}

extern "C" PyObject*
Text_set_pos(Text_object *self, PyObject *args)
{
  TInt pos;

  if (!PyArg_ParseTuple(args, "i", &pos))
    return NULL;

  TInt error = self->ob_control->SetPos(pos);

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject*
Text_get_pos(Text_object *self, PyObject* /*args*/)
{
  return Py_BuildValue("i", self->ob_control->GetPos());
}

extern "C" PyObject *
Text_bind(Text_object *self, PyObject* args)
{
  int key_code;
  PyObject* c;

  if (!PyArg_ParseTuple(args, "iO", &key_code, &c))
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
  bind_info.iType = SAmarettoEventInfo::EKey;
  bind_info.iKeyEvent.iCode = key_code;
  bind_info.iKeyEvent.iModifiers = 0;
  bind_info.iCb = c;
  Py_XINCREF(c);

  TRAPD(error, self->ob_event_bindings->InsertEventBindingL(bind_info));

  if (error != KErrNone)
    Py_XDECREF(c);

  RETURN_ERROR_OR_PYNONE(error);
}


extern "C" {
  static void
  Text_dealloc(Text_object *op)
  {
    delete op->ob_event_bindings;
    op->ob_event_bindings = NULL;
    delete op->ob_control;
    op->ob_control = NULL;

    PyObject_Del(op);
  }

  const static PyMethodDef Text_methods[] = {
    {"clear", (PyCFunction)Text_clear, METH_NOARGS},
    {"get", (PyCFunction)Text_get, METH_VARARGS},
    {"set", (PyCFunction)Text_set, METH_VARARGS},
    {"add", (PyCFunction)Text_add, METH_VARARGS},
    {"delete", (PyCFunction)Text_del, METH_VARARGS},
    {"len", (PyCFunction)Text_len, METH_NOARGS},
    {"set_pos", (PyCFunction)Text_set_pos, METH_VARARGS},
    {"get_pos", (PyCFunction)Text_get_pos, METH_NOARGS},
    {"bind", (PyCFunction)Text_bind, METH_VARARGS},
    {NULL,              NULL}           // sentinel
  };

  static PyObject *
  Text_getattr(Text_object *op, char *name)
  {
    if (!strcmp(name, "focus")) {
      if (op->ob_control->IsFocused()) {
        Py_INCREF(Py_True);
        return Py_True;
      }
      else {
    Py_INCREF(Py_False);
    return Py_False;
      }
    }

    else if (!strcmp(name, "style")) {
      return Py_BuildValue("i", op->ob_control->Style());
    }
    else if (!strcmp(name, "color")) {
      return ColorSpec_FromRgb(op->ob_control->TextColor());
    }
    else if (!strcmp(name, "highlight_color")) {
      return ColorSpec_FromRgb(op->ob_control->HighlightColor());
    }
    else if (!strcmp(name, "font")) {
        return python_fontspec_from_TFontSpec(op->ob_control->Font(), *CEikonEnv::Static()->ScreenDevice());
    }
    else if (!strcmp(name, UICONTROLAPI_NAME)) {
      Py_INCREF(op);
      return PyCObject_FromVoidPtr(op,_uicontrolapi_decref);
    }


    return Py_FindMethod((PyMethodDef*)Text_methods,
                         (PyObject *)op, name);
  }

  static int
  Text_setattr(Text_object *op, char *name, PyObject *v)
  {
    /* focus */
    if (!strcmp(name, "focus"))
      if (v == Py_True) {
        op->ob_control->SetFocus(ETrue);
        return 0;
      }
      else if (v == Py_False) {
        op->ob_control->SetFocus(EFalse);
        return 0;
      }
      else {
        PyErr_SetString(PyExc_TypeError, "True or False expected");
        return -1;
      }

    /* style */
    else if (!strcmp(name, "style")) {
      if ((v!= Py_None) && !PyInt_Check(v)) {
        PyErr_SetString(PyExc_TypeError, "style must be a valid flag or valid combination of them");
        return -1;
      }

      int flag = PyInt_AsLong(v);

      op->ob_control->SetBold(flag & KBOLD?ETrue:EFalse);
      op->ob_control->SetUnderline(flag & KUNDERLINE?ETrue:EFalse);
      op->ob_control->SetPosture(flag & KITALIC?ETrue:EFalse);
      op->ob_control->SetStrikethrough(flag & KSTRIKETHROUGH?ETrue:EFalse);

      if (flag & HL_MASK)
    switch (flag & HL_MASK) {
    case KHIGHLIGHTNORMAL:
      op->ob_control->SetHighlight(KHIGHLIGHTNORMAL);
      break;
    case KHIGHLIGHTROUNDED:
      op->ob_control->SetHighlight(KHIGHLIGHTROUNDED);
      break;
    case KHIGHLIGHTSHADOW:
      op->ob_control->SetHighlight(KHIGHLIGHTSHADOW);
      break;
    default:
      PyErr_SetString(PyExc_TypeError, "Valid combination of flags for highlight style is expected");
      return -1;
    }
      else
    op->ob_control->SetHighlight(KHIGHLIGHTNONE);


      if ( (flag >= KNORMAL) && (flag <= (KBOLD|KUNDERLINE|KITALIC|KSTRIKETHROUGH|HL_MASK)) )
    op->ob_control->SetStyle(flag);
      else {
      PyErr_SetString(PyExc_TypeError, "Valid combination of flags for text and highlight style is expected");
      return -1;
      }
      return 0;
    }

    /* color */
    else if (!strcmp(name, "color")) {
      TRgb color;
      if (!ColorSpec_AsRgb(v,&color))
        return -1;
      op->ob_control->SetTextColor(color);
      return 0;
    }

    /* highlight color */
    else if (!strcmp(name, "highlight_color")) {
      TRgb color;
      if (!ColorSpec_AsRgb(v,&color))
        return -1;
      op->ob_control->SetHighlightColor(color);
      return 0;
    }

    /* font */
    else if (!strcmp(name, "font")) {
      TFontSpec fontspec;
      if (-1 == TFontSpec_from_python_fontspec(v, fontspec, *CEikonEnv::Static()->ScreenDevice()))
          return -1;
      op->ob_control->SetFont(fontspec);
      return 0;
    }

    /* non existing attribute */
    PyErr_SetString(PyExc_AttributeError, "no such attribute");
    return -1;
  }

  static const PyTypeObject c_Text_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "appuifw.Text",                            /*tp_name*/
    sizeof(Text_object),                       /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)Text_dealloc,                  /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)Text_getattr,                 /*tp_getattr*/
    (setattrfunc)Text_setattr,                 /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };
}       // extern "C"

/*
 *
 * Implementation of appuifw.Canvas
 *
 */

CAppuifwCanvas::CAppuifwCanvas(PyObject *aDrawCallback,
         PyObject *aEventCallback, PyObject* aResizeCallback) {
  iDrawCallback = aDrawCallback;
  iEventCallback = aEventCallback;
  iResizeCallback = aResizeCallback;
}

TInt CAppuifwCanvas::BeginRedraw(TRect& rect) {
  TRAPD(error, Window().BeginRedraw(rect));
  return error;
}

TInt CAppuifwCanvas::EndRedraw() {
  TRAPD(error, Window().EndRedraw());
  return error;
}

TKeyResponse CAppuifwCanvas::OfferKeyEventL(const TKeyEvent& aKeyEvent,
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

void CAppuifwCanvas::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{
  PyObject *arg=NULL, *ret=NULL;

  if( !touch_enabled_flag )
    return;

  /* All basic touch events cause this callback function to be hit before the
   * container's HandlePointerEventL function. This is because the Canvas is
   * above the Container and Canvas is a window-owning control as it derives
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

void CAppuifwCanvas::ConstructL(const TRect& aRect,
                const CCoeControl* aParent, CAmarettoAppUi* aAppui)
{
  //SetContainerWindowL(*aParent);

  /*  if (aParent)
      SetContainerWindowL(*aParent);
      else */
  __ASSERT_DEBUG(aParent, User::Panic(_L("CAppuifwCanvas"), 2)); // This control must have a parent.
  CreateWindowL(aParent);

  if(touch_enabled_flag){
    EnableDragEvents();
  }

  myParent = aParent;
  myAppui = aAppui;
  SetRect(aRect);
  ActivateL();
}

/* FIXME This method is declared const so it should not alter the
   state of the object, but arbitrary code execution is allowed in the
   Python callback. This could lead to trouble if the Canvas is
   altered there. Maybe we should switch to double buffering completely or
   invoke the callback via a callgate? */
void
CAppuifwCanvas::Draw(const TRect& aRect) const
{
  PyGILState_STATE gstate;
  if (iDrawCallback) {
    gstate = PyGILState_Ensure();
    PyObject *pyrect=Py_BuildValue("((iiii))",
                   aRect.iTl.iX,
                   aRect.iTl.iY,
                   aRect.iBr.iX,
                   aRect.iBr.iY);
    app_callback_handler(iDrawCallback,pyrect);
    Py_DECREF(pyrect);
    PyGILState_Release(gstate);
  } else {
    CWindowGc& gc=SystemGc();
    gc.SetBrushColor(KRgbWhite);
    gc.Clear(Rect());
  }
  myAppui->RefreshDirectionalPad();
}

void
CAppuifwCanvas::SizeChanged()
{
  PyGILState_STATE gstate;
  if (iResizeCallback) {
    gstate = PyGILState_Ensure();
    PyObject *pyrectsize=Py_BuildValue("((ii))", Rect().Width(), Rect().Height());
    app_callback_handler( iResizeCallback, pyrectsize);
    PyGILState_Release(gstate);
  }
  myAppui->RefreshDirectionalPad();
}

void
pyprintf(const char *format, ...)
{
  va_list args;
  va_start(args,format);
  char buf[128];
  // Unfortunately Symbian doesn't seem to support the vsnprintf
  // function, which would allow us to specify the length of the
  // buffer we are writing to. As a result using this function is
  // unsafe and will lead to a buffer overflow if given an argument
  // that is too big. Do not use this function in production code.
  vsprintf(buf, format, args);
  char buf2[128];
  sprintf(buf2, "print '%s'", buf);
  PyRun_SimpleString(buf2);
}

struct Canvas_object {
  PyObject_VAR_HEAD
  CAppuifwCanvas* ob_control;
  CAppuifwEventBindingArray* ob_event_bindings;
  ControlName control_name;
  // NOTE: fields above this line must match struct _control_object!
  PyObject *ob_drawcallback;
  PyObject *ob_eventcallback;
  PyObject *ob_resizecallback;
  TBool begin_redraw_notified;
};

#define FAILMEM(phase) do { ret=PyErr_NoMemory(); goto fail##phase; } while(0)
#define FAILSYMBIAN(phase) do { ret=SPyErr_SetFromSymbianOSErr(error); goto fail##phase; } while(0)

extern "C" PyObject *
new_Canvas_object(PyObject* /*self*/, PyObject* args)
{
  PyObject *ret;
  PyObject *draw_cb=NULL, *event_cb=NULL, *resize_cb=NULL;

  if (!PyArg_ParseTuple(args, "OOO", &draw_cb, &event_cb, &resize_cb)) {
    return NULL;
  }
  if ((draw_cb && !PyCallable_Check(draw_cb) && draw_cb != Py_None) ||
      (event_cb && !PyCallable_Check(event_cb) && event_cb != Py_None) ||
      (resize_cb && !PyCallable_Check(resize_cb) && resize_cb != Py_None)) {
    PyErr_SetString(PyExc_TypeError, "callable or None expected");
    return NULL;
  }
  if (draw_cb == Py_None)
    draw_cb=NULL;
  if (event_cb == Py_None)
    event_cb=NULL;
  if (resize_cb == Py_None)
    resize_cb=NULL;

  CAmarettoAppUi* appui = MY_APPUI;

  Canvas_object *op = PyObject_New(Canvas_object, &Canvas_type);
  if (!op) FAILMEM(1);
  op->ob_drawcallback=draw_cb;
  op->ob_eventcallback=event_cb;
  op->ob_resizecallback=resize_cb;
  op->ob_event_bindings = NULL;
  op->begin_redraw_notified = EFalse;
  op->control_name = ECanvas;

  if (!(op->ob_control = new CAppuifwCanvas(op->ob_drawcallback,
                        op->ob_eventcallback, op->ob_resizecallback))) FAILMEM(3);

  TRAPD(error, op->ob_control->ConstructL(appui->iContainer->Rect(), appui->iContainer, appui));

  if (error != KErrNone)
    FAILSYMBIAN(3);
  Py_XINCREF(op->ob_drawcallback);
  Py_XINCREF(op->ob_eventcallback);
  Py_XINCREF(op->ob_resizecallback);
  return (PyObject *) op;

 fail3:
      delete op->ob_control;
      PyObject_Del(op);
 fail1:return ret;
}

static void _Canvas_drawapi_decref(void *gc_obj, void *canvas_obj)
{
  CWindowGc *gc=STATIC_CAST(CWindowGc *,gc_obj);
  gc->Deactivate();
  delete gc;
  Py_DECREF((PyObject *)canvas_obj);
}

extern "C" PyObject*
Canvas__drawapi(Canvas_object *self, PyObject* /*args*/)
{
  Py_INCREF(self);
  CWindowGc *gc=self->ob_control->ControlEnv()->CreateGcL();
  gc->Activate(*self->ob_control->DrawableWindow());
  return PyCObject_FromVoidPtrAndDesc(STATIC_CAST(CBitmapContext *,gc),self,_Canvas_drawapi_decref);
}

extern "C" PyObject *
Canvas_bind(Canvas_object *self, PyObject* args)
{
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

extern "C" PyObject *
Canvas_begin_redraw(Canvas_object *self, PyObject* args)
{
  if (self->begin_redraw_notified) {
    PyErr_SetString(PyExc_RuntimeError, "Redundant begin_redraw call");
    return NULL;
  }

  TRect rect = self->ob_control->Rect();
  if (!PyArg_ParseTuple(args, "|(iiii)",
    &rect.iTl.iX,
    &rect.iTl.iY,
    &rect.iBr.iX,
    &rect.iBr.iY))
    return NULL;
  TInt error = self->ob_control->BeginRedraw(rect);
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
  else
    self->begin_redraw_notified = ETrue;
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
Canvas_end_redraw(Canvas_object *self, PyObject* /*args*/)
{
  if (!self->begin_redraw_notified) {
    PyErr_SetString(PyExc_RuntimeError, "begin_redraw should be called before calling end_redraw");
    return NULL;
  }

  TInt error = self->ob_control->EndRedraw();
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
  else
    self->begin_redraw_notified = EFalse;
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" {
  static void
  Canvas_dealloc(Canvas_object *op)
  {
    Py_XDECREF(op->ob_resizecallback);
    op->ob_resizecallback=NULL;
    Py_XDECREF(op->ob_eventcallback);
    op->ob_eventcallback=NULL;
    Py_XDECREF(op->ob_drawcallback);
    op->ob_drawcallback=NULL;
    delete op->ob_event_bindings;
    op->ob_event_bindings = NULL;

    delete op->ob_control;
    op->ob_control = NULL;

    PyObject_Del(op);
  }

  const static PyMethodDef Canvas_methods[] = {
    {"bind", (PyCFunction)Canvas_bind, METH_VARARGS},
    {"_drawapi", (PyCFunction)Canvas__drawapi, METH_NOARGS},
    {"begin_redraw", (PyCFunction)Canvas_begin_redraw, METH_VARARGS},
    {"end_redraw", (PyCFunction)Canvas_end_redraw, METH_NOARGS},
    {NULL,              NULL}           // sentinel
  };

  static PyObject *
  Canvas_getattr(Canvas_object *op, char *name)
  {
    CAppuifwCanvas *canvas=op->ob_control;
    if (!strcmp(name,"size")) {
      TSize size=canvas->Size();
      return Py_BuildValue("(ii)",size.iWidth,size.iHeight);
    }
    if (!strcmp(name, UICONTROLAPI_NAME)) {
      Py_INCREF(op);
      return PyCObject_FromVoidPtr(op,_uicontrolapi_decref);
    }

    return Py_FindMethod((PyMethodDef*)Canvas_methods,
                         (PyObject *)op, name);
  }

  static int
  Canvas_setattr(Canvas_object */*op*/, char */*name*/, PyObject */*v*/)
  {
    PyErr_SetString(PyExc_AttributeError, "no such attribute");
    return -1;
  }

  static const PyTypeObject c_Canvas_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "appuifw.Canvas",                          /*tp_name*/
    sizeof(Canvas_object),                       /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)Canvas_dealloc,                  /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)Canvas_getattr,                 /*tp_getattr*/
    (setattrfunc)Canvas_setattr,                 /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };
}       // extern "C"



/*
 *
 * Implementation of appuifw.Form
 *
 * The design of Form is essentially about keeping synchronized
 * the GUI and the form data stored as a Python object. The
 * GUI part is implemented by class CAppuifwForm that derives
 * from CAknForm. class CPyFormFields provides an interface
 * for accessing and manipulating the Python object that contains
 * the form data.
 */

/*
 * Combo/Popup -field data content in Symbian OS data types
 */

class TComboFieldData {
public:
  TComboFieldData():iOptions(NULL),iCurrent(0),iArrayOwned(EFalse) {;}
  ~TComboFieldData() {
    if(iArrayOwned && iOptions) {
      delete iOptions; iOptions = NULL;
    }
  }
  void NewArrayL();
  void AppendL(const TDesC& aData) {iOptions->AppendL(aData);}

  CDesCArray* iOptions;
  TInt iCurrent;
  TBool iArrayOwned;
};

/*
 * Representation of a form field in Symbian OS data types
 */

class TFormField {
public:
  static TInt FieldType(const TDesC8& aName);

  TFormField():iNumber(0),iNumReal(0.0),iTime(0),iType(0),iValue(NULL) {;}
  const TDesC& Label() {return iLabel;}
  TInt Type() {return iType;}
  TAny* Value() {return iValue;}

  TComboFieldData iCombo;
  TBuf<KAppuifwMaxTextField> iLabel;
  TBuf<KAppuifwMaxTextField> iText;
  TInt iNumber;
  TReal iNumReal;
  TTime iTime;
  TInt iType;
  TAny* iValue;
};

TInt TFormField::FieldType(const TDesC8& aName)
{
  if (aName.Compare(KTextFieldType) == 0)
    return EEikCtEdwin;
  else if (aName.Compare(KNumberFieldType) == 0)
    return EEikCtNumberEditor;
  else if (aName.Compare(KFloatFieldType) == 0)
    return EEikCtFlPtEd;
  else if (aName.Compare(KDateFieldType) == 0)
    return EEikCtDateEditor;
  else if (aName.Compare(KTimeFieldType) == 0)
    return EEikCtTimeEditor;
  else if (aName.Compare(KComboFieldType) == 0)
    return EAknCtPopupFieldText;
  else                             // unknown
    return -1;
}

/*
 * Utility class that hides from UI code the Python Form data object
 */

class CPyFormFields : public CBase {
public:
  static PyObject* ItemValidate(PyObject*);
  CPyFormFields():iIsInitialized(0),iFieldList(NULL) {;}
  ~CPyFormFields() {
    Py_XDECREF(iFieldList);
  }
  TInt Init(PyObject* aList);
  TBool GetCurrentItemL(TFormField* aField);
  PyObject* GetFieldList() {
    return iFieldList;
  }
  void InsertL(TInt aInsertPos, const TFormField& aField);
  void Next();
  void Reset();

private:
  TInt iPos;
  TInt iSz;
  TInt iIsInitialized;
  PyObject* iCurrentField;
  PyObject* iFieldList;
};

class CAppuifwForm;

#define KFormEditModeOnly 0x0001
#define KFormViewModeOnly 0x0002
#define KFormAutoLabelEdit 0x0004
#define KFormAutoFormEdit 0x0008
#define KFormDoubleSpaced 0x0010

void TComboFieldData::NewArrayL()
{
  if (iOptions)
    iOptions->Delete(0, iOptions->Count());
  else {
    iOptions = new (ELeave) CDesCArrayFlat(5);
    iArrayOwned = ETrue;
  }
}

PyObject* CPyFormFields::ItemValidate(PyObject* t)
{
  if (!t || !PyTuple_Check(t)) {
    PyErr_SetString(PyExc_TypeError, "tuple expected");
    return NULL;
  }

  int c_sz = PyTuple_Size(t);
  if (!((c_sz == 2) || (c_sz == 3))) {
    PyErr_SetString(PyExc_ValueError,
                    "Form field, tuple of size 2 or 3 expected");
    return NULL;
  }

  if (!PyUnicode_Check(PyTuple_GetItem(t, 0))) {
    PyErr_SetString(PyExc_TypeError,
                    "Form field label, unicode expected");
    return NULL;
  }

  PyObject* type = PyTuple_GetItem(t, 1);

  if (!PyString_Check(type)) {
    PyErr_SetString(PyExc_TypeError,
                    "Form field type, string expected");
    return NULL;
  }

  TPtrC8 type_name((TUint8*)PyString_AsString(type));

  if (TFormField::FieldType(type_name) == -1) {
    PyErr_SetString(PyExc_ValueError, "Form field, unknown type");
    return NULL;
  }

  if (c_sz == 2) {
    if (type_name.Compare(KComboFieldType) == 0) {
      PyErr_SetString(PyExc_ValueError, "Form combo field, no value");
      return NULL;
    }
    else
      return t;
  }

  PyObject* val = PyTuple_GetItem(t, 2);

  switch(TFormField::FieldType(type_name)) {
  case EEikCtEdwin:
    if (!PyUnicode_Check(val)) {
      PyErr_SetString(PyExc_TypeError,
                      "Form text field, unicode value expected");
      return NULL;
    }
    break;
  case EEikCtNumberEditor:
    if (PyLong_Check(val)) {
      long int longval = PyLong_AsLong(val);
      if (longval <0 || longval > 0x7fffffff) {
        PyErr_SetString(PyExc_ValueError,
                      "Form number field, value must be positive and below 2147483648");
        return NULL;
      }
      else {
        int intval = (int)longval;
        val = Py_BuildValue("i", intval);
        PyTuple_SetItem(t, 2, val);
      }
    }

    if (PyFloat_Check(val)) {
      TReal floatval = PyFloat_AsDouble(val);
      TInt intval = (TInt)(TReal(floatval));
      val = Py_BuildValue("i", intval);
      PyTuple_SetItem(t, 2, val);
    }

    if (!PyInt_Check(val)) {
      PyErr_SetString(PyExc_TypeError,
                      "Form number field, number value expected");
      return NULL;
    }

    if (  PyInt_AsLong(val) <0 || PyInt_AsLong(val) > 0x7fffffff  ) {
      PyErr_SetString(PyExc_ValueError,
                      "Form number field, value must be positive and below 2147483648");
      return NULL;
    }
    break;
  case EEikCtFlPtEd:
    if (!PyFloat_Check(val)) {
      PyErr_SetString(PyExc_TypeError,
                      "Form float field, float value expected");
      return NULL;
    }
    break;
  case EEikCtDateEditor:
  case EEikCtTimeEditor:
    if (!PyFloat_Check(val)) {
      PyErr_SetString(PyExc_TypeError,
                      "Form datetime field, float value expected");
      return NULL;
    }
    break;
  case EAknCtPopupFieldText:
    {
      if (!PyTuple_Check(val) || (PyTuple_Size(val) != 2)) {
        PyErr_SetString(PyExc_TypeError,
                        "Form combo field, tuple of size 2 expected");
        return NULL;
      }

      PyObject* l = PyTuple_GetItem(val, 0);
      PyObject* c = PyTuple_GetItem(val, 1);

      if (!PyList_Check(l)) {
        PyErr_SetString(PyExc_TypeError,
                        "Form combo field, list expected");
        return NULL;
      }

      int sz = PyList_Size(l);

      if ((!PyInt_Check(c)) || (PyLong_AsLong(c) >= sz)) {
        PyErr_SetString(PyExc_TypeError,
                        "Form combo field, bad index");
        return NULL;
      }

      for (int i = 0; i < sz; i++)
        if (!PyUnicode_Check(PyList_GetItem(l, i))) {
          PyErr_SetString(PyExc_TypeError,
                          "Form combo field, unicode expected");
          return NULL;
        }
    }
    break;
  default:
    break;
  }
  return t;
}

TInt CPyFormFields::Init(PyObject* aList)
{
  Py_XDECREF(iFieldList);

  if (aList) {
    Py_INCREF(aList);
    iFieldList = aList;
  }
  else
    if (!(iFieldList = PyList_New(0)))
      return KErrNoMemory;

  iPos = 0;
  if ((iSz = PyList_Size(iFieldList)) > 0) {
    if (!(iCurrentField = PyList_GetItem(iFieldList, 0))) {
      return KErrPython;
    }
  }
  else {
    iCurrentField = NULL;
  }
  iIsInitialized = 1;

  return KErrNone;
}

TBool CPyFormFields::GetCurrentItemL(TFormField* aField)
{
  if (!iIsInitialized || !iCurrentField)
    User::Leave(KErrUnknown);

  if (iPos > (iSz-1))
    return EFalse;

  // Label
  PyObject* us = PyTuple_GetItem(iCurrentField, 0);
  if (us)
    aField->iLabel.Copy(PyUnicode_AsUnicode(us), PyUnicode_GetSize(us));
  else
    User::Leave(KErrPython);

  // Type
  PyObject* tmp = PyTuple_GetItem(iCurrentField, 1);
  if (!tmp)
    User::Leave(KErrPython);

  aField->iType =
    TFormField::FieldType(TPtrC8((TUint8*)PyString_AsString(tmp)));

  // Value
  int tup_sz = PyTuple_Size(iCurrentField);
  if (tup_sz == 2) {
    aField->iValue = NULL;
    return ETrue;
  }

  PyObject* py_val = PyTuple_GetItem(iCurrentField, 2);
  if (!py_val) {
    User::Leave(KErrPython);
  }
  switch(aField->iType) {
  case EEikCtEdwin:
    aField->iText.Copy(PyUnicode_AsUnicode(py_val),
                       Min(PyUnicode_GetSize(py_val),
                           KAppuifwMaxTextField));
    aField->iValue = &(aField->iText);
    break;
  case EEikCtNumberEditor:
    aField->iNumber = PyLong_AsLong(py_val);
    aField->iValue = &(aField->iNumber);
    break;
  case EEikCtFlPtEd:
    aField->iNumReal = PyFloat_AsDouble(py_val);
    aField->iValue = &(aField->iNumReal);
    break;
  case EEikCtDateEditor:
    aField->iTime =
      epoch_local_as_TReal()+(PyFloat_AsDouble(py_val)*(1000.0*1000.0));
    aField->iValue = &(aField->iTime);
    break;
  case EEikCtTimeEditor:
    aField->iTime = (PyFloat_AsDouble(py_val)*(1000.0*1000.0));
    aField->iValue = &(aField->iTime);
    break;
  case EAknCtPopupFieldText:
    {
      if ((aField->iCombo.iCurrent =
           PyLong_AsLong(PyTuple_GetItem(py_val, 1))) == (-1))
        User::Leave(KErrPython);

      aField->iCombo.NewArrayL();
      PyObject* py_field_list = PyTuple_GetItem(py_val, 0);
      int l_sz = PyList_Size(py_field_list);
      for (int i = 0; i < l_sz; i++) {
        PyObject* s = PyList_GetItem(py_field_list, i);
        if (!PyUnicode_Check(s))
          User::Leave(KErrArgument);
        else
          aField->iCombo.AppendL(TPtrC(PyUnicode_AsUnicode(s),
                                       Min(PyUnicode_GetSize(s),
                                           KAppuifwMaxTextField)));
      }
      aField->iValue = &(aField->iCombo);
    }
    break;
  }

  return ETrue;
}

void CPyFormFields::Next()
{
  if (iIsInitialized) {
    iPos++;
    if (iPos <= (iSz - 1))
      iCurrentField = PyList_GetItem(iFieldList, iPos);
    // errors handled in GetCurrentItemL()
  }
}

void CPyFormFields::Reset()
{
  if (iIsInitialized) {
    iSz = PyList_Size(iFieldList);
    iPos = 0;
    iCurrentField = PyList_GetItem(iFieldList, iPos);
  }
}

void CPyFormFields::InsertL(TInt aInsertPos, const TFormField& aFld)
{
  if (!iIsInitialized)
    User::Leave(KErrUnknown);

  PyObject* tn = PyTuple_New(3);
  if (!tn)
    User::Leave(KErrPython);

  PyObject* label = PyUnicode_FromUnicode(aFld.iLabel.Ptr(),
                                          aFld.iLabel.Length());
  if (PyTuple_SetItem(tn, 0, label)) {
    Py_DECREF(tn);
    User::Leave(KErrPython);
  }

  TInt error = KErrNone;
  PyObject* type = NULL;
  PyObject* val = NULL;

  switch(aFld.iType) {
  case EEikCtEdwin:
    type = PyString_FromString((const char*)(&KTextFieldType)->Ptr());
    val = PyUnicode_FromUnicode(aFld.iText.Ptr(),aFld.iText.Length());
    break;
  case EEikCtNumberEditor:
    type = PyString_FromString((const char*)(&KNumberFieldType)->Ptr());
    val = PyLong_FromLong(aFld.iNumber);
    break;
  case EEikCtFlPtEd:
    type = PyString_FromString((const char*)(&KFloatFieldType)->Ptr());
    val = PyFloat_FromDouble(aFld.iNumReal);
    break;
  case EEikCtDateEditor:
    type = PyString_FromString((const char*)(&KDateFieldType)->Ptr());
    val =  PyFloat_FromDouble(time_as_UTC_TReal(aFld.iTime));
    break;
  case EEikCtTimeEditor:
    type = PyString_FromString((const char*)(&KTimeFieldType)->Ptr());
    val = PyFloat_FromDouble(datetime_with_secs_as_TReal(aFld.iTime));
    break;
  case EAknCtPopupFieldText:
    {
      PyObject* val_array = NULL;
      PyObject* val_index = NULL;

      type = PyString_FromString((const char*)(&KComboFieldType)->Ptr());
      if (!(val = PyTuple_New(2)) ||
          !(val_index = PyLong_FromLong(aFld.iCombo.iCurrent)))
        error = KErrPython;

      TInt array_size = 0;

      if (error == KErrNone) {
        array_size = aFld.iCombo.iOptions->Count();
        if (!(val_array = PyList_New(array_size)))
          error = KErrPython;
      }

      if (error == KErrNone) {
        for (int i = 0; i < array_size; i++) {
          PyObject* n =
            Py_BuildValue("u#",((*(aFld.iCombo.iOptions))[i]).Ptr(),
                          ((*(aFld.iCombo.iOptions))[i]).Length());
          if (!n || PyList_SetItem(val_array, i, n)) {
            error = KErrPython;
            break;
          }
        }
      }

      if (error == KErrNone) {
        if (PyTuple_SetItem(val, 0, val_array) ||
            PyTuple_SetItem(val, 1, val_index))
          error = KErrPython;
      }
      else {
        Py_XDECREF(val_array);
        val_array = NULL;
        Py_XDECREF(val_index);
        val_index = NULL;
      }
    }
    break;
  default:
    error = KErrUnknown;
    val = NULL;
    type = NULL;
  }

  if ((error == KErrNone) && (type && val)) {
    if (PyTuple_SetItem(tn, 1, type))
      error = KErrPython;
    if (PyTuple_SetItem(tn, 2, val))
      error = KErrPython;
  }
  else {
    Py_XDECREF(val);
    Py_XDECREF(type);
    error = KErrPython;
  }

  if (error == KErrNone) {
    if (PyList_Insert(iFieldList, aInsertPos, tn))
      error = KErrPython;
    else {
      iSz++;
      iCurrentField = PyList_GetItem(iFieldList, aInsertPos);
    }
  }

  Py_XDECREF(tn);

  User::LeaveIfError(error);
}

struct Form_object {
  PyObject_VAR_HEAD
  CAppuifwForm *ob_form; // right now this would not be needed...
  PyObject *ob_fields;
  PyObject *ob_save_hook;
  int ob_flags;
  PyObject* ob_menu;
};

#ifndef EKA2
class CAppuifwForm: public CAknForm
#else
NONSHARABLE_CLASS(CAppuifwForm): public CAknForm
#endif
{
public:
  static CAppuifwForm* New(Form_object*);
  ~CAppuifwForm() {
    RestoreThread();
  }
  TInt ExecuteLD(TInt aResourceId ) {
    SaveThread();
    CAknForm::ExecuteLD(aResourceId);
    return 0;
  }
  void SyncL(TInt aFlags=0);
  void RestoreThread() {
    if (iThreadState)
      PyEval_RestoreThread(iThreadState);
  }
  void SaveThread() {
    iThreadState = PyEval_SaveThread();
  }
  void ClearSyncUi() {
    iSyncUi = EFalse;
  }
  void SetInitialCurrentLine()
  {
    ActivateFirstPageL();
    TryChangeFocusToL( 1 );
  }

protected:
  TBool SaveFormDataL();
  void DoNotSaveFormDataL();

private:
  void PreLayoutDynInitL();
  void PostLayoutDynInitL();
  //  void SetInitialCurrentLine();
  void ProcessCommandL(TInt aCommandId);
  void DynInitMenuPaneL(TInt aResourceId, CEikMenuPane* aMenuPane);
  //  TBool OkToExitL(TInt aKeycode);
  //  void HandleControlStateChangeL(TInt aControlId);

private:
  CAppuifwForm(Form_object* op):iFop(op), iNextControlId(1),
                                iNumControls(0), iSyncUi(EFalse) {;}
  void ConstructL() {
    User::LeaveIfError(iPyFields.Init(iFop->ob_fields));
    CAknForm::ConstructL();
  }
  void AddItemL();
  void AddControlL(const TDesC& aLabel, TInt aControlType, TAny* aVal);
  void SetControlL(const TDesC* aLabel, TInt aIndex, TAny* aVal);
  void DoPreLayoutDynInitL();
  void InitPopupFieldL(CAknPopupFieldText* aField, TAny* aValue);
  CPyFormFields iPyFields;
  Form_object* iFop;

  TInt iControlIds[KFormMaxFields];
  TInt iNextControlId;
  TInt iNumControls;

  TBool iSyncUi;

  PyThreadState* iThreadState;
};

CAppuifwForm* CAppuifwForm::New(Form_object* op)
{
  CAppuifwForm* dlg = new CAppuifwForm(op);
  if (dlg != NULL) {
    TRAPD(error, dlg->ConstructL());
    if (error != KErrNone) {
      delete dlg;
      dlg = NULL;
    }
  }
  return dlg;
}

void CAppuifwForm::InitPopupFieldL(CAknPopupFieldText* aField,
                                   TAny* aValue)
{
  aField->iFlags = 0;
  aField->iWidth = 20;
  aField->iEmptyText = (_L("")).AllocL();
  aField->iInvalidText = (_L("")).AllocL();

  void (CAknPopupField::* pConstructL)() = &CAknPopupField::ConstructL;
  (aField->*pConstructL)();

  CDesCArray* aArray = ((TComboFieldData*)aValue)->iOptions;
  aField->iArray = new (ELeave) CDesCArrayFlat(5);
  TInt c = aArray->Count();
  for (TInt i = 0; i < c; i++)
    aField->iArray->InsertL(i, (*aArray)[i]);

  aField->iTextArray = CAknQueryValueTextArray::NewL();
  aField->iTextArray->SetArray(*(aField->iArray));
  aField->iTextValue = CAknQueryValueText::NewL();
  aField->iTextValue->SetArrayL(aField->iTextArray);
  aField->SetCurrentValueIndex(((TComboFieldData*)aValue)->iCurrent);
  void (CAknPopupField::* pSetQueryValueL)(MAknQueryValue*) =
    &CAknPopupField::SetQueryValueL;
  (aField->*pSetQueryValueL)(aField->iTextValue);
}

void CAppuifwForm::AddItemL()
{
  // a bit heavy but easy...

  TPtrC txt(KUcTextFieldType);
  TPtrC num(KUcNumberFieldType);
  TPtrC flt(KUcFloatFieldType);
  TPtrC dat(KUcDateFieldType);
  TPtrC tim(KUcTimeFieldType);

  TInt index = KErrGeneral;

  RestoreThread();
  PyObject* arg = Py_BuildValue("([u#u#u#u#u#])", txt.Ptr(), txt.Length(),
                                num.Ptr(), num.Length(), flt.Ptr(), flt.Length(),
                                dat.Ptr(), dat.Length(), tim.Ptr(), tim.Length());
  if (arg) {
    PyObject* pyindex = popup_menu(NULL, arg);
    if (pyindex) {
      if (pyindex == Py_None)
        index = KErrCancel;
      else
        index = PyInt_AsLong(pyindex);
      Py_DECREF(pyindex);
    }
    Py_DECREF(arg);
  }
  SaveThread();

  TInt type_id;

  switch(index) {
  case KErrGeneral:
    User::Leave(KErrPython);
  case KErrCancel:
    return;
  case 0:
    type_id = EEikCtEdwin;
    break;
  case 1:
    type_id = EEikCtNumberEditor;
    break;
  case 2:
    type_id = EEikCtFlPtEd;
    break;
  case 3:
    type_id = EEikCtDateEditor;
    break;
  case 4:
    type_id = EEikCtTimeEditor;
    break;
  default:
    type_id = EEikCtEdwin;
    break;
  }

  AddControlL(_L("<label>"), type_id, NULL);
  AddControlL(_L("<label>"), EEikCtNumberEditor, NULL);  // a dirty hack
  DeleteLine(iNextControlId-1);
}

void CAppuifwForm::AddControlL(const TDesC& aLb, TInt aType, TAny* aVal)
{
  if (iNumControls > KFormMaxFields)
    User::Leave(KErrNoMemory);

  if (!((aType == EEikCtEdwin) || (aType == EEikCtNumberEditor) ||
        (aType == EEikCtFlPtEd) || (aType == EEikCtDateEditor) ||
        (aType == EEikCtTimeEditor) || (aType == EAknCtPopupFieldText)))
    return;

  TInt ctrl_id = iNextControlId;
  TInt focus_id = IdOfFocusControl();
  CCoeControl* ctrl =
    CreateLineByTypeL(aLb, ActivePageId(), ctrl_id, aType, NULL);

  switch(aType) {
  case EEikCtEdwin:
    {
      CEikEdwin* edwin = STATIC_CAST(CEikEdwin*, ctrl);
      edwin->ConstructL(EEikEdwinNoHorizScrolling | EEikEdwinResizable,
                        10, KAppuifwMaxTextField, 1);

      // set input capabilities and character modes here...
      if (aVal)
        edwin->SetTextL((TDes*)aVal);
      edwin->CreateTextViewL();
    }
    break;
  case EEikCtNumberEditor:
    (STATIC_CAST(CEikNumberEditor*, ctrl))->
      ConstructL(0, 0x7fffffff, (aVal ? *((TInt*)aVal) : 0));
    break;
  case EEikCtFlPtEd:
    (STATIC_CAST(CEikFloatingPointEditor*, ctrl))->
      ConstructL(-9.9e99, 9.9e99, 15); // overflow after 15
    (STATIC_CAST(CEikFloatingPointEditor*, ctrl))->
      SetValueL((aVal ? ((TReal*)aVal) : 0));
    break;
  case EEikCtDateEditor:
    (STATIC_CAST(CEikDateEditor*, ctrl))->
      ConstructL(KAknMinimumDate, KAknMaximumDate,
                 (aVal ? *((TTime*)aVal) : KAknMinimumDate), ETrue);
    break;
  case EEikCtTimeEditor:
    (STATIC_CAST(CEikTimeEditor*, ctrl))->
      ConstructL(TTime(0), TTime(KAknMaximumDate),
                 (aVal ? *((TTime*)aVal) : TTime(0)),
                 EEikTimeForce24HourFormat);
    break;
  case EAknCtPopupFieldText:
    InitPopupFieldL(STATIC_CAST(CAknPopupFieldText*, ctrl), aVal);
    break;
  }

  if (iNumControls == 0)
    iControlIds[0] = ctrl_id;
  else
    for (int i = 0; i < KFormMaxFields; i++)
      if (iControlIds[i] == focus_id) {
        if (i != (iNumControls - 1))
          Mem::Copy(&(iControlIds[i+2]), &(iControlIds[i+1]),
                    ((iNumControls - 1) - i) * sizeof(TInt));
        iControlIds[i+1] = ctrl_id;
        break;
      }

  iNextControlId++;
  iNumControls++;

  CEikCaptionedControl* capctrl = Line(ctrl_id);
  capctrl->SetTakesEnterKey(ETrue);
  capctrl->SetOfferHotKeys(ETrue);
  capctrl->ActivateL();
}

void CAppuifwForm::SetControlL(const TDesC* aLb, TInt aIx, TAny* aVal)
{
  TInt ctrl_id = iControlIds[aIx];
  CEikCaptionedControl* ctrl = Line(ctrl_id);

  if (aLb)
    ctrl->SetCaptionL(*aLb);

  switch(ctrl->iControlType) {
  case EEikCtEdwin:
    SetEdwinTextL(ctrl_id, (TDes*)aVal);
    break;
  case EEikCtNumberEditor:
    SetNumberEditorValue(ctrl_id, (aVal ? *((TInt*)aVal) : 0));
    break;
  case EEikCtFlPtEd:
    SetFloatingPointEditorValueL(ctrl_id, (aVal ? ((TReal*)aVal) : 0));
    break;
  case EEikCtDateEditor:
  case EEikCtTimeEditor:
    SetTTimeEditorValue(ctrl_id, (aVal ? *((TTime*)aVal):
                                  TTime(KAknMinimumDate)));
    break;
  case EAknCtPopupFieldText:
    (STATIC_CAST(CAknPopupFieldText*, Control(ctrl_id)))->
      SetCurrentValueIndex(((TComboFieldData*)aVal)->iCurrent);
    (STATIC_CAST(CAknPopupFieldText*, Control(ctrl_id)))->SizeChanged();
    break;
  default:
    User::Leave(KErrUnknown);
  }
}

/*
 * Synchronize UI with Form_object->ob_fields
 */

static void ed_cleanup_helper(TAny* arg)
{
  ((CAppuifwForm*)arg)->SetEditableL(EFalse);
}

void CAppuifwForm::SyncL(TInt aFlags)
{
  User::LeaveIfError(iPyFields.Init(iFop->ob_fields));

  if (iSyncUi) {
    TBool editable = IsEditable();
    if (!editable) {
      CleanupStack::PushL(TCleanupItem(ed_cleanup_helper, (TAny*)this));
      SetEditableL(ETrue);
    }

    if ((aFlags & KFormAutoFormEdit) ||
        (iFop->ob_flags & KFormAutoFormEdit)) {
      TInt old_ncs = iNumControls;
      TInt old_cids[KFormMaxFields];
      Mem::Copy(old_cids, iControlIds, sizeof(iControlIds));

      iNumControls = 0;
      DoPreLayoutDynInitL();

      for (TInt i = old_ncs; i > 0; i--) {
        if (ControlOrNull(old_cids[i-1]))
          DeleteLine(old_cids[i-1]);
      }
    }
    else {
      TFormField tmp;
      TInt i = 0;
      iPyFields.Reset();

      while (iPyFields.GetCurrentItemL(&tmp)) {
        if ((aFlags & KFormAutoLabelEdit) ||
            (iFop->ob_flags & KFormAutoLabelEdit))
          SetControlL(&(tmp.Label()), i, tmp.Value());
        else
          SetControlL(NULL, i, tmp.Value());
        i++;
        iPyFields.Next();
      }

      if (i != iNumControls)
        User::Leave(KErrUnknown);
    }
    if (!editable)
      CleanupStack::PopAndDestroy();   // SetEditable(EFalse)
  }
}

static void tsave_cleanup_helper(TAny* arg)
{
  ((CAppuifwForm*)arg)->SaveThread();
}

void CAppuifwForm::PreLayoutDynInitL()
{
  RestoreThread();
  CleanupStack::PushL(TCleanupItem(tsave_cleanup_helper, (TAny*)this));

  DoPreLayoutDynInitL();

  CleanupStack::PopAndDestroy();   // SaveThread()
  return;
}

void CAppuifwForm::DoPreLayoutDynInitL()
{
  TFormField tmp;

  iPyFields.Reset();

  while (iPyFields.GetCurrentItemL(&tmp)) {
    AddControlL(tmp.Label(), tmp.Type(), tmp.Value());
    iPyFields.Next();
  }

  return;
}

void CAppuifwForm::PostLayoutDynInitL()
{
  CAknForm::PostLayoutDynInitL();
  // set the context pane thumbnail here
}

void CAppuifwForm::DynInitMenuPaneL(TInt aResourceId,
                                    CEikMenuPane* aMenuPane)
{
  CAknForm::DynInitMenuPaneL(aResourceId, aMenuPane);

  if (aResourceId == R_AVKON_FORM_MENUPANE) {
    if (iFop->ob_flags & KFormViewModeOnly)
      aMenuPane->SetItemDimmed(EAknFormCmdEdit, ETrue);
    if (!(iFop->ob_flags & KFormAutoLabelEdit))
      aMenuPane->SetItemDimmed(EAknFormCmdLabel, ETrue);
    if (!(iFop->ob_flags & KFormAutoFormEdit)) {
      aMenuPane->SetItemDimmed(EAknFormCmdAdd, ETrue);
      aMenuPane->SetItemDimmed(EAknFormCmdDelete, ETrue);
    }

    RestoreThread();

    TInt error = KErrNone;
    int sz = PyList_Size(iFop->ob_menu);

    for (int i = 0; i < sz; i++) {
      PyObject* text =
        PyTuple_GetItem(PyList_GetItem(iFop->ob_menu, i), 0);
      if (!text)
        break;
      CEikMenuPaneItem::SData item;
      item.iCommandId = (EAknFormMaxDefault+1)+i;
      item.iCascadeId = 0;
      item.iFlags = 0;
      item.iText.Copy(PyUnicode_AsUnicode(text),
                      Min(PyUnicode_GetSize(text),
                          CEikMenuPaneItem::SData::ENominalTextLength));
      item.iExtraText = _L("");

      TRAP(error, aMenuPane->AddMenuItemL(item));
      if (error != KErrNone)
        break;
    }

    SaveThread();
    User::LeaveIfError(error);
  }
}

void CAppuifwForm::ProcessCommandL(TInt aCId)
{
  iSyncUi = ETrue;
  CAknForm::ProcessCommandL(aCId);

  if (aCId > EAknFormMaxDefault) {
    RestoreThread();
    if (aCId > (EAknFormMaxDefault+PyList_Size(iFop->ob_menu))) {
      SaveThread();
      iSyncUi = EFalse;
      return;
    }
    PyObject* callback =
      PyTuple_GetItem(PyList_GetItem(iFop->ob_menu,
                                     aCId-(EAknFormMaxDefault+1)), 1);
    TInt error =  app_callback_handler(callback);
    SaveThread();
    iSyncUi = EFalse;
    User::LeaveIfError(error);
  }

  iSyncUi = EFalse;
  SetEditableL(IsEditable());
}

TBool CAppuifwForm::SaveFormDataL()
{
  RestoreThread();
  CleanupStack::PushL(TCleanupItem(tsave_cleanup_helper, (TAny*)this));

  CPyFormFields* py_form = new (ELeave) CPyFormFields();
  CleanupStack::PushL(py_form);
  User::LeaveIfError(py_form->Init(NULL));

  TFormField tmp;
  TInt insert_pos = 0;

  for (TInt i = 0; i < iNumControls; i++) {
    if (ControlOrNull(iControlIds[i])) {
      TInt control_id = iControlIds[i];
      tmp.iLabel.Copy(Line(control_id)->GetFullCaptionText());
      tmp.iType = Line(control_id)->iControlType;

      switch(tmp.iType) {
      case EEikCtEdwin:
        GetEdwinText(tmp.iText, control_id);
        break;
      case EEikCtNumberEditor:
        tmp.iNumber = NumberEditorValue(control_id);
        break;
      case EEikCtFlPtEd:
        tmp.iNumReal = FloatEditorValue(control_id);
        break;
      case EEikCtDateEditor:
      case EEikCtTimeEditor:
        tmp.iTime = TTimeEditorValue(control_id);
        break;
      case EAknCtPopupFieldText:
        {
          CAknPopupFieldText* ctrl =
            (CAknPopupFieldText*)Control(control_id);
          tmp.iCombo.iOptions = ctrl->iArray;
          tmp.iCombo.iCurrent = ctrl->CurrentValueIndex();
        }
        break;
      default:
        User::Leave(KErrUnknown);
      }

      py_form->InsertL(insert_pos++, tmp);
    }
  }

  if (iFop->ob_save_hook) {
    PyObject *arg = Py_BuildValue("(O)", py_form->GetFieldList());
    if (arg == NULL)
      User::Leave(KErrPython);
    PyObject* ret = PyEval_CallObject(iFop->ob_save_hook, arg);
    Py_DECREF(arg);
    if (ret == NULL)
      User::Leave(KErrPython);
    else {
      TBool is_ret_false = (ret == Py_False);
      Py_DECREF(ret);
      if (is_ret_false) {
        CleanupStack::PopAndDestroy(/*py_form*/);
        SyncL();
        CleanupStack::PopAndDestroy();   // SaveThread()
        return EFalse;
      }
    }
  }

  Py_XDECREF(iFop->ob_fields);
  Py_INCREF(py_form->GetFieldList());
  iFop->ob_fields = py_form->GetFieldList();
  CleanupStack::PopAndDestroy(/*py_form*/);
  if (iFop->ob_save_hook)
    SyncL();
  else
    User::LeaveIfError(iPyFields.Init(iFop->ob_fields));

  CleanupStack::PopAndDestroy();   // SaveThread()
  return ETrue;
}

static void DoNotSave_cleanup_helper(TAny* arg)
{
  ((CAppuifwForm*)arg)->SaveThread();
  ((CAppuifwForm*)arg)->ClearSyncUi();
}

void CAppuifwForm::DoNotSaveFormDataL()
{
  RestoreThread();
  CleanupStack::PushL(TCleanupItem(DoNotSave_cleanup_helper, (TAny*)this));
  iSyncUi = ETrue;
  SyncL();
  CleanupStack::PopAndDestroy();   // SaveThread()
}

extern "C" PyObject *
new_Form_object(PyObject* /*self*/, PyObject *args, PyObject *kwds)
{
  PyObject* list_fields;
  int flags = 0;
  static const char *const kwlist[] = {"fields", "flags", NULL};

  if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!|i",
                                   (char**)kwlist, &PyList_Type,
                                   &list_fields, &flags)) {
    return NULL;
  }
  // validate contents of field list argument
  int sz = PyList_Size(list_fields);
  for (int i = 0; i < sz; i++)
    if (!CPyFormFields::ItemValidate(PyList_GetItem(list_fields, i))) {
      return NULL;
    }
  Form_object *op = PyObject_New(Form_object, &Form_type);
  if (op == NULL)
    return PyErr_NoMemory();

  if (!(op->ob_menu = PyList_New(0)))
    return PyErr_NoMemory();
  op->ob_flags = flags;
  op->ob_form = NULL;
  op->ob_save_hook = NULL;
  op->ob_fields = list_fields;
  Py_INCREF(list_fields);

  return (PyObject *) op;
}

extern "C" PyObject*
Form_execute(Form_object *self, PyObject* /*args*/)
{
  if (PyList_Size(self->ob_fields) == 0) {
    PyErr_SetString(PyExc_ValueError, "cannot execute empty form");
    return NULL;
  }
  if ((self->ob_form = CAppuifwForm::New(self)) == NULL)
    return PyErr_NoMemory();

  TInt rid;

  if (self->ob_flags & KFormDoubleSpaced)
    rid = ((self->ob_flags & KFormEditModeOnly) ?
           R_APPUIFW_FORM_DOUBLE_EDIT_DIALOG :
           R_APPUIFW_FORM_DOUBLE_DIALOG);
  else
    rid = ((self->ob_flags & KFormEditModeOnly) ?
           R_APPUIFW_FORM_SINGLE_EDIT_DIALOG :
           R_APPUIFW_FORM_SINGLE_DIALOG);

  TRAPD(error, self->ob_form->ExecuteLD(rid));

  self->ob_form = NULL;

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject*
Form_insert(Form_object *self, PyObject *args)
{
  int index;
  PyObject* new_item;

  if (!PyArg_ParseTuple(args, "iO", &index, &new_item) ||
      !CPyFormFields::ItemValidate(new_item))
    return NULL;

  int ret = PyList_Insert(self->ob_fields, index, new_item);

  if (self->ob_form && (ret == 0)) {
    // this is safe, as this can not be called from within
    // PreLayoutDynInitL()
    TRAPD(error, self->ob_form->SyncL(KFormAutoFormEdit));
    if (error != KErrNone) {
      ret = (-1);
      SPyErr_SetFromSymbianOSErr(error);
    }
  }

  if (ret == 0) {
    Py_INCREF(Py_None);
    return Py_None;
  }
  else
    return NULL;
}

extern "C" PyObject*
Form_pop(Form_object *self, PyObject *args)
{
  int ix = -1;
  PyObject *v;

  if (!PyArg_ParseTuple(args, "|i", &ix))
    return NULL;

  int sz = PyList_Size(self->ob_fields);

  // compare to listpop() in listobject.c
  if (sz == 0) {
    PyErr_SetString(PyExc_IndexError, "pop from empty list");
    return NULL;
  }
  if (ix < 0)
    ix += sz;
  if (ix < 0 || ix >= sz) {
    PyErr_SetString(PyExc_IndexError, "pop index out of range");
    return NULL;
  }
  v = PyList_GetItem(self->ob_fields, ix);
  Py_INCREF(v);

  if (PyList_SetSlice(self->ob_fields, ix, ix+1, (PyObject *)NULL)) {
    Py_DECREF(v);
    return NULL;
  }
  else
    if (self->ob_form) {
      // this is safe, as this can not be called from within
      // PreLayoutDynInitL()
      TRAPD(error, self->ob_form->SyncL(KFormAutoFormEdit));
      if (error != KErrNone) {
        Py_DECREF(v);
        return SPyErr_SetFromSymbianOSErr(error);
      }
    }

  return v;
}

/*
 * Form as sequence -- methods (notice also insert() and pop() above)
 */

extern "C" {

  static int
  Form_length(Form_object *f)
  {
    return PyList_Size(f->ob_fields);
  }

  static PyObject *
  Form_item(Form_object *f, int i)
  {
    PyObject* item = PyList_GetItem(f->ob_fields, i);
    Py_XINCREF(item);
    return item;
  }

  static int
  Form_ass_item(Form_object *f, int i, PyObject *v)
  {
    if (!CPyFormFields::ItemValidate(v))
      return NULL;

    Py_INCREF(v);

    TInt edit_mode = KFormAutoFormEdit;
    PyObject* new_type = PyTuple_GetItem(v, 1);
    PyObject* cur_type = PyTuple_GetItem(PyList_GetItem(f->ob_fields, i), 1);
    if (PyObject_RichCompareBool(new_type, cur_type, Py_EQ) == 1)
      edit_mode = KFormAutoLabelEdit;

    int ret = PyList_SetItem(f->ob_fields, i, v);

    if (f->ob_form && (ret == 0)) {
      // this is safe, as this can not be called from within
      // PreLayoutDynInitL()
      TRAPD(error, f->ob_form->SyncL(edit_mode));
      if (error != KErrNone) {
        ret = (-1);
        SPyErr_SetFromSymbianOSErr(error);
      }
    }
    return ret;
  }
} /* extern "C" */

extern "C" {
  static void
  Form_dealloc(Form_object *op)
  {
    Py_XDECREF(op->ob_fields);
    Py_XDECREF(op->ob_menu);

    delete op->ob_form;
    op->ob_form = NULL;

    PyObject_Del(op);
  }

  const static PyMethodDef Form_methods[] = {
    {"execute", (PyCFunction)Form_execute, METH_NOARGS},
    {"insert", (PyCFunction)Form_insert, METH_VARARGS},
    {"pop", (PyCFunction)Form_pop, METH_VARARGS},
    {NULL,              NULL}           // sentinel
  };

  const static PyMemberDef Form_memberlist[] = {
    {"flags", T_INT, offsetof(Form_object, ob_flags),
     0, "configuration flags"},
    {"save_hook", T_OBJECT, offsetof(Form_object, ob_save_hook),
     0, NULL},
    {"menu", T_OBJECT, offsetof(Form_object, ob_menu),
     0, NULL},
    {NULL}      /* Sentinel */
  };

  const static PySequenceMethods Form_as_sequence = {
    (inquiry)Form_length,       /*sq_length*/
    0,                          /*sq_concat*/
    0,                          /*sq_repeat*/
    (intargfunc)Form_item,      /*sq_item*/
    0,                          /*sq_slice*/
    (intobjargproc)Form_ass_item, /*sq_ass_item*/
    0,                          /*sq_ass_slice*/
    0,                          /*sq_contains*/
  };

  static const PyTypeObject c_Form_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "appuifw.Form",                            /*tp_name*/
    sizeof(Form_object),                       /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)Form_dealloc,                  /*tp_dealloc*/
    0,                                         /*tp_print*/
    0,                                         /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    (PySequenceMethods*)&Form_as_sequence,     /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
    0,                                         /*tp_call*/
    0,                                         /*tp_str*/
    PyObject_GenericGetAttr,                   /*tp_getattro*/
    PyObject_GenericSetAttr,                   /*tp_setattro*/
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                        /*tp_flags*/
    0,                                         /*tp_doc*/
    0,                                         /*tp_traverse*/
    0,                                         /*tp_clear*/
    0,                                         /*tp_richcompare*/
    0,                                         /*tp_weaklistoffset*/
    0,                                         /*tp_iter*/
    0,                                         /*tp_iternext*/
    (struct PyMethodDef *)Form_methods,        /*tp_methods*/
    (struct PyMemberDef *)Form_memberlist,     /*tp_members*/
  };
}       // extern "C"


/*
 * Module level implementation of module "appuifw". Notice that
 * finalize<module_name> is not a common practice for extension
 * modules -- in this particular case it is needed to manage
 * the lifetime of the "app" -object.
 */

extern "C" {

  static const PyMethodDef appuifw_methods[] = {
    /*    {"Application", (PyCFunction)new_application_object, METH_NOARGS, NULL}, */
    {"selection_list", (PyCFunction)selection_list, METH_VARARGS|METH_KEYWORDS, NULL},
    {"multi_selection_list", (PyCFunction)multi_selection_list, METH_VARARGS|METH_KEYWORDS, NULL},
    {"query", (PyCFunction)query, METH_VARARGS, NULL},
    {"Listbox", (PyCFunction)new_Listbox_object, METH_VARARGS, NULL},
    {"Content_handler", (PyCFunction)new_Content_handler_object, METH_VARARGS, NULL},
    {"note", (PyCFunction)note, METH_VARARGS, NULL},
    {"multi_query", (PyCFunction)Multi_line_data_query_dialog, METH_VARARGS, NULL},
    {"popup_menu", (PyCFunction)popup_menu, METH_VARARGS, NULL},
    {"available_fonts", (PyCFunction)available_fonts, METH_NOARGS, NULL},
    {"touch_enabled", (PyCFunction)touch_enabled, METH_NOARGS, NULL},
    {"Form", (PyCFunction)new_Form_object, METH_VARARGS|METH_KEYWORDS, NULL},
    {"Text", (PyCFunction)new_Text_object, METH_VARARGS, NULL},
    {"Canvas", (PyCFunction)new_Canvas_object, METH_VARARGS, NULL},
    {"Icon", (PyCFunction)new_Icon_object, METH_VARARGS, NULL},
#if SERIES60_VERSION>=30
    {"InfoPopup", (PyCFunction)new_InfoPopup_object, METH_VARARGS, NULL},
#endif
    {NULL,              NULL}           /* sentinel */
  };

}

void initappuifw(void)
{
  PyObject *m, *d;

  PyGILState_STATE gstate = PyGILState_Ensure();

  Application_type = c_Application_type;
  Application_type.ob_type = &PyType_Type;

  Listbox_type = c_Listbox_type;
  Listbox_type.ob_type = &PyType_Type;

  Text_type = c_Text_type;
  Text_type.ob_type = &PyType_Type;

  Form_type = c_Form_type;
  Form_type.ob_type = &PyType_Type;

  Content_handler_type = c_Content_handler_type;
  Content_handler_type.ob_type = &PyType_Type;

  Canvas_type = c_Canvas_type;
  Canvas_type.ob_type = &PyType_Type;

  Icon_type = c_Icon_type;
  Icon_type.ob_type = &PyType_Type;

#if SERIES60_VERSION>=30
  InfoPopup_type = c_InfoPopup_type;
  InfoPopup_type.ob_type = &PyType_Type;
#endif /* SERIES60_VERSION */

  m = Py_InitModule("_appuifw", (PyMethodDef*)appuifw_methods);
  d = PyModule_GetDict(m);

  PyDict_SetItemString(d, "FFormEditModeOnly", PyInt_FromLong(KFormEditModeOnly));
  PyDict_SetItemString(d, "FFormViewModeOnly", PyInt_FromLong(KFormViewModeOnly));
  PyDict_SetItemString(d, "FFormAutoLabelEdit", PyInt_FromLong(KFormAutoLabelEdit));
  PyDict_SetItemString(d, "FFormAutoFormEdit", PyInt_FromLong(KFormAutoFormEdit));
  PyDict_SetItemString(d, "FFormDoubleSpaced", PyInt_FromLong(KFormDoubleSpaced));
  /* text style flags */
  PyDict_SetItemString(d, "STYLE_BOLD", PyInt_FromLong(KBOLD));
  PyDict_SetItemString(d, "STYLE_UNDERLINE", PyInt_FromLong(KUNDERLINE));
  PyDict_SetItemString(d, "STYLE_ITALIC", PyInt_FromLong(KITALIC));
  PyDict_SetItemString(d, "STYLE_STRIKETHROUGH", PyInt_FromLong(KSTRIKETHROUGH));
  /* text highlight flags */
  PyDict_SetItemString(d, "HIGHLIGHT_STANDARD", PyInt_FromLong(KHIGHLIGHTNORMAL));
  PyDict_SetItemString(d, "HIGHLIGHT_ROUNDED", PyInt_FromLong(KHIGHLIGHTROUNDED));
  PyDict_SetItemString(d, "HIGHLIGHT_SHADOW", PyInt_FromLong(KHIGHLIGHTSHADOW));
  /* event codes for raw event processing */
  PyDict_SetItemString(d, "EEventKey", PyInt_FromLong(EEventKey));
  PyDict_SetItemString(d, "EEventKeyDown", PyInt_FromLong(EEventKeyDown));
  PyDict_SetItemString(d, "EEventKeyUp", PyInt_FromLong(EEventKeyUp));
#if SERIES60_VERSION>=28
  /* layout codes */
  PyDict_SetItemString(d, "EScreen", PyInt_FromLong(AknLayoutUtils::EScreen));
  PyDict_SetItemString(d, "EApplicationWindow", PyInt_FromLong(AknLayoutUtils::EApplicationWindow));
  PyDict_SetItemString(d, "EStatusPane", PyInt_FromLong(AknLayoutUtils::EStatusPane));
  PyDict_SetItemString(d, "EMainPane", PyInt_FromLong(AknLayoutUtils::EMainPane));
  PyDict_SetItemString(d, "EControlPane", PyInt_FromLong(AknLayoutUtils::EControlPane));
  PyDict_SetItemString(d, "ESignalPane", PyInt_FromLong(AknLayoutUtils::ESignalPane));
  PyDict_SetItemString(d, "EContextPane", PyInt_FromLong(AknLayoutUtils::EContextPane));
  PyDict_SetItemString(d, "ETitlePane", PyInt_FromLong(AknLayoutUtils::ETitlePane));
  PyDict_SetItemString(d, "EBatteryPane", PyInt_FromLong(AknLayoutUtils::EBatteryPane));
  PyDict_SetItemString(d, "EUniversalIndicatorPane", PyInt_FromLong(AknLayoutUtils::EUniversalIndicatorPane));
  PyDict_SetItemString(d, "ENaviPane", PyInt_FromLong(AknLayoutUtils::ENaviPane));
  PyDict_SetItemString(d, "EFindPane", PyInt_FromLong(AknLayoutUtils::EFindPane));
  PyDict_SetItemString(d, "EWallpaperPane", PyInt_FromLong(AknLayoutUtils::EWallpaperPane));
  PyDict_SetItemString(d, "EIndicatorPane", PyInt_FromLong(AknLayoutUtils::EIndicatorPane));
  PyDict_SetItemString(d, "EAColumn", PyInt_FromLong(AknLayoutUtils::EAColunm));
  PyDict_SetItemString(d, "EBColumn", PyInt_FromLong(AknLayoutUtils::EBColunm));
  PyDict_SetItemString(d, "ECColumn", PyInt_FromLong(AknLayoutUtils::ECColunm));
  PyDict_SetItemString(d, "EDColumn", PyInt_FromLong(AknLayoutUtils::EDColunm));
  PyDict_SetItemString(d, "EStaconTop", PyInt_FromLong(AknLayoutUtils::EStaconTop));
  PyDict_SetItemString(d, "EStaconBottom", PyInt_FromLong(AknLayoutUtils::EStaconBottom ));
  PyDict_SetItemString(d, "EStatusPaneBottom", PyInt_FromLong(AknLayoutUtils::EStatusPaneBottom));
  PyDict_SetItemString(d, "EControlPaneBottom", PyInt_FromLong(AknLayoutUtils::EControlPaneBottom));
  PyDict_SetItemString(d, "EControlPaneTop", PyInt_FromLong(AknLayoutUtils::EControlPaneTop));
  PyDict_SetItemString(d, "EStatusPaneTop", PyInt_FromLong(AknLayoutUtils::EStatusPaneTop));
#endif /* SERIES60_VERSION */
#if SERIES60_VERSION>=30
  PyDict_SetItemString(d, "EHLeftVTop", PyInt_FromLong(EHLeftVTop));
  PyDict_SetItemString(d, "EHLeftVCenter", PyInt_FromLong(EHLeftVCenter));
  PyDict_SetItemString(d, "EHLeftVBottom", PyInt_FromLong(EHLeftVBottom));
  PyDict_SetItemString(d, "EHCenterVTop", PyInt_FromLong(EHCenterVTop));
  PyDict_SetItemString(d, "EHCenterVCenter", PyInt_FromLong(EHCenterVCenter));
  PyDict_SetItemString(d, "EHCenterVBottom", PyInt_FromLong(EHCenterVBottom));
  PyDict_SetItemString(d, "EHRightVTop", PyInt_FromLong(EHRightVTop));
  PyDict_SetItemString(d, "EHRightVCenter", PyInt_FromLong(EHRightVCenter));
  PyDict_SetItemString(d, "EHRightVBottom", PyInt_FromLong(EHRightVBottom));
#endif /* SERIES60_VERSION */

  PyObject* app = SPy_S60app_New();
  if (app)
    PyDict_SetItemString(d, "app", app);

  PyGILState_Release(gstate);
}

void finalizeappuifw(void)
{
  PyObject *d, *m, *a;

  PyGILState_STATE gstate = PyGILState_Ensure();

  PyInterpreterState *interp = PyThreadState_Get()->interp;
  PyObject *modules = interp->modules;

  m = PyDict_GetItemString(modules, "_appuifw");
  if (m) {
    d = PyModule_GetDict(m);
    a = PyDict_GetItemString(d, "app");
    /*      PyDict_DelItemString(d, "app"); */
    Py_XDECREF(a);
    /*      application_dealloc((Application_object *)a); */
  }

  PyGILState_Release(gstate);
}

