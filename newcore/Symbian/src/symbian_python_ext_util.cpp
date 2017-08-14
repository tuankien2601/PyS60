/* Copyright (c) 2005-2009 Nokia Corporation
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

/*
 *  symbian_python_ext_util.cpp
 *
 *  Utilities for Symbian OS specific Python extensions.
 */

#include "symbian_python_ext_util.h"

static PyDateTime_CAPI *PyDateTimeAPI;

extern "C" {

  PyObject* global_dict = NULL;

  DL_EXPORT(PyObject *)
  SPyErr_SymbianOSErrAsString(int err)
  {
    char* err_string;
    char buffer[32];

    switch(err) {
    case 0:
      err_string = "KErrNone";
      break;
    case (-1):
      err_string = "KErrNotFound";
      break;
    case (-2):
      err_string = "KErrGeneral";
      break;
    case (-3):
      err_string = "KErrCancel";
      break;
    case (-4):
      err_string = "KErrNoMemory";
      break;
    case (-5):
      err_string = "KErrNotSupported";
      break;
    case (-6):
      err_string = "KErrArgument";
      break;
    case (-7):
      err_string = "KErrTotalLossOfPrecision";
      break;
    case (-8):
      err_string = "KErrBadHandle";
      break;
    case (-9):
      err_string = "KErrOverflow";
      break;
    case (-10):
      err_string = "KErrUnderflow";
      break;
    case (-11):
      err_string = "KErrAlreadyExists";
      break;
    case (-12):
      err_string = "KErrPathNotFound";
      break;
    case (-13):
      err_string = "KErrDied";
      break;
    case (-14):
      err_string = "KErrInUse";
      break;
    case (-15):
      err_string = "KErrServerTerminated";
      break;
    case (-16):
      err_string = "KErrServerBusy";
      break;
    case (-17):
      err_string = "KErrCompletion";
      break;
    case (-18):
      err_string = "KErrNotReady";
      break;
    case (-19):
      err_string = "KErrUnknown";
      break;
    case (-20):
      err_string = "KErrCorrupt";
      break;
    case (-21):
      err_string = "KErrAccessDenied";
      break;
    case (-22):
      err_string = "KErrLocked";
      break;
    case (-23):
      err_string = "KErrWrite";
      break;
    case (-24):
      err_string = "KErrDisMounted";
      break;
    case (-25):
      err_string = "KErrEof";
      break;
    case (-26):
      err_string = "KErrDiskFull";
      break;
    case (-27):
      err_string = "KErrBadDriver";
      break;
    case (-28):
      err_string = "KErrBadName";
      break;
    case (-29):
      err_string = "KErrCommsLineFail";
      break;
    case (-30):
      err_string = "KErrCommsFrame";
      break;
    case (-31):
      err_string = "KErrCommsOverrun";
      break;
    case (-32):
      err_string = "KErrCommsParity";
      break;
    case (-33):
      err_string = "KErrTimedOut";
      break;
    case (-34):
      err_string = "KErrCouldNotConnect";
      break;
    case (-35):
      err_string = "KErrCouldNotDisconnect";
      break;
    case (-36):
      err_string = "KErrDisconnected";
      break;
    case (-37):
      err_string = "KErrBadLibraryEntryPoint";
      break;
    case (-38):
      err_string = "KErrBadDescriptor";
      break;
    case (-39):
      err_string = "KErrAbort";
      break;
    case (-40):
      err_string = "KErrTooBig";
      break;
    case (-41):
      err_string = "KErrDivideByZero";
      break;
    case (-42):
      err_string = "KErrBadPower";
      break;
    case (-43):
      err_string = "KErrDirFull";
      break;
    case (-44):
      err_string = "KErrHardwareNotAvailable";
      break;
    case (-45):
      err_string = "KErrSessionClosed";
      break;
    case (-46):
      err_string = "KErrPermissionDenied";
      break;
    default:
      sprintf(buffer, "Error %d", err);
      err_string = buffer;
      break;
    }
    return Py_BuildValue("s",err_string);
  }

  DL_EXPORT(PyObject *)
    SPyErr_SetFromSymbianOSErr(int err)
  {
    if (err == KErrPython)
      return NULL;
    PyObject *err_string=SPyErr_SymbianOSErrAsString(err);
    if (!err_string)
      return NULL;
    PyObject *v=Py_BuildValue("(iO)", err, err_string);
    if (v != NULL) {
      PyErr_SetObject(PyExc_SymbianError, v);
      Py_DECREF(v);
      Py_DECREF(err_string);
    }
    return NULL;
  }

  /* rely on the separate Python dynamic memory pool for cleanup */

  DL_EXPORT(int) SPyAddGlobal(PyObject *key, PyObject *value)
  {
    if (!global_dict && !(global_dict = PyDict_New()))
      return (-1);

    return PyDict_SetItem(global_dict, key, value);
  }

  DL_EXPORT(int) SPyAddGlobalString(char *key, PyObject *value)
  {
    if (!global_dict && !(global_dict = PyDict_New()))
      return (-1);

    return PyDict_SetItemString(global_dict, key, value);
  }

  DL_EXPORT(PyObject *) SPyGetGlobal(PyObject *key)
  {
    return ((global_dict) ?
            PyDict_GetItem(global_dict, key) : NULL);
  }

  DL_EXPORT(PyObject *) SPyGetGlobalString(char *key)
  {
    return ((global_dict) ?
            PyDict_GetItemString(global_dict, key) : NULL);
  }

  DL_EXPORT(void) SPyRemoveGlobal(PyObject *key)
  {
    if (global_dict)
      PyDict_DelItem(global_dict, key);
  }

  DL_EXPORT(void) SPyRemoveGlobalString(char *key)
  {
    if (global_dict)
      PyDict_DelItemString(global_dict, key);
  }

  DL_EXPORT(TReal) epoch_as_TReal()
  {
    _LIT(KAppuifwEpoch, "19700000:");
    TTime epoch;
    epoch.Set(KAppuifwEpoch);
#ifndef EKA2
    return epoch.Int64().GetTReal();
#else
    return TReal64(epoch.Int64());
#endif
  }


  DL_EXPORT(TReal) time_as_UTC_TReal(const TTime& aTime)
  {
    TLocale loc;
#ifndef EKA2
     return (((aTime.Int64().GetTReal()-epoch_as_TReal())/
            (1000.0*1000.0))-
            TInt64(loc.UniversalTimeOffset().Int()).GetTReal());
#else
    return ((TReal64(aTime.Int64())-epoch_as_TReal())/
            (1000.0*1000.0))-
            TInt64(TReal64(loc.UniversalTimeOffset().Int()));
#endif
}


  DL_EXPORT(void) pythonRealAsTTime(TReal timeValue, TTime& theTime)
  {
    TLocale loc;
#ifndef EKA2
    TInt64 timeInt((timeValue +
                   TInt64(loc.UniversalTimeOffset().Int()).GetTReal())
                   *(1000.0*1000.0)+epoch_as_TReal());
    theTime = timeInt;
#else
    TInt64 timeInt((TInt64)((timeValue +
            TReal64(TInt64(loc.UniversalTimeOffset().Int())))*(1000.0*1000.0) +
            epoch_as_TReal()));
    theTime = timeInt;
#endif
}

  TInt getMonth(TMonth aMonth)
  {
    switch(aMonth){
        case EJanuary:
            return 1;
        case EFebruary:
            return 2;
        case EMarch:
            return 3;
        case EApril:
            return 4;
        case EMay:
            return 5;
        case EJune:
            return 6;
        case EJuly:
            return 7;
        case EAugust:
            return 8;
        case ESeptember:
            return 9;
        case EOctober:
            return 10;
        case ENovember:
            return 11;
        case EDecember:
            return 12;
        default:
            return -1;
    }
  }

  DL_EXPORT(PyObject *)
  ttimeAsPythonFloat(const TTime& timeValue)
  {
    return Py_BuildValue("d",time_as_UTC_TReal(timeValue));
  }

  TInt64
  epoc_to_unix_time(TTime aTime)
  {
    TTime unixTime;
    TInt64 utcTime = aTime.Int64();
    if (unixTime.Set(KUnixEpoch) != KErrNone){
      return NULL;
    }

    return (utcTime - unixTime.Int64());
  }

  // Utility function to convert time from Symbian to Unix representation.
  // The Unix time is returned as a datetime object.
  // convert_to_local: Use this to convert aTime to local time before
  // creating datetime object, if it is not already in local time.
  // (default value is true)
  PyObject *
  epoch_to_datetime(TTime aTime, TBool convert_to_local)
  {
    PyDateTime_IMPORT;
    struct tm *timePtr;

    TInt64 unixTimeInuSecs = epoc_to_unix_time(aTime);
    TInt timeInSecs = unixTimeInuSecs / (1000 * 1000);
    TInt microSecs = unixTimeInuSecs % (1000 * 1000);

    if (convert_to_local){ // aTime is not in local time, convert to local time
        timePtr = localtime(&timeInSecs);
        if (timePtr == NULL){
            return NULL;
        }
        // The timePtr structure returns year as a count from 1900. To get actual
        // year add 1900 years to it. Also month is counted starting from 0. Thus
        // add 1 to get the exact month.
        return PyDateTime_FromDateAndTime(timePtr->tm_year + 1900,
                                      timePtr->tm_mon + 1,
                                      timePtr->tm_mday,
                                      timePtr->tm_hour,
                                      timePtr->tm_min,
                                      timePtr->tm_sec,
                                      microSecs);
    }

    // aTime is already in local time
    TDateTime dTime = aTime.DateTime();
    return PyDateTime_FromDateAndTime(dTime.Year(),
                                      getMonth(dTime.Month()),
                                      dTime.Day() + 1,
                                      dTime.Hour(),
                                      dTime.Minute(),
                                      dTime.Second(),
                                      dTime.MicroSecond());
  }

  // Utility function to convert time from Unix to Symbian representation.
  // The input to this function is a datetime object(in local time) and 
  // output is an instance of TTime (UTC time).

  TTime
  datetime_to_epoch(PyObject *aDateTime)
  {
    PyDateTime_IMPORT;
    TLocale loc;

    TDateTime timeVal(PyDateTime_GET_YEAR(aDateTime),
                      TMonth(PyDateTime_GET_MONTH(aDateTime) - 1),
                      PyDateTime_GET_DAY(aDateTime) - 1,
                      PyDateTime_DATE_GET_HOUR(aDateTime),
                      PyDateTime_DATE_GET_MINUTE(aDateTime),
                      PyDateTime_DATE_GET_SECOND(aDateTime),
                      PyDateTime_DATE_GET_MICROSECOND(aDateTime));
    TTime t_time(timeVal);

    // ToDo: Can the frequent connect be avoided?
    RTz tzServer;
    tzServer.Connect();
    tzServer.ConvertToUniversalTime(t_time);
    tzServer.Close();

    return t_time;
  }

}
