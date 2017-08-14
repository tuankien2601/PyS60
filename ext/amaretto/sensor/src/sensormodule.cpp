/**
 * ====================================================================
 * sensormodule.cpp
 *
 * Implements Python support for the S60 sensor API.
 *
 * - Apparently a sensor can only receive one "AddDataListener" call in
 *   its life time, if more then we get a panic this is why the Python
 *   wrapper creates a new sensor object for every 'connect' call
 * - Could not be compiled or tested in the emulator at all (.lib for
 *   the emulator is missing).
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
 * ====================================================================
 */

#include "Python.h"

#include "symbian_python_ext_util.h"

#ifndef __WINS__ /* Remove when .lib for WINSCW supplied */

#include <rrsensorapi.h>
#include <e32cmn.h>


NONSHARABLE_CLASS(CSensorDataListener):public CBase, public MRRSensorDataListener
{
 public:
  static CSensorDataListener* NewL(PyObject* aCallback)
  {
    CSensorDataListener* self = new (ELeave) CSensorDataListener(aCallback);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
  };
  void ConstructL()
  {
  };
  CSensorDataListener(PyObject* aCallback):iCallback(aCallback)
  {
    Py_XINCREF(iCallback);
  };
  virtual ~CSensorDataListener()
  {
    Py_XDECREF(iCallback);
  };
  void HandleDataEventL(TRRSensorInfo aSensor, TRRSensorEvent aEvent){
    if(iCallback){
      PyGILState_STATE gstate = PyGILState_Ensure();
      PyObject* parameters = Py_BuildValue("({s:i,s:i,s:i:s:i})", "sensor_id", aSensor.iSensorId,
                                                                "data_1", aEvent.iSensorData1,
                                                                "data_2", aEvent.iSensorData2,
                                                                "data_3", aEvent.iSensorData3);
      PyObject* tmp = PyEval_CallObject(iCallback, parameters);
      Py_XDECREF(tmp);
      Py_XDECREF(parameters);
      if (PyErr_Occurred()){
        PyErr_Print();
      }
      PyGILState_Release(gstate);
    }
  };

 private:
  PyObject* iCallback;
};

#define Sensor_type ((PyTypeObject*)SPyGetGlobalString("SensorType"))

struct Sensor_object {
  PyObject_VAR_HEAD
  CRRSensorApi* sensor;
  CSensorDataListener* listener;
};


/*
 * Returns the operating system version.
 */
extern "C" PyObject *
sensor_sensors(PyObject* /*self*/, PyObject* /*args*/)
{
  TInt error = KErrNone;
  PyObject* sensorInfoItems = NULL;
  RArray<TRRSensorInfo> sensorInfoArray;

  TRAP(error, CRRSensorApi::FindSensorsL(sensorInfoArray));
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  sensorInfoItems = PyDict_New();
  if (sensorInfoItems == NULL){
    sensorInfoArray.Close();
    return PyErr_NoMemory();
  }
  for(TInt i=0;i<sensorInfoArray.Count();i++){
    PyObject* sensorInfo = Py_BuildValue("{s:i,s:i}",  "category",
                                                       sensorInfoArray[i].iSensorCategory,
													   "id",
    												   sensorInfoArray[i].iSensorId);
    if(sensorInfo == NULL){
      sensorInfoArray.Close();
      return NULL;
    }
    PyObject* key = Py_BuildValue("u#", sensorInfoArray[i].iSensorName.Ptr(),
    									sensorInfoArray[i].iSensorName.Length());
    if(key == NULL){
      sensorInfoArray.Close();
      Py_DECREF(sensorInfo);
      return NULL;
    }
    error = PyDict_SetItem(sensorInfoItems, key, sensorInfo);
    Py_DECREF(key);
    Py_DECREF(sensorInfo);
    if(error){
      sensorInfoArray.Close();
      Py_DECREF(sensorInfoItems);
      return NULL;
    }
  }

  sensorInfoArray.Close();

  return sensorInfoItems;
}


/*
 * Create new sensor object.
 */
extern "C" PyObject *
new_Sensor_object(TInt id, TInt category)
{
  TInt error = KErrNone;
  CRRSensorApi* sensor = NULL;
  TRRSensorInfo sensorInfo;
  sensorInfo.iSensorId = id;
  sensorInfo.iSensorCategory = category;

  TRAP(error, {
    sensor = CRRSensorApi::NewL(sensorInfo);
  });
  if(error!=KErrNone){
    delete sensor;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Sensor_object* sensorObj =
    PyObject_New(Sensor_object, Sensor_type);
  if(sensorObj == NULL){
    delete sensor;
    return PyErr_NoMemory();
  }
  sensorObj->sensor = sensor;
  sensorObj->listener = NULL;
  return (PyObject*)sensorObj;
}


/*
 * Returns the specified sensor object.
 */
extern "C" PyObject *
sensor_sensor(PyObject* /*self*/, PyObject* args)
{
  TInt id = 0;
  TInt category = 0;

  if (!PyArg_ParseTuple(args, "ii", &id, &category)){
    return NULL;
  }

  return new_Sensor_object(id, category);
}


extern "C" {
  PyObject *Sensor_connect(Sensor_object *self, PyObject *args)
  {
  	PyObject *callback = NULL;
    CSensorDataListener* listener = NULL;

  	if (!PyArg_ParseTuple(args, "O", &callback))
  	{
		return NULL;
	}

	if (self->listener != NULL)
	{
		return Py_BuildValue("i", 0);
	}

    TRAPD(error, {
      listener = new (ELeave) CSensorDataListener(callback);
	  self->sensor->AddDataListener(listener);
    });
    if (error != KErrNone) {
      delete listener;
      return SPyErr_SetFromSymbianOSErr(error);
    }

	self->listener = listener;

	return Py_BuildValue("i", 1);
  }


  PyObject *Sensor_disconnect(Sensor_object *self, PyObject *args)
  {
	if (self->listener == NULL)
	{
		return Py_BuildValue("i", 0);
	}

	// XXX Can this really leave?
  TRAPD(error, {
	  self->sensor->RemoveDataListener();
	});
    if (error != KErrNone)
	{
      return SPyErr_SetFromSymbianOSErr(error);
	}

	delete self->listener;
	self->listener = NULL;

	return Py_BuildValue("i", 1);
  }
}


/*
 * Deallocate the sensor object.
 */
extern "C" {
  static void Sensor_dealloc(Sensor_object *self)
  {
    self->sensor->RemoveDataListener();
    if (self->listener != NULL)
    {
	    delete self->listener;
    	self->listener = NULL;
	}
    delete self->sensor;
    self->sensor = NULL;
    PyObject_Del(self);
  }
}


extern "C" {

    const static PyMethodDef Sensor_methods[] = {
      {"connect", (PyCFunction)Sensor_connect, METH_VARARGS, NULL},
      {"disconnect", (PyCFunction)Sensor_disconnect, METH_NOARGS, NULL},
      {NULL, NULL}
    };

    static PyObject *
    Sensor_getattr(Sensor_object *op, char *name)
    {
      return Py_FindMethod((PyMethodDef*)Sensor_methods, (PyObject *)op, name);
    }

    static const PyTypeObject c_Sensor_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_sensor.Sensor",                         /*tp_name*/
    sizeof(Sensor_object),                    /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)Sensor_dealloc,               /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)Sensor_getattr,              /*tp_getattr*/
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


  static const PyMethodDef sensor_methods[] = {
    {"sensors", (PyCFunction)sensor_sensors, METH_NOARGS, NULL},
    {"sensor", (PyCFunction)sensor_sensor, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initsensor(void)
  {
    PyTypeObject* sensor_type = PyObject_New(PyTypeObject, &PyType_Type);
    *sensor_type = c_Sensor_type;
    sensor_type->ob_type = &PyType_Type;
    SPyAddGlobalString("SensorType", (PyObject*)sensor_type);


    PyObject *m;
    m = Py_InitModule("_sensor", (PyMethodDef*)sensor_methods);
  }
} /* extern "C" */

#else /* __WINS__ */

extern "C" {
  DL_EXPORT(void) initsensor(void)
  {
    /* empty */
  }
} /* extern "C" */

#endif /* __WINS__ */
