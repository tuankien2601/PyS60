/**
 * ====================================================================
 * logging.h
 * Copyright (c) 2006-2007 Nokia Corporation
 *
 * Python API to Series 60 Log Engine API
 *
 * Current version supports log event retrieval for event types 
 * defined in the SDK documentation. Supported log event types are:
 *
 * - KLogCallEventTypeUid
 * - KLogShortMessageEventTypeUid
 * - KLogDataEventTypeUid
 * - KLogFaxEventTypeUid
 * - KLogMailEventTypeUid
 * - KLogTaskSchedulerEventTypeUid
 *
 * Log events can have following direction/mode definitions as
 * defined by the CLogEvent class:
 *
 * - R_LOG_DIR_IN
 * - R_LOG_DIR_OUT
 * - R_LOG_DIR_MISSED
 * - R_LOG_DIR_FETCHED
 * - R_LOG_DIR_IN_ALT
 * - R_LOG_DIR_OUT_ALT
 *
 * Restrictions:
 *
 * - Extension makes no restrictions to possible combinations of the 
 *   log event types and directions.
 * - Fields of the log events are copied 1:1 except the data-field length 
 *   (which has no limit by the system) is reduced to 500 bytes
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
#include <e32base.h>
#include <e32std.h>
#include <e32def.h>
//
#include "container.h"
//
//
enum TLogTypes {
	ELogTypeCall = 0,
	ELogTypeSms,
	ELogTypeData,
	ELogTypeFax,
	ELogTypeEMail,
	ELogTypeScheduler
};
enum TLogDirection {
	ELogModeIn = 0,
	ELogModeOut,
	ELogModeMissed,
	ELogModeFetched,
	ELogModeInAlt,
	ELogModeOutAlt
};
//
//
TInt getLogDirection(TInt aDirection )
{
	//
	switch (aDirection) {
	//
	case ELogModeIn:
		return R_LOG_DIR_IN;
	//
	case ELogModeOut:
		return R_LOG_DIR_OUT;
	//
	case ELogModeMissed:
		return R_LOG_DIR_MISSED;
	//
	case ELogModeFetched:
		return R_LOG_DIR_FETCHED;
	//
	case ELogModeInAlt:
		return R_LOG_DIR_IN_ALT;
	//
	case ELogModeOutAlt:
		return R_LOG_DIR_OUT_ALT;

	//
	default:
		return -1;
	}
}
TUid getLogType(TInt aType )
{
		if (TUid::Uid(aType)==TUid::Uid(ELogTypeCall)) {
			return KLogCallEventTypeUid;
		}
		else if (TUid::Uid(aType)==TUid::Uid(ELogTypeSms)) {
			return KLogShortMessageEventTypeUid;
		}
		else if (TUid::Uid(aType)==TUid::Uid(ELogTypeData)) {
			return KLogDataEventTypeUid;
		}
		else if (TUid::Uid(aType)==TUid::Uid(ELogTypeFax)) {
			return KLogFaxEventTypeUid;
		}
		else if (TUid::Uid(aType)==TUid::Uid(ELogTypeEMail)) {
			return KLogMailEventTypeUid;
		}
		else if (TUid::Uid(aType)==TUid::Uid(ELogTypeScheduler)) {
			return KLogTaskSchedulerEventTypeUid;
		}
		else {
			return TUid::Uid(-1);
		}
}
/*
 * Time handling. (copied from appuifw/calendar).
 */
 /*
static TReal epoch_as_TReal()
{
  _LIT(KAppuifwEpoch, "19700000:");
  TTime epoch;
  epoch.Set(KAppuifwEpoch);
  return epoch.Int64().GetTReal();
}
static TReal time_as_UTC_TReal(const TTime& aTime)
{
  TLocale loc;
  return (((aTime.Int64().GetTReal()-epoch_as_TReal())/
          (1000.0*1000.0))-TInt64(loc.UniversalTimeOffset().Int()).GetTReal());
}
*/
//
//
//
//
//
//
//Library methods:
extern "C" PyObject* 
logs_list(PyObject* /*self*/, PyObject* args, PyObject* keys)
{
	//
	//
	const char* const keywords[] = {	
		(char *)"type",
		(char *)"mode",
		NULL
	};
	//
	//Get the command line arguments.
	TInt iType = ELogTypeCall;
	TInt iMode = ELogModeIn;
	//
	//
	if(!PyArg_ParseTupleAndKeywords(args, keys, "|ii", const_cast<char**>(keywords), &iType, &iMode)) {
		return NULL;
	}
	//
	//
	TInt iError = KErrNone;
	CLogContainer* lc = NULL;
	TRAP(iError, 
		{
		lc = CLogContainer::NewL(getLogDirection(iMode), getLogType(iType));
		//
		Py_BEGIN_ALLOW_THREADS
		lc->StartL();
		Py_END_ALLOW_THREADS
		}
	);
	if(iError!=KErrNone){
		//
		return SPyErr_SetFromSymbianOSErr(iError);
	}

	//
	// HANDLE RECEIVED LOGS
	//
	//Get event count
	TInt iEventCount = lc->Count();
	//List to hold the log events
	PyObject* eventList = NULL;
	//Init list for the events. Exit in case no memory.
	eventList = PyList_New(iEventCount);
	if(eventList == NULL) {
		if(lc) delete lc;
		return PyErr_NoMemory();
	}
	// Loop through the stored events.
	for(TInt i=0; i<iEventCount; i++) {
		//Get reference to stored event data.
		CEventData& e = lc->Get(i);
		
		const TDesC *number = e.GetNumber();
		const TDesC8 *data = e.GetData();
		
		if ((!number) || (!data))
		{
			return SPyErr_SetFromSymbianOSErr(KErrGeneral);
		}
		
		//Build now the return codes.
		PyObject* infoDict = Py_BuildValue(
								"{s:u#,s:u#,s:u#,s:u#,s:u#,s:u#,s:i,s:i,s:i,s:i,s:h,s:i,s:d,s:s#}",
								"number",				number->Ptr(), number->Length(),
								"name",					e.iName.Ptr(), e.iName.Length(),
								"description",	e.iDescription.Ptr(), e.iDescription.Length(),
								"direction",		e.iDirection.Ptr(), e.iDirection.Length(),
								"status",				e.iStatus.Ptr(), e.iStatus.Length(),
								"subject",			e.iSubject.Ptr(), e.iSubject.Length(),
								"id",						e.iId,
								"contact",			e.iContactId,
								"duration",			e.iDuration,
								"duration type",e.iDurationType,
								"flags",				e.iFlags,
								"link",					e.iLink,
								"time",					time_as_UTC_TReal(e.iTime),
								"data",					data->Ptr(), data->Length()
								);
								
		//Error handling...
		if(infoDict==NULL){
			if(lc) delete lc;
			Py_DECREF(eventList);
			return NULL;
		}
		if(PyList_SetItem(eventList, i, infoDict)<0){
			if(lc) delete lc;
			Py_DECREF(infoDict);
			Py_DECREF(eventList);
			return NULL;
		}
	}
	//
	if(lc) {
		delete lc;
	}
	//Return the list built from dictionaries containing the log event data.
	return eventList;
}


extern "C" 
{
	//
	//Method table for mapping library(.PYD) methods to C-methods.
	static const PyMethodDef logs_methods[] = {
		{"list", (PyCFunction)logs_list, METH_VARARGS|METH_KEYWORDS, NULL},
		{NULL, NULL}
	};
	//
	// Init routine for PYTHON.
	DL_EXPORT(void) initlogs(void)
	{
		PyObject* module = Py_InitModule("_logs", (PyMethodDef*)logs_methods);
		if (!module)
		{
			return;
		}
		//Define constants for wrapper/scripts.
    PyObject* d = PyModule_GetDict(module);
		//Types
		PyDict_SetItemString(d,"ELogTypeCall", PyInt_FromLong(ELogTypeCall));
		PyDict_SetItemString(d,"ELogTypeData", PyInt_FromLong(ELogTypeData));
		PyDict_SetItemString(d,"ELogTypeFax", PyInt_FromLong(ELogTypeFax));
		PyDict_SetItemString(d,"ELogTypeSms", PyInt_FromLong(ELogTypeSms));
		PyDict_SetItemString(d,"ELogTypeEMail", PyInt_FromLong(ELogTypeEMail));
		PyDict_SetItemString(d,"ELogTypeScheduler", PyInt_FromLong(ELogTypeScheduler));
		//Directions
		PyDict_SetItemString(d,"ELogModeIn", PyInt_FromLong(ELogModeIn));
		PyDict_SetItemString(d,"ELogModeOut", PyInt_FromLong(ELogModeOut));
		PyDict_SetItemString(d,"ELogModeMissed", PyInt_FromLong(ELogModeMissed));
		PyDict_SetItemString(d,"ELogModeFetched", PyInt_FromLong(ELogModeFetched));
		PyDict_SetItemString(d,"ELogModeInAlt", PyInt_FromLong(ELogModeInAlt));
		PyDict_SetItemString(d,"ELogModeOutAlt", PyInt_FromLong(ELogModeOutAlt));
	}
}


#ifndef EKA2
//
// For Symbian or rather S60 3rd edition sdk
GLDEF_C TInt E32Dll(TDllReason)
{
  return KErrNone;
}
#endif
