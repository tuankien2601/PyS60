/**
 * ====================================================================
 * recordermodule.cpp
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
 *
 * Python API to recording and playing, based on the sound example
 * app in Series 60 SDK.
 *
 * Implements currently following Python type:
 * 
 * Player
 *
 *    play(times, time_between, callback)
 *      times, how many times the sound file is played, time_between
 *      in microseconds prior playing again. callback gets called with
 *      previous state, current state and error code (0 = no error).
 *  
 *    stop()
 *    
 *    record()
 *      only ".wav"-files can be recorded.
 *    
 *    open_file(filename)
 *      creates the file if it does not exist already.
 *    
 *    close_file()
 *    
 *    int state()
 *      the possible states are in the module dictionary.
 *
 *    bind(callable)
 *      callable called with (previousState, currentState, errorCode) when
 *      state is changed.
 *    
 *    int max_volume()
 *      the maximum volume supported by the device.
 *
 *    set_volume(volume)
 *      set the volume (integer).
 *
 *    int current_volume()
 *      get the current volume set.
 *
 *    int duration()
 *      the duration of the file opened. 
 *
 *    set_position(int)
 *      set the position in microseconds, affects when play() is called 
 *      next time.
 *
 *    int current_position()
 *      get the current position in microseconds.
 *
 * Calling record() cannot happen after calling play(), open_file() must
 * preceed record().
 *
 * Playing file and not stopping the playing prior exiting the Python script
 * will leave the sound playing on, stop() needs to be called explicitly.
 *
 * Recording always appends to the end of the opened file. You must delete 
 * the file explicitly if you want to start from the beginning.
 *
 * TODO add here also phone uplink recording and/or playing, see:
 *       iMdaAudioRecorderUtility->
 *       SetAudioDeviceMode( CMdaAudioRecorderUtility::ETelephonyNonMixed);
 *      and "Developer Library » Symbian OS Guide » C++ API guide » 
 *      Multimedia » Multi Media Framework - Client API"
 * TODO increasegradient() --> SetVolumeRamp()
 * ====================================================================
 */

#include "recorder.h"

//////////////TYPE METHODS

/*
 * Deallocate rec.
 */
extern "C" {

  static void rec_dealloc(REC_object *reco)
  {
    if (reco->recorder) {
      delete reco->recorder;
      reco->recorder = NULL;
    }
    if (reco->callBackSet) {
      //XXX is this ok:
      Py_XDECREF(reco->myCallBack.iCb);
    }
    PyObject_Del(reco);
  }

}

/*
 * Allocate rec.
 */
extern "C" PyObject *
new_rec_object(PyObject* /*self*/, PyObject /**args*/)
{
  REC_object *reco = PyObject_New(REC_object, REC_type);
  if (reco == NULL)
    return PyErr_NoMemory();

  TRAPD(error, reco->recorder = CRecorderAdapter::NewL());
  if (reco->recorder == NULL) {
    PyObject_Del(reco);
    return PyErr_NoMemory();
  }

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  reco->callBackSet = EFalse;

  return (PyObject*) reco;
}

/*
 * Start recording to an opened file.
 */
extern "C" PyObject *
rec_record(REC_object* self, PyObject /**args*/)
{
  TInt error = KErrNone;
  
  Py_BEGIN_ALLOW_THREADS
  TRAP(error, self->recorder->RecordL());
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }  

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Stop recording.
 */
extern "C" PyObject *
rec_stop(REC_object* self, PyObject /**args*/)
{
  Py_BEGIN_ALLOW_THREADS
  self->recorder->Stop();
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Play an opened file.
 */
extern "C" PyObject *
rec_play(REC_object* self, PyObject *args)
{
  TInt error = KErrNone;
  TInt times;
  PY_LONG_LONG tv;
  if (!PyArg_ParseTuple(args, "iL", &times, &tv))
    return NULL;
  
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
  TTimeIntervalMicroSeconds timeInterval = hi_and_lo;
#else
  unsigned int tv_low, tv_high; 
  tv_low=*(unsigned int *)(&tv); 
  tv_high=*((unsigned int *)(&tv)+1);
  TTimeIntervalMicroSeconds timeInterval = TInt64(tv_high, tv_low);
#endif

  Py_BEGIN_ALLOW_THREADS
  TRAP(error, self->recorder->PlayL(times, timeInterval));
  Py_END_ALLOW_THREADS  
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }    

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Open descriptor.
 */
extern "C" PyObject *
rec_say(REC_object* self, PyObject *args)
{
  TInt error = KErrNone;
  char* text;
  int text_l;
  HBufC8* text_play = NULL;
  
  if (!PyArg_ParseTuple(args, "s#", &text, &text_l))
    return NULL;

  TPtrC8 p_text((TUint8*)text, text_l);

  TRAP(error, text_play = p_text.AllocL());
 
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_BEGIN_ALLOW_THREADS
  TRAP(error, self->recorder->PlayDescriptorL(text_play));
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Open a file for playing or recording.
 */
extern "C" PyObject *
rec_open_file(REC_object* self, PyObject *args)
{
  //Parse the passed filename:
  PyObject* filename = NULL;

  if (!PyArg_ParseTuple(args, "U", &filename))  
    return NULL;
  
  if ((filename && PyUnicode_GetSize(filename) > KMaxFileName)) {
    PyErr_SetString(PyExc_NameError, "filename too long");
    return NULL;
  }

  TPtrC soundfile((TUint16*) PyUnicode_AsUnicode(filename), 
                    PyUnicode_GetSize(filename));  
  
  TInt error = KErrNone;
  
  Py_BEGIN_ALLOW_THREADS
  TRAP(error, self->recorder->OpenFileL(soundfile));
  Py_END_ALLOW_THREADS
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }      

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Close the file opened.
 */
extern "C" PyObject *
rec_close_file(REC_object* self, PyObject /**args*/)
{
  Py_BEGIN_ALLOW_THREADS
  self->recorder->CloseFile();
  Py_END_ALLOW_THREADS

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Bind the call back for state modifications.
 */
extern "C" PyObject *
rec_bind(REC_object* self, PyObject *args)
{
  PyObject* c;
  
  if (!PyArg_ParseTuple(args, "O:set_callback", &c))
    return NULL;
  
  if (c == Py_None)
    c = NULL;
  else if (!PyCallable_Check(c)) {
    PyErr_SetString(PyExc_TypeError, "callable expected");
    return NULL;
  }

  Py_XINCREF(c);
  
  self->myCallBack.iCb = c;
  self->callBackSet = ETrue;

  self->recorder->SetCallBack(self->myCallBack);

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Query for the state
 */
extern "C" PyObject *
rec_state(REC_object* self, PyObject /**args*/)
{
  return Py_BuildValue("i", self->recorder->State());
}

/*
 * Query for the maximum volume
 */
extern "C" PyObject *
rec_max_volume(REC_object* self, PyObject /**args*/)
{
  return Py_BuildValue("i", self->recorder->GetMaxVolume());
}

/*
 * Set the volume
 */
extern "C" PyObject *
rec_set_volume(REC_object* self, PyObject *args)
{
  TInt volume;
  if (!PyArg_ParseTuple(args, "i", &volume))
    return NULL;
  
  self->recorder->SetVolume(volume);
  
  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Query for the current volume
 */
#if SERIES60_VERSION>12
extern "C" PyObject *
rec_current_volume(REC_object* self, PyObject /**args*/)
{
  TInt volume = 0;
  TInt error = KErrNone;
  
  error = self->recorder->GetCurrentVolume(volume);
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }      

  return Py_BuildValue("i", volume);
}
#endif /* SERIES60_VERSION */

/*
 * The duration of the opened file.
 */
extern "C" PyObject *
rec_duration(REC_object* self, PyObject /**args*/)
{
  TTimeIntervalMicroSeconds timeInterval = TTimeIntervalMicroSeconds();

  self->recorder->Duration(timeInterval);
  
  return Py_BuildValue("L", timeInterval.Int64());
}

/*
 * Set the position for the playhead.
 */
extern "C" PyObject *
rec_set_position(REC_object* self, PyObject *args)
{
  PY_LONG_LONG tv;
  if (!PyArg_ParseTuple(args, "L", &tv))
    return NULL;
  
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
  TTimeIntervalMicroSeconds timeInterval = hi_and_lo;
#else
  unsigned int tv_low, tv_high; 
  tv_low=*(unsigned int *)(&tv); 
  tv_high=*((unsigned int *)(&tv)+1);
  TTimeIntervalMicroSeconds timeInterval = TInt64(tv_high, tv_low);
#endif

  self->recorder->SetPosition(timeInterval);

  Py_INCREF(Py_None);
  return Py_None;
}

/*
 * Get the current position of the playhead.
 */
extern "C" PyObject *
rec_current_position(REC_object* self, PyObject /**args*/)
{
  TTimeIntervalMicroSeconds timeInterval = TTimeIntervalMicroSeconds();

  self->recorder->GetCurrentPosition(timeInterval);

  return Py_BuildValue("L", timeInterval.Int64());
}

//////////////TYPE SET

extern "C" {

  const static PyMethodDef rec_methods[] = {
    {"record", (PyCFunction)rec_record, METH_NOARGS},
    {"stop", (PyCFunction)rec_stop, METH_NOARGS},
    {"play", (PyCFunction)rec_play, METH_VARARGS},
    {"say", (PyCFunction)rec_say, METH_VARARGS},
    {"open_file", (PyCFunction)rec_open_file, METH_VARARGS},
    {"close_file", (PyCFunction)rec_close_file, METH_NOARGS},
    {"bind", (PyCFunction)rec_bind, METH_VARARGS},
    {"state", (PyCFunction)rec_state, METH_NOARGS},
    {"max_volume", (PyCFunction)rec_max_volume, METH_NOARGS},
    {"set_volume", (PyCFunction)rec_set_volume, METH_VARARGS},
#if SERIES60_VERSION>12
    {"current_volume", (PyCFunction)rec_current_volume, METH_NOARGS},
#endif /* SERIES60_VERSION */
    {"duration", (PyCFunction)rec_duration, METH_NOARGS},
    {"set_position", (PyCFunction)rec_set_position, METH_VARARGS},
    {"current_position", (PyCFunction)rec_current_position, METH_NOARGS},
    {NULL, NULL}  
  };

  static PyObject *
  rec_getattr(REC_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)rec_methods, (PyObject *)op, name);
  }

  static const PyTypeObject c_rec_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_recorder.Player",                       /*tp_name*/
    sizeof(REC_object),                       /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)rec_dealloc,                  /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)rec_getattr,                 /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash*/
  };
} //extern C

//////////////INIT

extern "C" {

  static const PyMethodDef recorder_methods[] = {
    {"Player", (PyCFunction)new_rec_object, METH_NOARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initrecorder(void)
  {
    PyTypeObject* rec_type = PyObject_New(PyTypeObject, &PyType_Type);
    *rec_type = c_rec_type;
    rec_type->ob_type = &PyType_Type;

    SPyAddGlobalString("RECType", (PyObject*)rec_type);

    PyObject *m, *d;

    m = Py_InitModule("_recorder", (PyMethodDef*)recorder_methods);
    d = PyModule_GetDict(m);
    
    PyDict_SetItemString(d,"KMdaRepeatForever", PyInt_FromLong(KMdaRepeatForever));
    PyDict_SetItemString(d,"EOpen", PyInt_FromLong(CMdaAudioRecorderUtility::EOpen));
    PyDict_SetItemString(d,"EPlaying", PyInt_FromLong(CMdaAudioRecorderUtility::EPlaying));
    PyDict_SetItemString(d,"ENotReady", PyInt_FromLong(CMdaAudioRecorderUtility::ENotReady));
    PyDict_SetItemString(d,"ERecording", PyInt_FromLong(CMdaAudioRecorderUtility::ERecording));
  }
} /* extern "C" */

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif
