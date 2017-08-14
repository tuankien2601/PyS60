/*
 * ====================================================================
 *  locationacqmodule.cpp
 *
 * Copyright (c) 2007-2008 Nokia Corporation
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
 *  Python API for getting satellitite and position information
 * 
 *  Implements currently following Python type:
 *    
 *  PositionServer
 *    positioner
 *    - Create (and connect) new Positioner object.
 *  
 *    default_module
 *    - Returns the default module id.
 *
 *    modules
 *    - Return information about the available positioning modules.   
 *  
 *    module_info
 *    - Return information about the specified position module.
 *  
 *  Positioner
 *    set_requestors
 *    - Set requestors.
 *    - Requestors must be given in a list.
 *    - Each requestor is represented as a dictionary.
 *     
 *    position
 *    - Returns position infomation.
 *   
 *    stop_position
 *    - Stops the position feed
 *    
 *    last_position
 *    - returns last position information 
 * ====================================================================
 */

#include "locationacqmodule.h"

/*
 * Debug print.
 */
/*void
debugprint(const char *f, ...)
{
  va_list a;
  va_start(a,f);
  char buffer[256];
  vsprintf(buffer, f, a);    
  char buffer2[256];
  sprintf(buffer2, "print '%s'", buffer);
  PyRun_SimpleString(buffer2);
} */

/*
 * Python LONG_LONG to TTimeIntervalMicroSeconds. 
 */
static TTimeIntervalMicroSeconds lltoTIMS(const PY_LONG_LONG& tv)
{
  // from e32dbmodule.cpp:
#ifdef EKA2
  unsigned int tv_low, tv_high; 
  tv_low=*(unsigned int *)(&tv); 
  tv_high=*((unsigned int *)(&tv)+1);
  TInt64 hi_and_lo;
  TUint32* lo=(TUint32*)&hi_and_lo;
  TUint32* hi=((TUint32*)&hi_and_lo)+1;
  *lo=tv_low;
  *hi=tv_high;
  return hi_and_lo;
#else
  unsigned int tv_low, tv_high; 
  tv_low=*(unsigned int *)(&tv); 
  tv_high=*((unsigned int *)(&tv)+1);
  return TInt64(tv_high, tv_low);
#endif
}

/*
 * Create new PositionServer object.
 */
extern "C" PyObject *
new_PositionServer_object()
{
  // create PositionServer_object.
  PositionServer_object* posServ = 
    PyObject_New(PositionServer_object, PositionServer_type);
  if (posServ == NULL){
    return PyErr_NoMemory();
  }
  // initialize PositionServer_object's pointers.
  posServ->posServ=NULL;
  
  // create symbian object RPositionServer and connect it.
  RPositionServer* positionServer=NULL;
  TInt err=KErrNone;
  TRAP(err,{
    positionServer=new (ELeave) RPositionServer;
    User::LeaveIfError(positionServer->Connect());
    posServ->posServ=positionServer;
  });
  if(err!=KErrNone){
    delete positionServer;
    Py_DECREF(posServ);
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  return (PyObject*)posServ;
}


/*
 * Create (and connect) new Positioner object.
 */
extern "C" PyObject *
PositionServer_positioner(PositionServer_object* self, PyObject* args, PyObject* keywds)
{
  TInt id=KNullUidValue;
  
  static const char *const kwlist[] = 
    {"module_id", NULL};
 
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "|i", (char**)kwlist, &id)){ 
    return NULL;
  }
  TPositionModuleId moduleUid={id};
  return new_Positioner_object(self,moduleUid);
}


/*
 * Return the default module id.
 */
extern "C" PyObject *
PositionServer_default_module(PositionServer_object* self, PyObject* /*args*/)
{
  TInt err=KErrNone;
  TPositionModuleId moduleId;
  
  err = self->posServ->GetDefaultModuleId(moduleId);
  if(err!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(err);
  }
  return Py_BuildValue("i",moduleId);
}


/*
 * Return information about the available positioning modules.
 */
extern "C" PyObject *
PositionServer_modules(PositionServer_object* self, PyObject* /*args*/)
{  
  TUint numOfModules=0;
  TInt err=KErrNone;
  
  err = self->posServ->GetNumModules(numOfModules);
  if(err!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  TPositionModuleInfo moduleInfo;
  TBuf<KPositionMaxModuleName> moduleName;
  PyObject* modules=PyList_New(numOfModules);
  if(modules==NULL){
    return PyErr_NoMemory();
  }
  
  for(TInt i=0;i<(TInt)numOfModules;i++){
    self->posServ->GetModuleInfoByIndex(i,moduleInfo);
    moduleInfo.GetModuleName(moduleName);
    PyObject* moduleDict = Py_BuildValue("{s:i,s:i,s:u#}",
                                         "id",moduleInfo.ModuleId(),
                                         "available",moduleInfo.IsAvailable(),
                                         "name",moduleName.Ptr(),moduleName.Length()); 
    if(moduleDict==NULL){
      Py_DECREF(modules);
      return NULL;
    }
    if(PyList_SetItem(modules, i, moduleDict)){
      Py_DECREF(modules);
      return NULL;
    };
  }
  return (PyObject*)modules;
}


/*
 * Return information about the specified position module.
 */
extern "C" PyObject *
PositionServer_module_info(PositionServer_object* self, PyObject* args)
{
  TInt moduleId = KNullUidValue;
  TInt err=KErrNone;
  
  if (!PyArg_ParseTuple(args, "i", &moduleId)){ 
    return NULL;
  }
  
  TPositionModuleId moduleUid={moduleId};
  TPositionModuleInfo info;
  err = self->posServ->GetModuleInfoById(moduleUid, info); 
  if(KErrNone!=err){
    return SPyErr_SetFromSymbianOSErr(err);
  }
    
  TBuf<KPositionMaxModuleName> moduleName;
  info.GetModuleName(moduleName);
  
  TPositionQuality posQual;
  info.GetPositionQuality(posQual);
  
  TPositionModuleStatus status;
  self->posServ->GetModuleStatus(status, moduleUid); 
  if(KErrNone!=err){
    return SPyErr_SetFromSymbianOSErr(err);
  }
 
  PyObject* positionQuality = Py_BuildValue("{s:d,s:d,s:i,s:i,s:L,s:L}",
                                            "horizontal_accuracy",posQual.HorizontalAccuracy(),
                                            "vertical_accuracy",posQual.HorizontalAccuracy(),
                                            "power_consumption",posQual.PowerConsumption(),
                                            "cost",posQual.CostIndicator(),
                                            "time_to_next_fix",posQual.TimeToNextFix().Int64(),
                                            "time_to_first_fix",posQual.TimeToNextFix().Int64());
  if(positionQuality==NULL){
    return NULL;
  }
                                            
  PyObject* moduleStatus = Py_BuildValue("{s:i,s:i}", 
                                   "device_status",status.DeviceStatus(),
                                   "data_quality",status.DataQualityStatus());   
  if(moduleStatus==NULL){
    Py_DECREF(positionQuality);
    return NULL;
  }                                     
                                                                                                             
  return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:u#,s,u#,s:N,s:N}",
                        "available",info.IsAvailable(),
                        "id",info.ModuleId(),
                        "technology",info.TechnologyType(),
                        "location",info.DeviceLocation(),
                        "capabilities",info.Capabilities(),
                        "name",moduleName.Ptr(),moduleName.Length(),
                        "version",info.Version().Name().Ptr(),info.Version().Name().Length(),
                        "position_quality",positionQuality,
                        "status",moduleStatus);                     
}


/*
 * Deallocate the PositionServer object.
 */
extern "C" {
  static void PositionServer_dealloc(PositionServer_object *posServ)
  {
    //debugprint("position server dealloc"); // ??? debug
    if (posServ->posServ) {
       posServ->posServ->Close();
       delete posServ->posServ;
       posServ->posServ = NULL;
    }   
    PyObject_Del(posServ);
  }
}


/*
 * Create new Positioner object.
 */
extern "C" PyObject *
new_Positioner_object(PositionServer_object* posServ, TPositionModuleId moduleUid)
{
  // create Positioner_object.
  Positioner_object* positioner = 
    PyObject_New(Positioner_object, Positioner_type);
  if (positioner == NULL){
    return PyErr_NoMemory();
  }
  
  // initialize Subsession_object's pointers.
  positioner->positionServer=NULL;
  positioner->positioner=NULL;
  positioner->requestorStack=NULL;
  positioner->dataFetcher=NULL;
    
  positioner->moduleid=moduleUid;  
  positioner->isSubSessionOpen=EFalse;
  // create symbian object RPositioner and connect it.
  RPositioner* pos=NULL;
  TInt err=KErrNone;
  TRAP(err,{
    pos=new (ELeave) RPositioner;
    positioner->positioner=pos;
  });
  if(err!=KErrNone){
    delete pos;
    Py_DECREF(positioner);
    return SPyErr_SetFromSymbianOSErr(err);
  }
  
  // XXX add dataFetcher creation here?
  
  positioner->callBackSet=EFalse;
  
  positioner->positionServer=posServ;
  Py_INCREF(posServ);
  return (PyObject*)positioner;
}


/*
 * Set requestors.
 * Requestors must be given in a list.
 * Each requestor is represented as a dictionary.
 */
extern "C" PyObject *
Positioner_set_requestors(Positioner_object *self,PyObject* args)
{
  TInt err=KErrNone;
  if(!self->isSubSessionOpen){
    err=opensubsession(self);
    if(err!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(err);
    }
  }
  
  PyObject* requestorList=NULL;
  if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &requestorList)){ 
    return NULL;
  }
  
  if(PyList_Size(requestorList)<1){
    PyErr_SetString(PyExc_SyntaxError, 
                    "no requestors given in the list");
    return NULL;
  }
    
  // Create requestor object.
  deleteRequestorStack(self);
  TRAP(err,{
    self->requestorStack=new (ELeave) RRequestorStack;
  });
  if(KErrNone!=err){
    return SPyErr_SetFromSymbianOSErr(err);
  }

  TInt reqType=0;
  TInt reqFormat=0;
  for(int i=0;i<PyList_Size(requestorList);i++){
    PyObject* requestor = PyList_GetItem(requestorList,i);
    if(!PyDict_Check(requestor)){
      PyErr_SetString(PyExc_TypeError, 
                    "requestors must be dictonaries");
      return NULL;
    };
 
    PyObject* key = Py_BuildValue("s","type");
    if(key==NULL){
      return NULL;
    }
    PyObject* value = PyDict_GetItem(requestor,key);
    Py_DECREF(key);
    if(!PyInt_Check(value)){
      PyErr_SetString(PyExc_TypeError, 
                     "illegal requestor type information");
      return NULL;
    }
    reqType=PyInt_AsLong(value);
 
    key = Py_BuildValue("s","format");
    if(key==NULL){
      return NULL;
    }
    value = PyDict_GetItem(requestor,key);
    Py_DECREF(key);
    if(!PyInt_Check(value)){
      PyErr_SetString(PyExc_TypeError, 
                     "illegal requestor format information");
      return NULL;
    }     
    reqFormat=PyInt_AsLong(value);
    
    key = Py_BuildValue("s","data");
    if(key==NULL){
      return NULL;
    }
    value = PyDict_GetItem(requestor,key);
    Py_DECREF(key);
    if(!PyUnicode_Check(value)){
      PyErr_SetString(PyExc_TypeError, 
                     "illegal requestor data");
      return NULL;
    };     
    TPtrC valuePtr((TUint16*) PyUnicode_AsUnicode(value), PyUnicode_GetSize(value));
    
    CRequestor* req=NULL;
    TRAP(err, req = CRequestor::NewL(reqType, reqFormat, valuePtr));
    if(KErrNone!=err){
      return SPyErr_SetFromSymbianOSErr(err);
    } 
    err=self->requestorStack->Append(req); 
    if(KErrNone!=err){
      delete req;
      return SPyErr_SetFromSymbianOSErr(err);
    }
  }
  
  err = self->positioner->SetRequestor(*(self->requestorStack));
  if(KErrNone!=err){
    return SPyErr_SetFromSymbianOSErr(err);
  }
    
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Return position infomation.
 */
extern "C" PyObject *
Positioner_position(Positioner_object *self, PyObject* args)
{ 
  TInt flags=0;
  PyObject* c;
  PY_LONG_LONG tv;
  TInt partial=0;
  TInt err=KErrNone;
  if(!self->isSubSessionOpen){
    err=opensubsession(self);
    if(err!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(err);
    }
    if(self->requestorStack){
      err=self->positioner->SetRequestor(*(self->requestorStack));
      if(err!=KErrNone){
        return SPyErr_SetFromSymbianOSErr(err);
      }
    }
  }
    
  if (!PyArg_ParseTuple(args, "|iOLi", &flags, &c, &tv, &partial)){ 
    return NULL;
  }

  TTimeIntervalMicroSeconds timeInterval = lltoTIMS(tv);

  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  Py_XINCREF(c);
  
  self->myCallBack.iCb = c;
  self->callBackSet = ETrue;

  // ask for position updates
  TRequestStatus status; 
  
  Py_BEGIN_ALLOW_THREADS
  TRAP(err,{
    self->dataFetcher=CPosDataFetcher::NewL(*(self->positioner));
    self->dataFetcher->CreatePositionType(flags);
    self->dataFetcher->SetCallBack(self->myCallBack);
    self->dataFetcher->FetchData(status, timeInterval, partial);
  });
  Py_END_ALLOW_THREADS
  
  Py_INCREF(Py_None);
  return Py_None;            
}
/*
 *  Returns Last Position information
 */
extern "C" PyObject*
Positioner_last_position(Positioner_object* self, PyObject* /*args*/)
{
  TInt err=KErrNone;
  if(!self->isSubSessionOpen){
    err=opensubsession(self);
    if(err!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(err);
    }
    if(self->requestorStack){
      err=self->positioner->SetRequestor(*(self->requestorStack));
      if(err!=KErrNone){
        return SPyErr_SetFromSymbianOSErr(err);
      }
    }
  }
  TRequestStatus status;
  TPositionInfo posInfo;
  self->positioner->GetLastKnownPosition(posInfo, status);
  User::WaitForRequest(status);
  if(status.Int()!=KErrNone)
    return SPyErr_SetFromSymbianOSErr(status.Int());
  TPosition position;
  posInfo.GetPosition(position);
  // convert TTime to TReal
  _LIT(KEpoch, "19700000:");
  TTime epochTime;
  epochTime.Set(KEpoch);
  TLocale locale;
  TReal time;
  #ifndef EKA2
    time=((position.Time().Int64().GetTReal()-(epochTime.Int64().GetTReal()))/(1000.0*1000.0))-
         TInt64(locale.UniversalTimeOffset().Int()).GetTReal();
  #else
    time=((TReal64(position.Time().Int64())-TReal64(epochTime.Int64()))/(1000.0*1000.0))-
         TInt64(TReal64(locale.UniversalTimeOffset().Int()));
  #endif

  return Py_BuildValue("{s:d,s:d,s:d,s:d,s:d,s:d}",
                       "latitude", position.Latitude(),
                       "longitude", position.Longitude(),
                       "altitude", position.Altitude(),
                       "vertical_accuracy", position.VerticalAccuracy(),
                       "horizontal_accuracy", position.HorizontalAccuracy(),
                       "time",time); 
}

/*
 * Stop position position infomation feed.
 */
extern "C" PyObject *
Positioner_stop_position(Positioner_object *self, PyObject* args)
{ 
  if (self->callBackSet){
    Py_BEGIN_ALLOW_THREADS
    self->dataFetcher->Cancel();
    Py_END_ALLOW_THREADS
  
    delete self->dataFetcher;
    self->dataFetcher = NULL;
  
    Py_XDECREF(self->myCallBack.iCb);
    self->callBackSet = EFalse;  
  }
  if (self->positioner){
    self->positioner->Close();
  }

  self->isSubSessionOpen = EFalse;
  
  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Deletes the requestors in the requestor stack and the stack itself.
 */
void deleteRequestorStack(Positioner_object *positioner)
{
  if(positioner->requestorStack){
    TInt count=positioner->requestorStack->Count();
    for(int i=0;i<count;i++){
       CRequestor* req = (*(positioner->requestorStack))[i];
       delete req;
    }
    positioner->requestorStack->Reset();
    delete positioner->requestorStack;
    positioner->requestorStack=NULL;
  }
}

/*
 * Open Sub-Session
 */
TInt opensubsession(Positioner_object *self)
{
  TInt err=KErrNone;
  TRAP(err,{
    if(self->moduleid==TPositionModuleId::Null()){
      // use default module.
      User::LeaveIfError(self->positioner->Open(*(self->positionServer->posServ)));
    }
    else{
      User::LeaveIfError(self->positioner->Open(*(self->positionServer->posServ),self->moduleid));
    }
  });
  if(err==KErrNone){
    self->isSubSessionOpen=ETrue;
  }
  return err;
}
/*
 * Deallocate the Positioner object.
 */
extern "C" {
  static void Positioner_dealloc(Positioner_object *positioner)
  {
    deleteRequestorStack(positioner);

    if(positioner->dataFetcher){
      delete positioner->dataFetcher;
      positioner->dataFetcher=NULL;
    }

    if (positioner->callBackSet) {
      Py_XDECREF(positioner->myCallBack.iCb);
    }

    //debugprint("positioner dealloc"); // ??? debug
    if (positioner->positioner) {
       positioner->positioner->Close();
       delete positioner->positioner;
       positioner->positioner = NULL;
    }   
    if(positioner->positionServer){
      Py_DECREF(positioner->positionServer);
      positioner->positionServer=NULL;
    }
    
    PyObject_Del(positioner);
  }
}

  const static PyMethodDef PositionServer_methods[] = {
    {"positioner", (PyCFunction)PositionServer_positioner, METH_VARARGS | METH_KEYWORDS, NULL},
    {"default_module", (PyCFunction)PositionServer_default_module, METH_NOARGS, NULL},
    {"modules", (PyCFunction)PositionServer_modules, METH_NOARGS, NULL},
    {"module_info", (PyCFunction)PositionServer_module_info, METH_VARARGS, NULL},
    {NULL, NULL, 0 , NULL} 
  };
  
  const static PyMethodDef Positioner_methods[] = {
    {"set_requestors", (PyCFunction)Positioner_set_requestors, METH_VARARGS, NULL},
    {"position", (PyCFunction)Positioner_position, METH_VARARGS, NULL},
    {"stop_position", (PyCFunction)Positioner_stop_position, METH_NOARGS, NULL},    
    {"last_position", (PyCFunction)Positioner_last_position, METH_NOARGS, NULL},
    {NULL, NULL, 0 , NULL} 
  };
  
  static PyObject *
  PositionServer_getattr(PositionServer_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)PositionServer_methods, (PyObject *)op, name);
  }

  static PyObject *
  Positioner_getattr(Positioner_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)Positioner_methods, (PyObject *)op, name);
  }
  
  static const PyTypeObject c_PositionServer_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "locationacq.PositionServer",             /*tp_name*/
    sizeof(PositionServer_object),            /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)PositionServer_dealloc,       /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)PositionServer_getattr,      /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
  };

  static const PyTypeObject c_Positioner_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "locationacq.Positioner",                 /*tp_name*/
    sizeof(Positioner_object),                /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)Positioner_dealloc,           /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)Positioner_getattr,          /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
  };


extern "C" {
  static const PyMethodDef locationacq_methods[] = {
    {"position_server", (PyCFunction)new_PositionServer_object, METH_NOARGS, NULL},
    {NULL, NULL}           /* sentinel */
  };

  DL_EXPORT(void) initlocationacq(void)
  {
    PyTypeObject* position_server_type = PyObject_New(PyTypeObject, &PyType_Type);
    *position_server_type = c_PositionServer_type;
    position_server_type->ob_type = &PyType_Type;
    SPyAddGlobalString("PositionServerType", (PyObject*)position_server_type);    
    
    PyTypeObject* positioner_type = PyObject_New(PyTypeObject, &PyType_Type);
    *positioner_type = c_Positioner_type;
    positioner_type->ob_type = &PyType_Type;
    SPyAddGlobalString("PositionerType", (PyObject*)positioner_type);  
    
    PyObject *m, *d;
    m = Py_InitModule("_locationacq", (PyMethodDef*)locationacq_methods);
    d = PyModule_GetDict(m);

    // info flags
    PyDict_SetItemString(d,"info_course", PyInt_FromLong(COURSE_INFO));
    PyDict_SetItemString(d,"info_satellites", PyInt_FromLong(SATELLITE_INFO));
        
    // requestor types
    PyDict_SetItemString(d,"req_type_service", PyInt_FromLong(CRequestorBase::ERequestorService));
    PyDict_SetItemString(d,"req_type_contact", PyInt_FromLong(CRequestorBase::ERequestorContact));
    
    // requestor formats
    PyDict_SetItemString(d,"req_format_app", PyInt_FromLong(CRequestorBase::EFormatApplication));
    PyDict_SetItemString(d,"req_format_tel", PyInt_FromLong(CRequestorBase::EFormatTelephone));
    PyDict_SetItemString(d,"req_format_url", PyInt_FromLong(CRequestorBase::EFormatUrl));
    PyDict_SetItemString(d,"req_format_mail", PyInt_FromLong(CRequestorBase::EFormatMail));
    
    // position module capabilities    
    PyDict_SetItemString(d,"cap_none", PyInt_FromLong(TPositionModuleInfo::ECapabilityNone));
    PyDict_SetItemString(d,"cap_horizontal", PyInt_FromLong(TPositionModuleInfo::ECapabilityHorizontal));
    PyDict_SetItemString(d,"cap_vertical", PyInt_FromLong(TPositionModuleInfo::ECapabilityVertical));
    PyDict_SetItemString(d,"cap_speed", PyInt_FromLong(TPositionModuleInfo::ECapabilitySpeed));
    PyDict_SetItemString(d,"cap_direction", PyInt_FromLong(TPositionModuleInfo::ECapabilityDirection));
    PyDict_SetItemString(d,"cap_satellite", PyInt_FromLong(TPositionModuleInfo::ECapabilitySatellite));
    PyDict_SetItemString(d,"cap_compass", PyInt_FromLong(TPositionModuleInfo::ECapabilityCompass));
    PyDict_SetItemString(d,"cap_nmea", PyInt_FromLong(TPositionModuleInfo::ECapabilityNmea));
    PyDict_SetItemString(d,"cap_address", PyInt_FromLong(TPositionModuleInfo::ECapabilityAddress));
    PyDict_SetItemString(d,"cap_building", PyInt_FromLong(TPositionModuleInfo::ECapabilityBuilding));
    PyDict_SetItemString(d,"cap_media", PyInt_FromLong(TPositionModuleInfo::ECapabilityMedia));

    // device location    
    PyDict_SetItemString(d,"dev_loc_unknown", PyInt_FromLong(TPositionModuleInfo::EDeviceUnknown));
    PyDict_SetItemString(d,"dev_loc_internal", PyInt_FromLong(TPositionModuleInfo::EDeviceInternal));
    PyDict_SetItemString(d,"dev_loc_external", PyInt_FromLong(TPositionModuleInfo::EDeviceExternal));
 
    // technology type
    PyDict_SetItemString(d,"technology_unknown", PyInt_FromLong(TPositionModuleInfo::ETechnologyUnknown)); 
    PyDict_SetItemString(d,"technology_terminal", PyInt_FromLong(TPositionModuleInfo::ETechnologyTerminal));
    PyDict_SetItemString(d,"technology_network", PyInt_FromLong(TPositionModuleInfo::ETechnologyNetwork));
    PyDict_SetItemString(d,"technology_assisted", PyInt_FromLong(TPositionModuleInfo::ETechnologyAssisted));

    // power consumption
    PyDict_SetItemString(d,"power_unknown", PyInt_FromLong(TPositionQuality::EPowerUnknown)); 
    PyDict_SetItemString(d,"power_zero", PyInt_FromLong(TPositionQuality::EPowerZero)); 
    PyDict_SetItemString(d,"power_low", PyInt_FromLong(TPositionQuality::EPowerLow)); 
    PyDict_SetItemString(d,"power_medium", PyInt_FromLong(TPositionQuality::EPowerMedium)); 
    PyDict_SetItemString(d,"power_high", PyInt_FromLong(TPositionQuality::EPowerHigh)); 
    
    // cost indicator
    PyDict_SetItemString(d,"cost_unknown", PyInt_FromLong(TPositionQuality::ECostUnknown)); 
    PyDict_SetItemString(d,"cost_zero", PyInt_FromLong(TPositionQuality::ECostZero)); 
    PyDict_SetItemString(d,"cost_possible", PyInt_FromLong(TPositionQuality::ECostPossible)); 
    PyDict_SetItemString(d,"cost_charge", PyInt_FromLong(TPositionQuality::ECostCharge)); 
       
    // device status
    PyDict_SetItemString(d,"status_unknown", PyInt_FromLong(TPositionModuleStatus::EDeviceUnknown)); 
    PyDict_SetItemString(d,"status_error", PyInt_FromLong(TPositionModuleStatus::EDeviceError)); 
    PyDict_SetItemString(d,"status_disabled", PyInt_FromLong(TPositionModuleStatus::EDeviceDisabled)); 
    PyDict_SetItemString(d,"status_inactive", PyInt_FromLong(TPositionModuleStatus::EDeviceInactive)); 
    PyDict_SetItemString(d,"status_initializing", PyInt_FromLong(TPositionModuleStatus::EDeviceInitialising)); 
    PyDict_SetItemString(d,"status_standby", PyInt_FromLong(TPositionModuleStatus::EDeviceStandBy)); 
    PyDict_SetItemString(d,"status_ready", PyInt_FromLong(TPositionModuleStatus::EDeviceReady)); 
    PyDict_SetItemString(d,"status_active", PyInt_FromLong(TPositionModuleStatus::EDeviceActive)); 
   
    // data quality status
    PyDict_SetItemString(d,"data_quality_unknown", PyInt_FromLong(TPositionModuleStatus::EDataQualityUnknown)); 
    PyDict_SetItemString(d,"data_quality_loss", PyInt_FromLong(TPositionModuleStatus::EDataQualityLoss)); 
    PyDict_SetItemString(d,"data_quality_partial", PyInt_FromLong(TPositionModuleStatus::EDataQualityPartial)); 
    PyDict_SetItemString(d,"data_quality_normal", PyInt_FromLong(TPositionModuleStatus::EDataQualityNormal)); 

    // field id:s for HPositionGenericInfo
    PyDict_SetItemString(d,"field_none", PyInt_FromLong(EPositionFieldNone)); 
    PyDict_SetItemString(d,"field_comment", PyInt_FromLong(EPositionFieldComment)); 
    PyDict_SetItemString(d,"field_speed_capabilities_begin", PyInt_FromLong(EPositionFieldSpeedCapabilitiesBegin)); 
    PyDict_SetItemString(d,"field_horizontal_speed", PyInt_FromLong(EPositionFieldHorizontalSpeed)); 
    PyDict_SetItemString(d,"field_horizontal_speed_error", PyInt_FromLong(EPositionFieldHorizontalSpeedError)); 
    PyDict_SetItemString(d,"field_vertical_speed", PyInt_FromLong(EPositionFieldVerticalSpeed)); 
    PyDict_SetItemString(d,"field_vertical_speed_error", PyInt_FromLong(EPositionFieldVerticalSpeedError)); 
    PyDict_SetItemString(d,"field_direction_capabilities_begin", PyInt_FromLong(EPositionFieldDirectionCapabilitiesBegin)); 
    PyDict_SetItemString(d,"field_true_course", PyInt_FromLong(EPositionFieldTrueCourse)); 
    PyDict_SetItemString(d,"field_true_course_error", PyInt_FromLong(EPositionFieldTrueCourseError)); 
    PyDict_SetItemString(d,"field_magnetic_course", PyInt_FromLong(EPositionFieldMagneticCourse)); 
   // PyDict_SetItemString(d,"field_magnetic_course_error", PyInt_FromLong(EPositionFieldMagneticCourseError)); 
    PyDict_SetItemString(d,"field_compass_capabilities_begin", PyInt_FromLong(EPositionFieldCompassCapabilitiesBegin)); 
    PyDict_SetItemString(d,"field_heading", PyInt_FromLong(EPositionFieldHeading)); 
    PyDict_SetItemString(d,"field_heading_error", PyInt_FromLong(EPositionFieldHeadingError)); 
   // PyDict_SetItemString(d,"field_magnetic_heading", PyInt_FromLong(EPositionFieldMagneticHeading)); 
   // PyDict_SetItemString(d,"field_magnetic_heading_error", PyInt_FromLong(EPositionFieldMagneticHeadingError)); 
    PyDict_SetItemString(d,"field_address_capabilities_begin", PyInt_FromLong(EPositionFieldAddressCapabilitiesBegin)); 
    PyDict_SetItemString(d,"field_country", PyInt_FromLong(EPositionFieldCountry)); 
    PyDict_SetItemString(d,"field_country_code", PyInt_FromLong(EPositionFieldCountryCode)); 
    PyDict_SetItemString(d,"field_state", PyInt_FromLong(EPositionFieldState)); 
    PyDict_SetItemString(d,"field_city", PyInt_FromLong(EPositionFieldCity)); 
    PyDict_SetItemString(d,"field_district", PyInt_FromLong(EPositionFieldDistrict)); 
    PyDict_SetItemString(d,"field_street", PyInt_FromLong(EPositionFieldStreet)); 
    PyDict_SetItemString(d,"field_street_extension", PyInt_FromLong(EPositionFieldStreetExtension)); 
    PyDict_SetItemString(d,"field_location_name", PyInt_FromLong(EPositionFieldLocationName)); 
    PyDict_SetItemString(d,"field_postal_code", PyInt_FromLong(EPositionFieldPostalCode)); 
    PyDict_SetItemString(d,"field_locality", PyInt_FromLong(EPositionFieldLocality));
    PyDict_SetItemString(d,"field_crossing1", PyInt_FromLong(EPositionFieldCrossing1));
    PyDict_SetItemString(d,"field_crossing2", PyInt_FromLong(EPositionFieldCrossing2));
    PyDict_SetItemString(d,"field_building_capabilities_begin", PyInt_FromLong(EPositionFieldBuildingCapabilitiesBegin));
    PyDict_SetItemString(d,"field_building_name", PyInt_FromLong(EPositionFieldBuildingName));
    PyDict_SetItemString(d,"field_building_floor", PyInt_FromLong(EPositionFieldBuildingFloor));
    PyDict_SetItemString(d,"field_building_room", PyInt_FromLong(EPositionFieldBuildingRoom));
    PyDict_SetItemString(d,"field_building_zone", PyInt_FromLong(EPositionFieldBuildingZone));
    PyDict_SetItemString(d,"field_building_telephone", PyInt_FromLong(EPositionFieldBuildingTelephone));
    PyDict_SetItemString(d,"field_nmea_capabilities_begin", PyInt_FromLong(EPositionFieldNMEACapabilitiesBegin));
    PyDict_SetItemString(d,"field_nmea_sentences", PyInt_FromLong(EPositionFieldNMEASentences));
    PyDict_SetItemString(d,"field_nmea_sentences_start", PyInt_FromLong(EPositionFieldNMEASentencesStart));
    PyDict_SetItemString(d,"field_satellite_capabilities_begin", PyInt_FromLong(EPositionFieldSatelliteCapabilitiesBegin));
    PyDict_SetItemString(d,"field_satellite_num_in_view", PyInt_FromLong(EPositionFieldSatelliteNumInView));
    PyDict_SetItemString(d,"field_satellite_num_used", PyInt_FromLong(EPositionFieldSatelliteNumUsed));
    PyDict_SetItemString(d,"field_satellite_time", PyInt_FromLong(EPositionFieldSatelliteTime));
    PyDict_SetItemString(d,"field_satellite_horizontal_dop", PyInt_FromLong(EPositionFieldSatelliteHorizontalDoP));
    PyDict_SetItemString(d,"field_satellite_vertical_dop", PyInt_FromLong(EPositionFieldSatelliteVerticalDoP));
    PyDict_SetItemString(d,"field_satellite_time_dop", PyInt_FromLong(EPositionFieldSatelliteTimeDoP));
    PyDict_SetItemString(d,"field_satellite_position_dop", PyInt_FromLong(EPositionFieldSatellitePositionDoP));
    PyDict_SetItemString(d,"field_satellite_sea_level_altitude", PyInt_FromLong(EPositionFieldSatelliteSeaLevelAltitude));
    PyDict_SetItemString(d,"field_satellite_geoidal_separation", PyInt_FromLong(EPositionFieldSatelliteGeoidalSeparation));
    PyDict_SetItemString(d,"field_media_capabilities_begin", PyInt_FromLong(EPositionFieldMediaCapabilitiesBegin));
    PyDict_SetItemString(d,"field_media_links", PyInt_FromLong(EPositionFieldMediaLinks));
    PyDict_SetItemString(d,"field_media_links_start", PyInt_FromLong(EPositionFieldMediaLinksStart));
    //PyDict_SetItemString(d,"field_media_links_end", PyInt_FromLong(EPositionFieldMediaLinksEnd)); // ???
    PyDict_SetItemString(d,"field_proprietary_fields_begin", PyInt_FromLong(EPositionFieldProprietaryFieldsBegin));
    PyDict_SetItemString(d,"field_id_last", PyInt_FromLong(EPositionFieldIdLast));
   
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}

#endif
