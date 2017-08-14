/*
 * ====================================================================
 *  calendarmodule.h
 *
 *  Python API to Series 60 Calendar database.
 *
 * Copyright (c) 2006-2007 Nokia Corporation
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

#include <calsession.h> 
#include <calentryview.h> 
#include <calinstanceview.h> 
#include <calinstance.h> 
#include <calentry.h>
#include <caliterator.h>
#include <calprogresscallback.h>
#include <e32base.h>
#include <calalarm.h>
#include <s32mem.h>
#include <caldataexchange.h>
#include <CalDataFormat.h>
#include <e32std.h>
#include <calrrule.h> 
#include <calcommon.h>
#include <tz.h>

#define KNullEntryId 0

#define NOT_REPEATED 0
#define DAILY_REPEAT 1
#define WEEKLY_REPEAT 2
#define MONTHLY_BY_DATES_REPEAT 3
#define MONTHLY_BY_DAYS_REPEAT 4
#define YEARLY_BY_DATE_REPEAT 5
#define YEARLY_BY_DAY_REPEAT 6

#define DAYS_IN_WEEK 7
#define DAYS_IN_MONTH 31
#define WEEKS_IN_MONTH 5
#define MONTHS_IN_YEAR 12

#define EARLIEST_ALARM_DAY_INTERVAL  1001

// Keys of repeat dictionary
_LIT8(KRepeatType, "type");
_LIT8(KStartDate, "start");
_LIT8(KEndDate, "end");
_LIT8(KRepeatDays, "days");
_LIT8(KExceptions, "exceptions");
_LIT8(KInterval, "interval");

_LIT8(KWeek, "week");
_LIT8(KMonth, "month");
_LIT8(KDay, "day");

// Repeat types
_LIT8(KDaily, "daily");
_LIT8(KWeekly, "weekly");
_LIT8(KMonthlyByDates, "monthly_by_dates");
_LIT8(KMonthlyByDays, "monthly_by_days");
_LIT8(KYearlyByDate, "yearly_by_date");
_LIT8(KYearlyByDay, "yearly_by_day");
_LIT8(KRepeatNone, "no_repeat");

_LIT8(KUniqueId, "id");
_LIT8(KDateTime, "datetime");




NONSHARABLE_CLASS(CCalEntryViewAdapter) : public CBase, public MCalProgressCallBack
{
 public:
  static CCalEntryViewAdapter* NewL(CCalSession& aSession)
  {
    CCalEntryViewAdapter* self = new (ELeave) CCalEntryViewAdapter(aSession);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
  };         
  void ConstructL()
  {
    iWait = new (ELeave) CActiveSchedulerWait;
  };
  CCalEntryViewAdapter(CCalSession& aSession):iWait(NULL),iSession(aSession),iError(KErrNone),iEntryView(NULL)
  {
  };
  virtual ~CCalEntryViewAdapter()
  {
    delete iWait;
    delete iEntryView;
  };
  TInt InitiateL()
  {
    iEntryView = CCalEntryView::NewL(iSession, *this);
    iWait->Start();
    return iError;
  };  
 private:
  void Progress(TInt /*aPercentageCompleted*/){
  };
  void Completed(TInt aError){
    iError = aError;
    iWait->AsyncStop(); 
  };
  TBool NotifyProgress(){
    return ETrue;
  }
  
  CActiveSchedulerWait* iWait; 
  CCalSession& iSession; 
  TInt iError;
 public:
  CCalEntryView* iEntryView; 
};


NONSHARABLE_CLASS(CCalInstanceViewAdapter) : public CBase, public MCalProgressCallBack
{
 public:
  static CCalInstanceViewAdapter* NewL(CCalSession& aSession)
  {
    CCalInstanceViewAdapter* self = new (ELeave) CCalInstanceViewAdapter(aSession);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
  };         
  void ConstructL()
  {
    iWait = new (ELeave) CActiveSchedulerWait;
  };
  CCalInstanceViewAdapter(CCalSession& aSession):iWait(NULL),iSession(aSession),iError(KErrNone),iInstanceView(NULL)
  {
  };
  virtual ~CCalInstanceViewAdapter()
  {
    delete iWait;
    delete iInstanceView;
  };
  TInt InitiateL()
  {
    iInstanceView = CCalInstanceView::NewL(iSession, *this);
    iWait->Start();
    return iError;
  };  
 private:
  void Progress(TInt /*aPercentageCompleted*/){
  };
  void Completed(TInt aError){
    iError = aError;
    iWait->AsyncStop(); 
  };
  TBool NotifyProgress(){
    return ETrue;
  }
  
  CActiveSchedulerWait* iWait; 
  CCalSession& iSession; 
  TInt iError;
 public:
  CCalInstanceView* iInstanceView; 
};



//////////////TYPE DEFINITION



#define CalendarDb_type ((PyTypeObject*)SPyGetGlobalString("CalendarDbType"))
struct CalendarDb_object {
  PyObject_VAR_HEAD
  CCalSession* session;
  CCalEntryViewAdapter* entryViewAdapter;  
  CCalInstanceViewAdapter* instanceViewAdapter;  
};

#define Entry_type ((PyTypeObject*)SPyGetGlobalString("EntryType"))
struct Entry_object {
  PyObject_VAR_HEAD
  CalendarDb_object* calendarDb;
  CCalEntry* entry;
};

#define EntryIterator_type ((PyTypeObject*)SPyGetGlobalString("EntryIteratorType"))
struct EntryIterator_object {
  PyObject_VAR_HEAD
  CalendarDb_object* calendarDb;
  RArray<TCalLocalUid>* ids;
  TInt index;
  TBool Initialized;
};



//////////////MACRO DEFINITION///////////////



#define GET_VALUE_FROM_DICT(str_key, dict) \
  key = Py_BuildValue("s", (&str_key)->Ptr());\
  if(NULL==key){ \
    return NULL; \
  } \
  value = PyDict_GetItem(dict, key); \
  Py_DECREF(key); \
  
#define GET_REP_START_AND_END \
  pythonRealAsTTime(startDate, startTime);\
  pythonRealAsTTime(endDate, endTime); \
  startTime=truncToDate(startTime); \
  endTime=truncToDate(endTime); \
  if(eternalRepeat!=EFalse){ \
    endTime=startTime; \
  } \
  if(startDate==0 || EFalse==Check_time_validity(startTime)){ \
    PyErr_SetString(PyExc_TypeError, "start date illegal or missing"); \
    return NULL; \
  } \
  if((eternalRepeat==EFalse && endDate==0) || EFalse==Check_time_validity(endTime)){ \
    PyErr_SetString(PyExc_TypeError, "end date illegal or missing"); \
    return NULL; \
  } \
  if(endTime<=startTime){ \
    PyErr_SetString(PyExc_TypeError, "end date must be greater than start date"); \
    return NULL; \
  } \
  endTime+=TTimeIntervalDays(1); \
  TRAP(error,{ \
    startCalTime.SetTimeUtcL(startTime); \
    endCalTime.SetTimeUtcL(endTime); \
  }); \
  if(error!=KErrNone){ \
    return SPyErr_SetFromSymbianOSErr(error); \
  } \
 
#define SET_REPEAT_DATES \
  rpt.SetDtStart(startCalTime); \
  if(eternalRepeat==EFalse){ \
    rpt.SetUntil(endCalTime); \
  } \
  rpt.SetInterval(interval); \
  
#define ADD_REPEAT \
  TRAP(error, { \
    self->entry->SetRRuleL(rpt); \
  }); \
  if(error!=KErrNone){ \
    return SPyErr_SetFromSymbianOSErr(error); \
  }; \
  if(exceptionArray){ \
    TRAP(error, { \
      self->entry->SetExceptionDatesL(*exceptionArray); \
    }); \
    if(error != KErrNone){ \
      return SPyErr_SetFromSymbianOSErr(error); \
    } \
  } \
  
#define ADD_ITEM_TO_REP_DICT(key, value) \
  if(!(key && value)){ \
    Py_XDECREF(key); \
    Py_XDECREF(value); \
    Py_DECREF(repeatDataDict); \
    return NULL; \
  } \
  err = PyDict_SetItem(repeatDataDict, key, value); \
  Py_DECREF(key); \
  Py_DECREF(value); \
  if(err){ \
    Py_DECREF(repeatDataDict); \
    return NULL; \
  } \
  
#define CHECK_DAYLIST_CREATION \
  if(dayList==NULL){ \
    days.Close(); \
    return PyErr_NoMemory(); \
  } \
  
#define SET_ITEM_TO_DAYLIST \
  if(dayNum==NULL){ \
    days.Close(); \
    Py_DECREF(dayList); \
    return NULL; \
  } \
  if(PyList_SetItem(dayList, i, dayNum)){ \
    days.Close(); \
    Py_DECREF(dayList); \
    return NULL; \
  } \

#define DELETE_INSTANCES \
  while(instanceArray.Count()>0){ \
    delete instanceArray[0]; \
    instanceArray.Remove(0); \
  } \
  instanceArray.Close(); \
  

    
//////////////METHODS///////////////
  


extern "C" PyObject *
new_Entry_object(CalendarDb_object* self, 
                 TCalLocalUid uniqueId, 
                 CCalEntry::TType entryType = CCalEntry::EAppt,
                 const TDesC8* globalUid = NULL);
                 
                 
