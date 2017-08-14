/**
 * ====================================================================
 *  e32dbmodule.cpp
 *
 *  Python API to Symbian DB. 
 *
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
 *
 * ====================================================================
 */

#include "Python.h"
#include "symbian_python_ext_util.h"


#include <eikenv.h>
#include <e32std.h>
#include <f32file.h>
#include <d32dbms.h>

#include "debugutil.h"

//#define IFDEB 
#define IFDEB if (0) 
#define DPYPRINTF IFDEB pyprintf

/*
 *
 * Implementation of e32db.Dbms
 *
 */

/*
 * Implementation of Dbms object
 */

#define Dbms_type ((PyTypeObject*)SPyGetGlobalString("DbmsType"))

struct DB_data {
  RDbs  dbsession;
  RDbNamedDatabase dbname;
  bool dbOpen;
  TBuf<KMaxFileName> fullName;
};

/*
 *  No class members here
 *  as they do not get
 *  properly initialized by PyObject_New() !!!
 */
struct dbms_object {
  PyObject_VAR_HEAD
  DB_data* db_obj;
};

extern "C" PyObject *
new_dbms_object()
{
  dbms_object *dbo = PyObject_New(dbms_object, Dbms_type);
  if (dbo == NULL)
    return PyErr_NoMemory();

  dbo->db_obj = new DB_data();
  dbo->db_obj->dbOpen=false;
  if (dbo->db_obj == NULL) {
    PyObject_Del(dbo);
    return PyErr_NoMemory();
  }

  return (PyObject *) dbo;
}

extern "C" PyObject *
dbms_create(dbms_object *self, PyObject *args)
{
  TInt error = KErrNone;
  int l;
  char* b;

  if (!PyArg_ParseTuple(args, "u#", &b, &l))
    return NULL;

  self->db_obj->fullName.Copy((TUint16*)b, l);

  /*RFs session needed*/
  RFs* rfs = new RFs();
  if (rfs == NULL)
    return PyErr_NoMemory();
  
  error = rfs->Connect();
  
  if (error == KErrNone) {
    // Make sure the database is closed before we try to Replace it.
    self->db_obj->dbname.Close();
    error = self->db_obj->dbname.Replace(*rfs, self->db_obj->fullName);
  }
  self->db_obj->dbname.Close();
  self->db_obj->dbOpen = false;
  rfs->Close();
  delete rfs;

  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
dbms_open(dbms_object *self, PyObject *args)
{
  TInt error = KErrNone;
  int l;
  char* b;

  if (!PyArg_ParseTuple(args, "u#", &b, &l))
    return NULL;

  self->db_obj->fullName.Copy((TUint16*)b, l);
  
  error = self->db_obj->dbsession.Connect();
  if (error == KErrNone) {
    // Make sure the database is closed before we try to open a new one.
    self->db_obj->dbname.Close();
    error = self->db_obj->dbname.Open(self->db_obj->dbsession, self->db_obj->fullName);
  }
  self->db_obj->dbOpen = (error == KErrNone);
  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
dbms_close(dbms_object *self)
{
  self->db_obj->dbname.Close();
  self->db_obj->dbOpen=false;

  Py_INCREF(Py_None);
  return Py_None;
}

#define ASSERT_DBOPEN						\
  if (!self->db_obj->dbOpen) {					\
    PyErr_SetString(PyExc_RuntimeError, "Database not open");	\
    return NULL;						\
  }

#define ASSERT_ATROW						\
  if (!self->view_obj->view.AtRow()) {				\
    PyErr_SetString(PyExc_RuntimeError, "Not on a row");	\
    return NULL;						\
  }

#define ASSERT_ROWREADY							\
  if (!self->view_obj->rowReady) {					\
    PyErr_SetString(PyExc_RuntimeError, "Row not ready (did you remember to call get_line?)"); \
    return NULL;							\
  }

#define ASSERT_ISPREPARED					\
  if (!self->view_obj->isPrepared) {				\
    PyErr_SetString(PyExc_RuntimeError, "View not prepared");	\
    return NULL;						\
  }

#define ASSERT_VALIDCOLUMN						\
  TInt nCols = self->view_obj->view.ColCount();				\
  if ((nCols<=0) || (column <=0) || (column > nCols)) {			\
    PyErr_SetString(PyExc_RuntimeError, "Invalid column number");	\
    return NULL;							\
  }

extern "C" PyObject *
dbms_begin(dbms_object *self)
{
  ASSERT_DBOPEN;
  self->db_obj->dbname.Begin();
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
dbms_commit(dbms_object *self)
{
  ASSERT_DBOPEN;
  self->db_obj->dbname.Commit();
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
dbms_rollback(dbms_object *self)
{
  ASSERT_DBOPEN;
  self->db_obj->dbname.Rollback();
  Py_INCREF(Py_None);
  return Py_None;
}

extern "C" PyObject *
dbms_compact(dbms_object *self)
{
  ASSERT_DBOPEN;
  self->db_obj->dbname.Compact();
  Py_INCREF(Py_None);
  return Py_None;
}


extern "C" PyObject *
dbms_execute(dbms_object *self, PyObject *args)
{
  TInt updated;
  int l;
  char* b;

  if (!PyArg_ParseTuple(args, "u#", &b, &l))
    return NULL;

  ASSERT_DBOPEN;

  TPtrC query((TUint16 *)b, l);

  updated = self->db_obj->dbname.Execute(query);
  if (updated < 0)
    return SPyErr_SetFromSymbianOSErr(updated);
  else
    return Py_BuildValue("i", updated);
}

extern "C" {

  static void dbms_dealloc(dbms_object *dbo)
  {
    dbo->db_obj->dbname.Close();
    dbo->db_obj->dbsession.Close();
    delete dbo->db_obj;
    PyObject_Del(dbo);
  }

  const static PyMethodDef dbms_methods[] = {
    {"create", (PyCFunction)dbms_create, METH_VARARGS},
    {"execute", (PyCFunction)dbms_execute, METH_VARARGS},
    {"open", (PyCFunction)dbms_open, METH_VARARGS},
    {"close", (PyCFunction)dbms_close, METH_NOARGS},
    {"begin", (PyCFunction)dbms_begin, METH_NOARGS},
    {"commit", (PyCFunction)dbms_commit, METH_NOARGS},
    {"rollback", (PyCFunction)dbms_rollback, METH_NOARGS},
    {"compact", (PyCFunction)dbms_compact, METH_NOARGS},
    {NULL, NULL}           // sentinel
  };

  static PyObject *
  dbms_getattr(dbms_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)dbms_methods,
                         (PyObject *)op, name);
  }

  static const PyTypeObject c_dbms_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                         /*ob_size*/
    "e32db.Dbms",                             /*tp_name*/
    sizeof(dbms_object),                     /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    /* methods */
    (destructor)dbms_dealloc,                /*tp_dealloc*/
    0,                                         /*tp_print*/
    (getattrfunc)dbms_getattr,               /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_compare*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
  };
} //extern C

/*
 *
 * Implementation of e32db.Db_view
 *
 */

/*
 * Implementation of Db_view object
 */

#define DBView_type ((PyTypeObject*)SPyGetGlobalString("DBViewType"))

struct View_data {
  RDbView view;
  int isPrepared;
  int rowReady;
};

/*
 *  No class members here
 *  as they do not get
 *  properly initialized by PyObject_New() !!!
 */
struct dbview_object {
  PyObject_VAR_HEAD
  View_data* view_obj;
  struct dbms_object* dbms_obj;
};

extern "C" PyObject *
new_dbview_object()
{
  dbview_object *dbv = PyObject_New(dbview_object, DBView_type);
  if (dbv == NULL)
    return PyErr_NoMemory();

  dbv->view_obj = new View_data();
  dbv->view_obj->isPrepared=0;
  dbv->view_obj->rowReady=0;
  dbv->dbms_obj=NULL;
  if (dbv->view_obj == NULL) {
    PyObject_Del(dbv);
    return PyErr_NoMemory();
  }
  else
    return (PyObject *) dbv;
}

static void
dbview_dealloc(dbview_object *dbv)
{
  dbv->view_obj->view.Close();
  delete dbv->view_obj;
  // If the view has been prepared to refer to some database object,
  // decrease its' reference count, so that it gets properly destroyed
  // when there are no more references to it. When doing the cleanup
  // it is crucial that the database is not destroyed before the views
  // that refer to it.
  if (dbv->dbms_obj)
    Py_DECREF(dbv->dbms_obj);
  PyObject_Del(dbv);  
}

extern "C" PyObject *
dbview_prepare(dbview_object *self, PyObject *args)
{
  TInt error = KErrNone;
  int l;
  char* b;
  dbms_object *db;

  if (!PyArg_ParseTuple(args, "O!u#", Dbms_type, &db, &b, &l))
    return NULL;

  // This is essentially ASSERT_DBOPEN but we can't use that here
  // because the name self refers to the view object, not the database
  // object.
  if (!db->db_obj->dbOpen) {					
    PyErr_SetString(PyExc_RuntimeError, "Database not open");	
    return NULL;						
  }

  // If this view has already been prepared to refer to some database,
  // decrease the reference count of that old database object.
  if (self->dbms_obj) 
    Py_DECREF(self->dbms_obj);
  self->dbms_obj=db;
  // Once we prepare a view to some database, that view refers to
  // the original database and thus the database object must not be
  // destroyed before the view is destroyed. Increase the reference
  // count of the database object to tell the Python interpreter of
  // this C-level dependency.
  Py_INCREF(self->dbms_obj);

  TPtrC query((TUint16 *)b, l);

  error = self->view_obj->view.Prepare(db->db_obj->dbname, (TDbQuery)query, RDbView::EReadOnly);

  if (error == KErrNone)
    error = self->view_obj->view.EvaluateAll();
  if (error == KErrNone) {
    TRAP(error, (self->view_obj->view.FirstL() ));
  }
  self->view_obj->isPrepared=(error == KErrNone);
  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
dbview_first_line(dbview_object *self)
{
  ASSERT_ISPREPARED;
  TRAPD(error, (self->view_obj->view.FirstL() ));
  self->view_obj->rowReady=0;
  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
dbview_count_line(dbview_object *self)
{
  ASSERT_ISPREPARED;
  TInt number = 0;

  TRAPD(error, (number = self->view_obj->view.CountL() ));
  if (error != KErrNone)
    return SPyErr_SetFromSymbianOSErr(error);
  else
    return Py_BuildValue("i", number);
}

extern "C" PyObject *
dbview_colCount(dbview_object *self)
{
  ASSERT_ISPREPARED;
  return Py_BuildValue("i", self->view_obj->view.ColCount());
}

extern "C" PyObject *
dbview_next_line(dbview_object *self)
{
  ASSERT_ISPREPARED;
  if (self->view_obj->view.AtEnd()) {
    PyErr_SetString(PyExc_RuntimeError, "End of view, there is no next line.");
    return NULL;
  }
  // Note: Due to a bug in Symbian, NextL will crash the entire system 
  // if you go beyond the last row in the view.
  TRAPD(error, (self->view_obj->view.NextL() ));
  self->view_obj->rowReady=0;
  RETURN_ERROR_OR_PYNONE(error);
}

extern "C" PyObject *
dbview_get_line(dbview_object *self)
{
  ASSERT_ISPREPARED;
  ASSERT_ATROW;
  TRAPD(error, (self->view_obj->view.GetL() ));
  self->view_obj->rowReady=(error == KErrNone);
  RETURN_ERROR_OR_PYNONE(error);
}

static PyObject *
PyUnicode_FromColumnL(RDbView& view, TDbColNo column)
{
  RDbColReadStream readStream;    
  DPYPRINTF("xopenlc");
  readStream.OpenLC(view, column);
  DPYPRINTF("get collength");
  int colLength=view.ColLength(column);
  DPYPRINTF("collength %d", colLength);
  DPYPRINTF("fromunicode");
  PyObject *obj=PyUnicode_FromUnicode(NULL,colLength);
  DPYPRINTF("asunicode");
  TPtr16 ptr(PyUnicode_AsUnicode(obj),colLength);
  DPYPRINTF("readl");
  readStream.ReadL(ptr);
  DPYPRINTF("pop");
  CleanupStack::PopAndDestroy(&readStream);
  DPYPRINTF("return");
  return obj;
}


// Extracts a Unicode object from a LONG VARCHAR column.
static PyObject *
col_longtext(dbview_object *self, PyObject *args)
{
  ASSERT_ISPREPARED;
  ASSERT_ATROW;
  ASSERT_ROWREADY;
  int column = 0;
  if (!PyArg_ParseTuple(args, "i", &column))
    return NULL;

  ASSERT_VALIDCOLUMN;

  // GCC and MSVC6 interpret this differently.
  // MSVC6 sees this as a _function_ declaration.
  // RDbView& view(self->view_obj->view); 
  RDbView& view=self->view_obj->view;
  /* check if the given column is the proper type*/
  if ( (view.ColType((TDbColNo)column)) != EDbColLongText) {
    PyErr_SetString(PyExc_TypeError, "Wrong column type. Required EDbColLongText");
    return NULL;
  } 
 
  PyObject *obj=NULL;
  DPYPRINTF("Getting column value...");
  TRAPD(error,obj=PyUnicode_FromColumnL(view,column));
  DPYPRINTF("Got column value.");
  if (error != KErrNone) {
    pyprintf("Symbian error %d\n",error);
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return obj;
}

extern "C" PyObject *
dbview_col_raw(dbview_object *self, PyObject *args)
{
  ASSERT_ISPREPARED;
  ASSERT_ATROW;
  ASSERT_ROWREADY;
  int column = 0;

  if (!PyArg_ParseTuple(args, "i", &column))
    return NULL;

  ASSERT_VALIDCOLUMN;
  
  /* check if the given column is the proper type*/
  int coltype=self->view_obj->view.ColType((TDbColNo)column);
  if ( coltype == EDbColLongText || coltype == EDbColLongBinary) {
    PyErr_SetString(PyExc_TypeError, "This function doesn't support LONG columns.");
    return NULL;
  } 
  TPtrC8 text = (self->view_obj->view.ColDes8((TDbColNo)column));
  return Py_BuildValue("s#", text.Ptr(), text.Length());
}

extern "C" PyObject *
dbview_col_rawtime(dbview_object *self, PyObject *args)
{
  ASSERT_ISPREPARED;
  ASSERT_ATROW;
  ASSERT_ROWREADY;
  int column = 0;
  if (!PyArg_ParseTuple(args, "i", &column))
    return NULL;
  ASSERT_VALIDCOLUMN;
  /* check if the given column is the proper type*/
  int coltype=self->view_obj->view.ColType((TDbColNo)column);
  if ( coltype != EDbColDateTime ) {
    PyErr_SetString(PyExc_TypeError, "Column must be of date/time type.");
    return NULL;
  } 
  return Py_BuildValue("L", self->view_obj->view.ColTime(column).Int64());    
}

// Generic column access function. 
// Returns the proper Python value for the given column type.
extern "C" PyObject *
dbview_col(dbview_object *self, PyObject *args)
{
  ASSERT_ISPREPARED;
  ASSERT_ATROW;
  ASSERT_ROWREADY;
  int column;
  if (!PyArg_ParseTuple(args, "i", &column))
    return NULL;
  ASSERT_VALIDCOLUMN;
  RDbView& view=self->view_obj->view;
  double doubleValue;
  switch (view.ColType(column)) {
  case EDbColLongText: 
    return col_longtext(self,args);
  case EDbColText16: // VARCHAR, CHAR
    {
      TPtrC16 textValue = view.ColDes16(column);
      return Py_BuildValue("u#", textValue.Ptr(), textValue.Length());
    }
  case EDbColText8:    
  case EDbColBinary:
    return dbview_col_raw(self,args);
  case EDbColDateTime: 
    {     
      // There will be roundoff errors here.
#ifndef EKA2
      double unixtime=
	(view.ColTime(column).Int64()-TTime(KUnixEpoch).Int64()).GetTReal();
#else
      double unixtime= 
	TReal64(view.ColTime(column).Int64()-TTime(KUnixEpoch).Int64());
#endif
      unixtime/=1e6;
      unixtime-=(TLocale().UniversalTimeOffset()).Int();
      return Py_BuildValue("d", unixtime);
    } 
  case EDbColInt64:    
    return Py_BuildValue("L", view.ColInt64(column));    
  case EDbColInt32:   // INTEGER
  case EDbColInt16:   // SMALLINT
  case EDbColInt8:    // TINYINT
    return Py_BuildValue("l", view.ColInt32(column));
  case EDbColUint32:  // UNSIGNED INTEGER
  case EDbColUint16:  // UNSIGNED SMALLINT
  case EDbColUint8:   // UNSIGNED TINYINT
  case EDbColBit:     // BIT
    return Py_BuildValue("l", view.ColUint32(column));
  case EDbColReal32:  // REAL
    doubleValue=view.ColReal32(column);
    return Py_BuildValue("d", doubleValue);
  case EDbColReal64:  // FLOAT, DOUBLE, DOUBLE PRECISION
    doubleValue=view.ColReal64(column);
    return Py_BuildValue("d", doubleValue);
  default:
    char buf[80];
    sprintf(buf,"Unknown column type: %d %d",view.ColType(column),EDbColBit);
    PyErr_SetString(PyExc_TypeError, buf);
    return NULL;    
  }
}

/* Interpret the given long integer as a Symbian time value
   (microseconds since 0 AD, local time) and return the value as a
   Unicode string, formatted in a form that, surrounded with #-marks,
   is suitable for use as a date/time literal in an SQL INSERT or
   UPDATE statement. */

_LIT(KFormatTxt,"%1-%2-%3 %-B%J:%T:%S.%C%+B");

static PyObject *
format_ttime(TTime timeValue)
{
  TBuf<32> timeString;  
  TRAPD(error,(timeValue.FormatL(timeString,KFormatTxt)));
  if (error != KErrNone) {
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return Py_BuildValue("u#",timeString.Ptr(),timeString.Length());
}


extern "C" PyObject *
format_rawtime(PyObject *self, PyObject *args)
{
#ifdef EKA2
  PY_LONG_LONG tv;
  if (!PyArg_ParseTuple(args, "L", &tv))
    return NULL;
  unsigned int tv_low, tv_high; 
  /* Assuming little-endian platform here. */
  tv_low=*(unsigned int *)(&tv); 
  tv_high=*((unsigned int *)(&tv)+1);
  TInt64 hi_and_lo;
  TUint32* lo=(TUint32*)&hi_and_lo;
  TUint32* hi=((TUint32*)&hi_and_lo)+1;
  *lo=tv_low;
  *hi=tv_high;
  TTime timeValue=hi_and_lo;
  return format_ttime(timeValue);  
#else  
  PY_LONG_LONG tv;
  if (!PyArg_ParseTuple(args, "L", &tv))
    return NULL;
  unsigned int tv_low, tv_high; 
  /* Assuming little-endian platform here. I'm sure hell will freeze
     over before that changes. */
  tv_low=*(unsigned int *)(&tv); 
  tv_high=*((unsigned int *)(&tv)+1);
  TTime timeValue=TInt64(tv_high, tv_low);
  return format_ttime(timeValue);
#endif
}


extern "C" PyObject *
format_time(PyObject *self, PyObject *args)
{
  double unixtime;
  if (!PyArg_ParseTuple(args, "d", &unixtime))
    return NULL;
  TTime timeValue=
    TTime(TTime(KUnixEpoch).Int64()+
	  (TInt64(unixtime)+TInt64(TLocale().UniversalTimeOffset().Int()))*TInt64(1e6));
  return format_ttime(timeValue);
}

extern "C" PyObject *
dbview_col_length(dbview_object *self, PyObject *args)
{
  ASSERT_ISPREPARED;
  ASSERT_ATROW;
  ASSERT_ROWREADY;
  int column = 0;
  TInt value = 0;
  if (!PyArg_ParseTuple(args, "i", &column))
    return NULL;
  ASSERT_VALIDCOLUMN;
  value = self->view_obj->view.ColLength((TDbColNo)column);
  return Py_BuildValue("i", value);
}

extern "C" PyObject *
dbview_col_type(dbview_object *self, PyObject *args)
{
  ASSERT_ISPREPARED;
  int column = 0;
  if (!PyArg_ParseTuple(args, "i", &column))
    return NULL;
  ASSERT_VALIDCOLUMN;
  return Py_BuildValue("i", self->view_obj->view.ColType((TDbColNo)column));
}

extern "C" PyObject *
dbview_is_col_null(dbview_object *self, PyObject *args)
{
  ASSERT_ISPREPARED;
  ASSERT_ATROW;
  ASSERT_ROWREADY;
  int column = 0;
  if (!PyArg_ParseTuple(args, "i", &column))
    return NULL;
  ASSERT_VALIDCOLUMN;
  PyObject* rval = (self->view_obj->view.IsColNull((TDbColNo)column) 
		    ? Py_True : Py_False);
  Py_INCREF(rval);
  return rval;
}


const static PyMethodDef dbview_methods[] = { 
  {"col_count", (PyCFunction)dbview_colCount, METH_NOARGS},
  {"col_raw", (PyCFunction)dbview_col_raw, METH_VARARGS},
  {"col_length", (PyCFunction)dbview_col_length, METH_VARARGS},
  {"col_type", (PyCFunction)dbview_col_type, METH_VARARGS},
  {"col", (PyCFunction)dbview_col, METH_VARARGS},
  {"col_rawtime", (PyCFunction)dbview_col_rawtime, METH_VARARGS},
  {"count_line", (PyCFunction)dbview_count_line, METH_NOARGS},
  {"first_line", (PyCFunction)dbview_first_line, METH_NOARGS},
  {"get_line", (PyCFunction)dbview_get_line, METH_NOARGS},
  {"is_col_null", (PyCFunction)dbview_is_col_null, METH_VARARGS},
  {"next_line", (PyCFunction)dbview_next_line, METH_NOARGS},
  {"prepare", (PyCFunction)dbview_prepare, METH_VARARGS},
  {NULL, NULL}           // sentinel
};

static PyObject *
dbview_getattr(dbview_object *op, char *name)
{
  return Py_FindMethod((PyMethodDef*)dbview_methods,
                       (PyObject *)op, name);
}

static const PyTypeObject c_dbview_type = {
  PyObject_HEAD_INIT(NULL)
  0,                                         /*ob_size*/
  "e32db.Db_view",                             /*tp_name*/
  sizeof(dbview_object),                     /*tp_basicsize*/
  0,                                         /*tp_itemsize*/
  /* methods */
  (destructor)dbview_dealloc,                /*tp_dealloc*/
  0,                                         /*tp_print*/
  (getattrfunc)dbview_getattr,               /*tp_getattr*/
  0,                                         /*tp_setattr*/
  0,                                         /*tp_compare*/
  0,                                         /*tp_repr*/
  0,                                         /*tp_as_number*/
  0,                                         /*tp_as_sequence*/
  0,                                         /*tp_as_mapping*/
  0,                                         /*tp_hash*/
};

extern "C" {

  static const PyMethodDef e32db_methods[] = {
    {"Dbms", (PyCFunction)new_dbms_object, METH_NOARGS, NULL},
    {"Db_view", (PyCFunction)new_dbview_object, METH_NOARGS, NULL},
    {"format_time", (PyCFunction)format_time, METH_VARARGS, NULL},
    {"format_rawtime", (PyCFunction)format_rawtime, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) inite32db(void)
  {
    PyTypeObject* dbms_type = PyObject_New(PyTypeObject, &PyType_Type);
    *dbms_type = c_dbms_type;
    dbms_type->ob_type = &PyType_Type;

    PyTypeObject* dbview_type = PyObject_New(PyTypeObject, &PyType_Type);
    *dbview_type = c_dbview_type;
    dbview_type->ob_type = &PyType_Type;

    SPyAddGlobalString("DbmsType", (PyObject*)dbms_type);
    SPyAddGlobalString("DBViewType", (PyObject*)dbview_type);

    Py_InitModule("e32db", (PyMethodDef*)e32db_methods);

    return;
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif
