/*
 * ====================================================================
 *  calendarmodule.h
 *
 *  Python API to Series 60 Calendar database.
 *
 * Copyright (c) 2006-2009 Nokia Corporation
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

  
#include "calendarmodule.h"
#include <e32const.h>


//////////////GENERAL FUNCTIONS///////////////

/*
 * Converts python time to UTC TTime value
 */
void pythonRealUtcAsTTime(TReal timeValue, TTime& theTime)
{
  TInt64 timeInt((timeValue*1000.0*1000.0)+epoch_as_TReal());
  theTime = timeInt;
}

/*
 * Returns date value that corresponds the given datetime value.
 */
TTime truncToDate(const TTime& theTime)
{
  TTime temp = theTime;
  
  // ??? check the summer to winter conversion and double local time.
  RTz tz;
  tz.Connect();
  tz.ConvertToLocalTime(temp);
  TDateTime aDateTime = temp.DateTime();
  aDateTime.SetHour(0); 
  aDateTime.SetMinute(0); 
  aDateTime.SetSecond(0); 
  temp = aDateTime;
  tz.Close();
  return temp;
}


/*
 * Checks if the time value is in the limits of native calendar API (and not null).
 */
TBool Check_time_validity(const TTime& theTime)
{
  if(theTime<=TCalTime::MinTime() ||
     TCalTime::MaxTime()<theTime){
    return EFalse; // Illegal time value.
  }else{
    return ETrue;
  }
}

/*
 * Give the next month of given month
 */
TMonth getNextMonth(TMonth aMonth){

switch(aMonth){
  case EJanuary:
    return EFebruary;
  case EFebruary:
    return EMarch;
  case EMarch:
    return EApril;
  case EApril:
    return EMay;
  case EMay:
    return EJune;
  case EJune:
    return EJuly;
  case EJuly:
    return EAugust;
  case EAugust:
    return ESeptember;
  case ESeptember:
    return EOctober;
  case EOctober:
    return ENovember;
  case ENovember:
    return EDecember;
  case EDecember:
    return EJanuary;
  }
  return aMonth;
}

/*
 * Checks if the value represents a weekday.
 */
TBool isWeekday(TInt value)
{
  TBool retVal = ETrue;
  switch(value){
    case EMonday:
    break;
    case ETuesday:
    break;
    case EWednesday:
    break; 
    case EThursday:
    break;
    case EFriday:
    break;
    case ESaturday:
    break;
    case ESunday:
    break;
    default:
      retVal = EFalse;
    break;
  }
  return retVal;
}


/*
 * Checks if the value represents a month.
 */
TBool isMonth(TInt value)
{
  TBool retVal = ETrue;
  switch(value){
    case EJanuary:
    break;
    case EFebruary:
    break;
    case EMarch:
    break;
    case EApril:
    break;
    case EMay:
    break;
    case EJune:
    break;
    case EJuly:
    break;
    case EAugust:
    break;
    case ESeptember:
    break;
    case EOctober:
    break;
    case ENovember:
    break;
    case EDecember:
    break;
    default:
      retVal = EFalse;
    break;
  }
  return retVal;
}


/*
 * Checks if the value represents a weeknumber (in month).
 */
TBool isWeekInMonth(TInt value)
{
  TBool retVal = ETrue;
  switch(value){
    case 0:
    break;
    case 1:
    break;
    case 2:
    break;
    case 3:
    break;
    case 4:
    break;
    default:
      retVal = EFalse;
    break;
  }
  return retVal;
}


/* Workaround for a platform bug that StartTimeL() raises exception
 * with return code -6 (KErrArgument) , when aCallTime is set to null time.
 * In case of the KErrArgument exception, aCallTime will be set to 
 * TCalTime::MinTime(). This workarround would cause problem if the 
 * exception KErrArgument is raised because of some other reason then previously 
 * mentioned platform bug.
 */ 
TInt GetStartTime(CCalEntry& aEntry,TTime& aStartTime)
{
  TInt error = KErrNone;
  TRAP(error, {
    aStartTime = aEntry.StartTimeL().TimeLocalL();
  });
  if(error == KErrArgument){
    aStartTime = TCalTime::MinTime();
    return (KErrNone);
  }
  return error;
}

TInt GetEndTime(CCalEntry& aEntry,TTime& aEndTime)
{
  TInt error = KErrNone;
  TRAP(error, {
    aEndTime = aEntry.EndTimeL().TimeLocalL();
  });
  if(error == KErrArgument){
    aEndTime = TCalTime::MinTime();
    return (KErrNone);
  }
  return error;
}

TInt GetInstanceStartTime(CCalInstance* aInstance,TTime& aStartTime)
{
  TInt error = KErrNone;
  TRAP(error, {
    aStartTime = aInstance->StartTimeL().TimeLocalL();
  });
  if(error == KErrArgument){
    aStartTime = TCalTime::MinTime();
    return (KErrNone);
  }
  return error;
}

TInt GetInstanceEndTime(CCalInstance& aInstance,TTime& aEndTime)
{
  TInt error = KErrNone;
  TRAP(error, {
    aEndTime = aInstance.EndTimeL().TimeLocalL();
  });
  if(error == KErrArgument){
    aEndTime = TCalTime::MinTime();
    return (KErrNone);
  }
  return error;
}

/*
 * CalendarDb methods.
 */

 

/*
 * Create new CalendarDb object.
 */
extern "C" PyObject *
new_CalendarDb_object(CCalSession* session, CCalEntryViewAdapter* entryViewAdapter)
{ 
  CalendarDb_object* calendarDb = 
    PyObject_New(CalendarDb_object, CalendarDb_type);
  if (calendarDb == NULL){
    delete entryViewAdapter;
    delete session;
    return PyErr_NoMemory();
  }
 
  calendarDb->session = session;
  calendarDb->entryViewAdapter = entryViewAdapter;
  calendarDb->instanceViewAdapter = NULL; // this is instantiated only if needed.
  
  return (PyObject*)calendarDb;
}


/*
 * This was implemented because there were crashes when using
 * CCalSession::OpenL (if the file did not exist).
 */
TInt isExistingFileL(CCalSession& session, const TDesC& filename)
{
  TInt pos = -1;
  CDesCArray* arr = session.ListCalFilesL();

  if(arr==NULL){
    return -1;
  }

  if(0 == arr->Find(filename,pos)){
    delete arr;
    return pos;
  };
  
  delete arr;
  return -1;
}

 
/*
 * Opens the database and creates and returns a CalendarDb-object.
 *
 * open() - opens the default agenda database file
 * open(u'<drive>:filename') - opens file if it exists.
 * open(u'<drive>:filename', 'c') - opens file, creates if the file does not exist.
 * open(u'<drive>:filename', 'n') - creates new empty database file (overwrites the existing one).
 */
extern "C" PyObject *
open_db(PyObject* /*self*/, PyObject *args)
{  
  PyObject* filename = NULL;
  char* flag = NULL;
  CCalSession* session = NULL;
  CCalEntryViewAdapter* entryViewAdapter = NULL;
  TInt err = KErrNone;
  TInt progressError = KErrNone;

  if (!PyArg_ParseTuple(args, "|Us", &filename, &flag)){ 
    return NULL;
  }
    
  // Open or create new agenda file.
  TRAP(err, session = CCalSession::NewL());
  if(err != KErrNone){
    return SPyErr_SetFromSymbianOSErr(err);
  }
  if(!flag){
    if(!filename){
      TRAP(err, {
        TInt fileExists = -1;        
        fileExists = isExistingFileL(*session,session->DefaultFileNameL());
        if(0<=fileExists){
          session->OpenL(session->DefaultFileNameL());
        }else{
          session->CreateCalFileL(session->DefaultFileNameL());
          session->OpenL(session->DefaultFileNameL());
        }
      }); 
      if(err != KErrNone){
        delete session;
        return SPyErr_SetFromSymbianOSErr(err);
      }
    }else{   
      TPtrC filenamePtr((TUint16*)PyUnicode_AsUnicode(filename),PyUnicode_GetSize(filename));
      TInt fileExists = -1;
      TRAP(err, {
        fileExists = isExistingFileL(*session,filenamePtr);
        if(0<=fileExists){
          session->OpenL(TPtrC((TUint16*)PyUnicode_AsUnicode(filename),PyUnicode_GetSize(filename)));
        }
      }); 
      if(err != KErrNone){
        delete session;
        return SPyErr_SetFromSymbianOSErr(err);
      }
      if(fileExists<0){
        delete session;
        PyErr_SetString(PyExc_RuntimeError, "no such file");
        return NULL; 
      }
    }
  }else{
    if(filename && flag[0] == 'c'){
      // Open, create if file doesn't exist.
      TPtrC filenamePtr((TUint16*)PyUnicode_AsUnicode(filename),PyUnicode_GetSize(filename));
      TInt fileExists = -1;
      TRAP(err, {
        fileExists = isExistingFileL(*session,filenamePtr);
        if(0<=fileExists){
          session->OpenL(filenamePtr);
        }else{
          session->CreateCalFileL(filenamePtr);
          session->OpenL(filenamePtr);
        }
      }); 
      if(err != KErrNone){
        delete session;
        return SPyErr_SetFromSymbianOSErr(err);
      }
    }else if(filename && flag[0] == 'n'){
      // Create a new empty file.
      TPtrC filenamePtr((TUint16*)PyUnicode_AsUnicode(filename),PyUnicode_GetSize(filename));
      TInt fileExists = -1;
      TRAP(err, {
        fileExists = isExistingFileL(*session,filenamePtr);
        if(0<=fileExists){
          session->DeleteCalFileL(filenamePtr);
          session->CreateCalFileL(filenamePtr);
          session->OpenL(filenamePtr);
        }else{
          session->CreateCalFileL(filenamePtr);
          session->OpenL(filenamePtr);
        }
      }); 
      if(err != KErrNone){
        delete session;
        return SPyErr_SetFromSymbianOSErr(err);
      }
    }else{
      delete session;
      PyErr_SetString(PyExc_SyntaxError, "illegal parameter combination");
      return NULL; 
    }  
  }
  
  TRAP(err,entryViewAdapter = CCalEntryViewAdapter::NewL(*session));
  if(err != KErrNone){
    delete session;
    return SPyErr_SetFromSymbianOSErr(err);
  } 
  
  Py_BEGIN_ALLOW_THREADS
  TRAP(err,progressError=entryViewAdapter->InitiateL());
  Py_END_ALLOW_THREADS
  if(err != KErrNone){
    delete entryViewAdapter;
    delete session;
    return SPyErr_SetFromSymbianOSErr(err);
  } 
  if(progressError!=KErrNone){
    delete entryViewAdapter;
    delete session;
    return SPyErr_SetFromSymbianOSErr(progressError);
  }
  
  return new_CalendarDb_object(session, entryViewAdapter);
}


/*
 * Returns calendar entry object (indicated by given unique id).
 */ 
static PyObject *
CalendarDb_getitem(CalendarDb_object *self, PyObject *args)
{
  TInt32 key = -1;
  
  if (!PyArg_ParseTuple(args, "i", &key)){ 
    return NULL;
  }
  return new_Entry_object(self, key);  
}


/*
 * Returns number of entries (also modifying/non-originating entries are included).
 */ 
static PyObject *
CalendarDb_entry_count(CalendarDb_object *self, PyObject * /*args*/)
{
  TInt error = KErrNone;
  TCalTime calTime;
  RArray<TCalLocalUid>* uidArray = NULL;
  PyObject* countObj = NULL;
  
  TRAP(error, {
    uidArray = new (ELeave) RArray<TCalLocalUid>(10);
    calTime.SetTimeUtcL(calTime.MinTime());
    self->entryViewAdapter->iEntryView->GetIdsModifiedSinceDateL(calTime, *uidArray);
  });
  if(error != KErrNone){
    delete uidArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  countObj = Py_BuildValue("i", uidArray->Count());
  
  uidArray->Close();
  delete uidArray;
  
  return countObj;
}


/*
 * Deallocate the calendarDb object.
 */
extern "C" {
  static void CalendarDb_dealloc(CalendarDb_object *calendarDb)
  { 
    delete calendarDb->instanceViewAdapter;
    delete calendarDb->entryViewAdapter;
    delete calendarDb->session;   
    
    calendarDb->instanceViewAdapter=NULL;
    calendarDb->entryViewAdapter=NULL;
    calendarDb->session=NULL;
    
    PyObject_Del(calendarDb);
  }
}


/*
 * Create unique (global) id string for calendar entry.
 */
void createUniqueString(TDes8& str)
{
  TTime theMoment;
  theMoment.UniversalTime();
  
  // fulfill the temp with random bits.
  TUint64 temp = Math::Random();
  temp <<= 32;
  temp += Math::Random();
  
  // add bits from microsecond value.
  temp += theMoment.MicroSecondsFrom(Time::MinTTime()).Int64();
  
  // clear the maxbit to avoid negative value.
  temp <<= 1;
  temp >>= 1;
  
  // convert the number to string.
  str.Num(temp); 
}


/*
 * Creates new entry.
 */ 
static PyObject *
CalendarDb_add_entry(CalendarDb_object *self, PyObject *args)
{
  TInt entryType = 0;
  if (!PyArg_ParseTuple(args, "i", &entryType)){ 
    return NULL;
  }

  TBuf8<50> randDesc;
  createUniqueString(randDesc);    
  return new_Entry_object(self, KNullEntryId, static_cast<CCalEntry::TType>(entryType),&randDesc);
}


/*
 * Removes the entry from the database.
 * -note that deleting the originating entry also removes the 
 * modifying entry.
 */ 
static PyObject *
CalendarDb_delete_entry(CalendarDb_object *self, PyObject* args)
{
  TInt error = KErrNone;
  TInt entryUid = 0;
  CCalEntry* entry = NULL;

  if (!PyArg_ParseTuple(args, "i", &entryUid)){ 
    return NULL;
  }
   
  TRAP(error, {
    entry = self->entryViewAdapter->iEntryView->FetchL(entryUid);
    if(entry!=NULL){
      self->entryViewAdapter->iEntryView->DeleteL(*entry);
    }
  });
  if(error != KErrNone){
    delete entry;
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(entry == NULL){
    PyErr_SetString(PyExc_RuntimeError, "no such entry");
    return NULL;
  }
  delete entry;
  
  Py_INCREF(Py_None);  
  return Py_None;
}


/*
 * Exports vcalendar entries from the database. 
 * -returned string contains entries indicated by content (unique ids)
 * of the tuple given as the parameter.
 */
extern "C" PyObject *
CalendarDb_export_vcals(CalendarDb_object* self, PyObject *args)
{
  RPointerArray<CCalEntry>* entryArray = NULL;
  PyObject* idTuple = NULL;
  PyObject* ret = NULL;
  TInt error = KErrNone;
  CCalEntry* entry = NULL;
  TInt i=0;

  if (!PyArg_ParseTuple(args, "O!", 
                        &PyTuple_Type, &idTuple)){ 
    return NULL;
  }
  
  if(PyTuple_Size(idTuple)<1){
    PyErr_SetString(PyExc_SyntaxError, 
                    "no calendar entry id:s given in the tuple");
    return NULL;
  }
  TRAP(error, 
      entryArray = new (ELeave) RPointerArray<CCalEntry>(PyTuple_Size(idTuple)));
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  // Put the calendar entrys into the entryArray.
  TInt entryCount = PyTuple_Size(idTuple);
  for(TInt i=0;i<entryCount;i++){
    PyObject* idItem = PyTuple_GetItem(idTuple, i);

    if(!PyInt_Check(idItem)){
      for(i=0;i<entryArray->Count();i++){
        delete ((*entryArray)[i]);
      }
      entryArray->Close();
      delete entryArray;
      PyErr_SetString(PyExc_TypeError, "illegal argument in the tuple");
      return NULL;
    };

    TRAP(error, 
      entry = self->entryViewAdapter->iEntryView->FetchL(PyInt_AsLong(idItem)));
    if(error != KErrNone){
      for(i=0;i<entryArray->Count();i++){
        delete ((*entryArray)[i]);
      }
      entryArray->Close();
      delete entryArray;
      return SPyErr_SetFromSymbianOSErr(error);
    }
    if(entry==NULL){
      for(i=0;i<entryArray->Count();i++){
        delete ((*entryArray)[i]);
      }
      entryArray->Close();
      delete entryArray;
      PyErr_SetString(PyExc_ValueError, "specified entry not found");
      return NULL;
    }

    TRAP(error, entryArray->AppendL(entry));
    if(error != KErrNone){
      for(i=0;i<entryArray->Count();i++){
        delete ((*entryArray)[i]);
      }
      entryArray->Close();
      delete entryArray;
      delete entry;
      return SPyErr_SetFromSymbianOSErr(error);
    }
  }

  // Get the vcalendar string.
  TRAP(error, {
    CBufFlat* flatBuf = CBufFlat::NewL(4);
    CleanupStack::PushL(flatBuf);
    RBufWriteStream outputStream(*flatBuf);
    CleanupClosePushL(outputStream);

    CCalDataExchange* dataExchange = CCalDataExchange::NewL(*(self->session));
    CleanupStack::PushL(dataExchange);
    
    TUid dataFormat(KUidVCalendar);
    dataExchange->ExportL(dataFormat,outputStream,*entryArray);

    outputStream.CommitL();        
    
    CleanupStack::PopAndDestroy(); // dataExchange.  
    CleanupStack::PopAndDestroy(); // Close outputStream.  

    ret = Py_BuildValue("s#", (char*)flatBuf->Ptr(0).Ptr(), flatBuf->Size()); 

    CleanupStack::PopAndDestroy(); // flatBuf.  
  });
  for(i=0;i<entryArray->Count();i++){
    delete ((*entryArray)[i]);
  }
  entryArray->Close();
  delete entryArray;
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  return ret;
}


/*
 * Imports vcalendar entries (given in the string parameter) to the database. 
 * -returns tuple that contains unique ids of the imported entries.
 */
extern "C" PyObject *
CalendarDb_import_vcals(CalendarDb_object* self, PyObject *args)
{
  RPointerArray<CCalEntry>* entryArray = NULL;
  char* vCalStr = NULL;
  TInt32 vCalStrLength = 0;
  PyObject* idTuple = NULL;
  TInt error = KErrNone;
  TInt flags = 0;
  TInt i=0;
  TInt numOfImportedEntries = 0;

  if (!PyArg_ParseTuple(args, "s#|i",
                        &vCalStr, &vCalStrLength, &flags)){ 
    return NULL;
  }

  TPtrC8 vCalStrPtr((TUint8*)vCalStr, vCalStrLength); 
  
  RMemReadStream inputStream(vCalStrPtr.Ptr(), vCalStrPtr.Length()); 

  TRAP(error, 
    entryArray = new (ELeave) RPointerArray<CCalEntry>(10));
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
    
  TRAP(error, {
    CleanupClosePushL(inputStream);
    CCalDataExchange* exchange = CCalDataExchange::NewL(*(self->session));
    CleanupStack::PushL(exchange);
    exchange->ImportL(KUidVCalendar,inputStream,*entryArray,flags);
    CleanupStack::PopAndDestroy(exchange); 
    CleanupStack::PopAndDestroy(); // Close inputStream.
  });
  if(error!=KErrNone){
    for(i=0;i<entryArray->Count();i++){
      delete ((*entryArray)[i]);
    }
    entryArray->Close();
    delete entryArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }  
  TRAP(error, {
    self->entryViewAdapter->iEntryView->StoreL(*entryArray,numOfImportedEntries);
  });
  if(error!=KErrNone){
    for(i=0;i<entryArray->Count();i++){
      delete ((*entryArray)[i]);
    };
    entryArray->Close();
    delete entryArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }
    
  idTuple = PyTuple_New(numOfImportedEntries);
  if(NULL!=idTuple){
    for(i=0;i<entryArray->Count();i++){
      TCalLocalUid uid = KNullEntryId;
      TRAP(error, {
        uid = ((*entryArray)[i])->LocalUidL();
      });
      if(error!=KErrNone){
        for(i=0;i<entryArray->Count();i++){
          delete ((*entryArray)[i]);
        };
        entryArray->Close();
        delete entryArray;
        Py_DECREF(idTuple);
        return SPyErr_SetFromSymbianOSErr(error);
      }
      if(uid == KNullEntryId){
        continue;
      }
      PyObject* id = Py_BuildValue("i",uid);
      if(id == NULL || PyTuple_SetItem(idTuple, i, id)<0){
        for(i=0;i<entryArray->Count();i++){
          delete ((*entryArray)[i]);
        };
        entryArray->Close();
        delete entryArray;
        Py_DECREF(idTuple);
        return NULL;
      }
    }
  } 
  for(i=0;i<entryArray->Count();i++){
    delete ((*entryArray)[i]);
  }
  entryArray->Close();
  delete entryArray;
  
  if(idTuple==NULL){
    return PyErr_NoMemory();
  }
  return idTuple;
}


/*
 * Do the actual searching.
 */
extern "C" PyObject *
CalendarDb_search_instances(CalendarDb_object* self, TTime startTTime, TTime endTTime, TInt filter, PyObject* searchText)
{
  TInt error = KErrNone;
  TInt progressError = KErrNone;
  TCalTime startCalTime;
  TCalTime endCalTime;
  TDateTime endDate;
  RPointerArray<CCalInstance> instanceArray;
  PyObject* idList = NULL;
  TInt i=0;
      
  if(Check_time_validity(startTTime)==EFalse){
    PyErr_SetString(PyExc_ValueError, "illegal start date");
    return NULL;
  }
  if(Check_time_validity(endTTime)==EFalse){
    PyErr_SetString(PyExc_ValueError, "illegal end date");
    return NULL;
  }
  
  if(filter==0){
    filter = CalCommon::EIncludeAll;
  }
   
  endDate = endTTime.DateTime();
  TInt endDateday = endDate.Day();
  TInt  totalDayMonth = Time::DaysInMonth(endDate.Year(), endDate.Month());
  /* Increment return value of TDateTime::Day() by one as it 
   * returns one day lesser than actual day.
   */
  if(totalDayMonth == (endDateday + 1)){
    if(endDate.Month()==EDecember){
      endDate.SetYear(endDate.Year() + 1);
    }
    endDate.SetDay(1);
    endDate.SetMonth(getNextMonth(endDate.Month()));
  }else{
    endDate.SetDay(endDate.Day()+1);
  }
 
  endTTime = endDate;
  startTTime=truncToDate(startTTime);
  endTTime=truncToDate(endTTime);
   
  TRAP(error, {
    startCalTime.SetTimeUtcL(startTTime);
    endCalTime.SetTimeUtcL(endTTime);
  });
  if(error!=KErrNone){
    instanceArray.Close();
    return SPyErr_SetFromSymbianOSErr(error);  
  }
  CalCommon::TCalTimeRange timeRange(startCalTime,endCalTime);
   
  if(self->instanceViewAdapter == NULL){
    Py_BEGIN_ALLOW_THREADS
    TRAP(error, {
      self->instanceViewAdapter = CCalInstanceViewAdapter::NewL(*(self->session));
      progressError = self->instanceViewAdapter->InitiateL();
    });
    Py_END_ALLOW_THREADS
    if(error!=KErrNone){
      instanceArray.Close();
      delete self->instanceViewAdapter;
      self->instanceViewAdapter = NULL;
      return SPyErr_SetFromSymbianOSErr(error);      
    }  
    if(progressError!=KErrNone){
      instanceArray.Close();
      delete self->instanceViewAdapter;
      self->instanceViewAdapter = NULL;
      return SPyErr_SetFromSymbianOSErr(progressError);      
    }    
  }
  TRAP(error, {
    if(searchText==NULL){
      self->instanceViewAdapter->iInstanceView->FindInstanceL(instanceArray,
                                                              filter,
                                                              timeRange);
    }else{
      TPtrC searchTextPtr((TUint16*) PyUnicode_AsUnicode(searchText), 
                          PyUnicode_GetSize(searchText));
      CCalInstanceView::TCalSearchParams searchParams(searchTextPtr,CalCommon::EFoldedTextSearch);
      self->instanceViewAdapter->iInstanceView->FindInstanceL(instanceArray,
                                                              filter,
                                                              timeRange,
                                                              searchParams);
    }
  });
  if(error!=KErrNone){
    DELETE_INSTANCES
    return SPyErr_SetFromSymbianOSErr(error);     
  }
                                                        
  idList = PyList_New(instanceArray.Count()); 
  if(idList == NULL){
    DELETE_INSTANCES
    return PyErr_NoMemory();
  }
  
  TCalLocalUid uid;
  TTime instStartTTime;
  for(i=0;i<instanceArray.Count();i++){
    CCalInstance* instance = instanceArray[i];
    CCalEntry& entry = instance->Entry();
    
    TRAP(error, {
      uid = entry.LocalUidL();
      error = GetInstanceStartTime(instance,instStartTTime);    
    });
    if(error!=KErrNone){
      DELETE_INSTANCES
      Py_DECREF(idList);
      return SPyErr_SetFromSymbianOSErr(error);  
    }
      
    PyObject* instanceInfo =
      Py_BuildValue("{s:i,s:d}",(const char*)(&KUniqueId)->Ptr(),
                                uid,
                                (const char*)(&KDateTime)->Ptr(), 
                                time_as_UTC_TReal(instStartTTime));
                                
    if(instanceInfo==NULL){
      Py_DECREF(idList);
      DELETE_INSTANCES
      return NULL;
    }
                                 
    if(PyList_SetItem(idList, i, instanceInfo)<0){
      Py_DECREF(idList);
      DELETE_INSTANCES
      return NULL;
    }                              
  }
  
  DELETE_INSTANCES
 
  return idList;
}


/*
 * Finds instances (that occur between the specified time interval) 
 * (optionally by search string).
 */
extern "C" PyObject *
CalendarDb_find_instances(CalendarDb_object* self, PyObject *args)
{
  TReal startDate = 0;
  TReal endDate = 0;
  TInt filter = 0;
  TTime startTTime;
  TTime endTTime;
  PyObject* searchText = NULL;
   
  if (!PyArg_ParseTuple(args, "dd|Ui", &startDate, &endDate, &searchText, &filter)){ 
    return NULL;
  }

  pythonRealAsTTime(startDate, startTTime);
  pythonRealAsTTime(endDate, endTTime);
  
  return CalendarDb_search_instances(self, startTTime, endTTime, filter, searchText);
}
  

/*
 * Returns all instances (instance = entry's unique id + 
 * datetime of the specific occurrence of the entry)
 * of the given month.
 */
extern "C" PyObject *
CalendarDb_monthly_instances(CalendarDb_object* self, PyObject *args)
{
  TReal month = 0;
  TInt filter = 0;
  TTime monthTTime;
  TTime monthStartTTime;
  TTime monthEndTTime;
  TDateTime monthStart;
  TDateTime monthEnd;
  if (!PyArg_ParseTuple(args, "d|i", &month, &filter)){ 
    return NULL;
  }

  pythonRealAsTTime(month, monthTTime);    
  if(Check_time_validity(monthTTime)==EFalse){
    PyErr_SetString(PyExc_ValueError, "illegal date value");
    return NULL;
  }
  
  monthStart = monthTTime.DateTime();
  monthStart.SetDay(0);
  
  monthEnd = monthTTime.DateTime();
  monthEnd.SetDay(monthTTime.DaysInMonth()-1);

  monthStartTTime = monthStart;
  monthEndTTime = monthEnd;
  
  return CalendarDb_search_instances(self,monthStartTTime,monthEndTTime,filter,NULL);  
}


/*
 * Returns all instances (instance = entry's unique id + 
 * datetime of the specific occurrence of the entry)
 * of the given day.
 */
extern "C" PyObject *
CalendarDb_daily_instances(CalendarDb_object* self, PyObject *args)
{
  TReal day = 0;
  TTime startTTime;
  TTime endTTime;
  TInt filter = 0;
  if (!PyArg_ParseTuple(args, "d|i", &day, &filter)){ 
    return NULL;
  }
 
  pythonRealAsTTime(day, startTTime);
  endTTime = startTTime;
  return CalendarDb_search_instances(self,startTTime,endTTime,filter,NULL);  
}



/*
 * Entry methods.
 */

 
 
/*
 * Creates new calendar entry "wrapper" object.
 * -fetches the entry identified by uniqueId from the 
 * database and wraps that into the python object.
 * -if uniqueId is KNullEntryId a new entry is created (but 
 * not added to the database until entry.commit() is called).
 * in this case entry's unique id will remain as KNullEntryId 
 * until the entry is added (by committing) into the database. 
 */
extern "C" PyObject *
new_Entry_object(CalendarDb_object* self, 
                 TCalLocalUid uniqueId, 
                 CCalEntry::TType entryType,
                 const TDesC8* globalUid)
{      
  TInt err = KErrNone;
  CCalEntry* entry = NULL;
  Entry_object* entryObj = NULL;
  
  if(KNullEntryId == uniqueId){    
    // Create new entry.
    TRAP(err, {
      //TODO: HERE IS PROTECTION NEEDED, if globalUid ==NULL, program will panic
      HBufC8* uidBuf = HBufC8::NewLC(globalUid->Length());
      *uidBuf = *globalUid;
      // note that CCalEntry takes the ownership of uidBuf.
      entry = CCalEntry::NewL(entryType, uidBuf, CCalEntry::EMethodNone, 0);
      CleanupStack::Pop(uidBuf);
    });
    if(err != KErrNone){
      return SPyErr_SetFromSymbianOSErr(err);
    } 
    //Initialise time to TCalTime::MinTime() instead to Time::NullTTime().
    //There's a platform bug that Time::NullTTime() can't be used 
    //with TCalTime::StartTimeL() and similar functions.
    TCalTime minTime;
    minTime.SetTimeUtcL(TCalTime::MinTime());     
  	entry->SetStartAndEndTimeL(minTime, minTime);
  }
  else{    
    // Wrap an existing entry.
    TRAP(err, {
      entry = self->entryViewAdapter->iEntryView->FetchL(uniqueId);
    });
    if(err != KErrNone){
      return SPyErr_SetFromSymbianOSErr(err);
    } 
    if(entry==NULL){
      PyErr_SetString(PyExc_RuntimeError, "no such entry");
      return NULL;
    } 
  } 
   
  entryObj = PyObject_New(Entry_object, Entry_type); 
  if(entryObj == NULL){
    delete entry;
    return PyErr_NoMemory();
  }
 
  entryObj->calendarDb = self;
  entryObj->entry = entry; 
   
  Py_INCREF(self); // increase the database's reference count so that it cannot be deleted
                   // until all the entries have been deleted (otherwise a crash will follow).
  return (PyObject*)entryObj;
};
 

/*
 * Check if this is an originating entry (returns 1) or
 * modifying entry (returns 0).
 */ 
extern "C" PyObject *
Entry_originating_entry(Entry_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  TInt originatingEntry = 0;
  TCalTime calTime;
  TRAP(error, {
    calTime = self->entry->RecurrenceIdL();
    if(EFalse==Check_time_validity(calTime.TimeUtcL())){
      // "recurrence id" not set => this is an originating entry.
      originatingEntry = 1;
    }
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  return Py_BuildValue("i", originatingEntry);
}

/*
 * Saves the entry into the database.
 * -new entries are added into the database, old entries are updated.
 */ 
extern "C" PyObject *
Entry_commit(Entry_object* self, PyObject* /*args*/)
{  
  TInt error = KErrNone;
  TInt updateSuccess = 0;
  TCalLocalUid uid = 0;
  TBool timeSet = EFalse;
  TInt originatingEntry = 0;
  RPointerArray<CCalEntry>* entryArray = NULL;
  
  TRAP(error, {
    TCalTime startTime = self->entry->StartTimeL();
    if(startTime.TimeUtcL() != TCalTime::MinTime()){
      timeSet = ETrue;
    };
  });     
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(timeSet == EFalse){
    PyErr_SetString(PyExc_RuntimeError, "entry's time not set");
    return NULL;
  };
  
  TRAP(error, { 
    uid = self->entry->LocalUidL();
    entryArray = new (ELeave) RPointerArray<CCalEntry>(1);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  TRAP(error, entryArray->AppendL(self->entry));
  if(error != KErrNone){
    entryArray->Close();
    delete entryArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }
    
  TRAP(error, {
    TCalTime calTime = self->entry->RecurrenceIdL();
    if(EFalse==Check_time_validity(calTime.TimeUtcL())){
      // "recurrence id" not set => this is an originating entry.
      originatingEntry = 1;
    }
  });
  if(error != KErrNone){
    entryArray->Close();
    delete entryArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(uid == KNullEntryId || !originatingEntry){
    // Compeletely new entry or modifying (non-originate) entry.
    TRAP(error,self->calendarDb->entryViewAdapter->iEntryView->StoreL(*entryArray,updateSuccess));
  }else{    
    // Entry already exists in the database, but must be updated.
    TRAP(error,self->calendarDb->entryViewAdapter->iEntryView->UpdateL(*entryArray,updateSuccess));
  }
  entryArray->Close();
  delete entryArray;
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(updateSuccess<1){ 
    PyErr_SetString(PyExc_RuntimeError, "entry could not be stored");
    return NULL;
  };
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns the type of the entry (appt, event, anniv, todo, reminder).
 */
extern "C" PyObject *
Entry_entry_type(Entry_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  TInt entryType = 0;
  TRAP(error, {
    entryType = self->entry->EntryTypeL();
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return Py_BuildValue("i", entryType);
}


/*
 * Returns entry's content text.
 */
extern "C" PyObject *
Entry_content(Entry_object* self, PyObject* /*args*/)
{ 
  PyObject* ret = NULL;
  HBufC* buf = NULL;
  TRAPD(error, {    
    buf = HBufC::NewL(self->entry->SummaryL().Length());
    CleanupStack::PushL(buf);
    *buf = self->entry->SummaryL();
    CleanupStack::Pop(); // buf.
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  ret = Py_BuildValue("u#", buf->Ptr(), buf->Length()); 
  delete buf;

  return ret;
}


/*
 * Returns entry's description text.
 */
extern "C" PyObject *
Entry_description(Entry_object* self, PyObject* /*args*/)
{ 
  PyObject* ret = NULL;
  HBufC* buf = NULL;
  TRAPD(error, {
    buf = HBufC::NewL(self->entry->DescriptionL().Length());
    CleanupStack::PushL(buf);
    *buf = self->entry->DescriptionL();
    CleanupStack::Pop(); // buf.
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  ret = Py_BuildValue("u#", buf->Ptr(), buf->Length()); 
  delete buf;

  return ret;
}


/*
 * Sets the content (text) of the entry.
 */
extern "C" PyObject *
Entry_set_content(Entry_object* self, PyObject* args)
{
  TInt error = KErrNone;
  PyObject* content = NULL;
  if (!PyArg_ParseTuple(args, "U", &content)){ 
    return NULL;
  }

  TPtrC contentPtr((TUint16*) PyUnicode_AsUnicode(content), 
                   PyUnicode_GetSize(content));
                   
  TRAP(error, {
    self->entry->SetSummaryL(contentPtr);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Sets the Description (text) of the entry.
 */
extern "C" PyObject *
Entry_set_description(Entry_object* self, PyObject* args)
{
  TInt error = KErrNone;
  PyObject* description = NULL;
  if (!PyArg_ParseTuple(args, "U", &description)){ 
    return NULL;
  }

  TPtrC descriptionPtr((TUint16*) PyUnicode_AsUnicode(description), 
                   PyUnicode_GetSize(description));

  TRAP(error, {
    self->entry->SetDescriptionL(descriptionPtr);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns entry's location text. 
 */
extern "C" PyObject *
Entry_location(Entry_object* self, PyObject* /*args*/)
{
  PyObject* ret = NULL;
  HBufC* buf = NULL;
  TRAPD(error, {    
    buf = HBufC::NewL(self->entry->LocationL().Length());
    CleanupStack::PushL(buf);
    *buf = self->entry->LocationL();
    CleanupStack::Pop(); // buf.
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  ret = Py_BuildValue("u#", buf->Ptr(), buf->Length()); 
  delete buf;
  
  return ret;
}


/*
 * Sets entry's location information (text). 
 */
extern "C" PyObject *
Entry_set_location(Entry_object* self, PyObject* args)
{
  PyObject* location = NULL;
  if (!PyArg_ParseTuple(args, "U", &location)){ 
    return NULL;
  }
  TPtrC locationPtr((TUint16*) PyUnicode_AsUnicode(location), 
                    PyUnicode_GetSize(location));
  TRAPD(error, {
    self->entry->SetLocationL(locationPtr);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns entry's id. 
 */
extern "C" PyObject *
Entry_unique_id(Entry_object* self, PyObject* /*args*/)
{ 
  TInt error = KErrNone;
  TCalLocalUid id = 0;
  TRAP(error, {
    id = self->entry->LocalUidL(); 
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  return Py_BuildValue("i", id); 
}


/*
 * Returns entry's start datetime.
 */
extern "C" PyObject *
Entry_start_datetime(Entry_object* self, PyObject* /*args*/)
{
  TTime startTTime;
  TInt error = KErrNone;
   
  error = GetStartTime(*(self->entry), startTTime);
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(startTTime==TCalTime::MinTime()){
    Py_INCREF(Py_None);
    return Py_None;
  }
  
  return ttimeAsPythonFloat(startTTime); 
}


/*
 * Returns entry's end datetime.
 */
extern "C" PyObject *
Entry_end_datetime(Entry_object* self, PyObject* /*args*/)
{
  TTime endTTime;
  TInt error = KErrNone;
  
  GetEndTime(*(self->entry), endTTime);
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(endTTime==TCalTime::MinTime()){
    Py_INCREF(Py_None);
    return Py_None;
  }
  return ttimeAsPythonFloat(endTTime); 
}


/*
 * Sets entry's start and end datetime.
 */
extern "C" PyObject *
Entry_set_start_and_end_datetime(Entry_object* self, PyObject* args)
{
  TInt error = KErrNone;
  TReal startTime = 0;
  TReal endTime = 0;
  TTime startTTime;
  TTime endTTime;
  TCalTime startCalTime;
  TCalTime endCalTime;
  
  if (!PyArg_ParseTuple(args, "dd", &startTime, &endTime)){ 
    return NULL;
  }
  
  pythonRealAsTTime(startTime, startTTime);
  pythonRealAsTTime(endTime, endTTime);
  
  if(EFalse==Check_time_validity(startTTime)){
    PyErr_SetString(PyExc_ValueError, 
                    "illegal start datetime");
    return NULL;
  }
  if(EFalse==Check_time_validity(endTTime)){
    PyErr_SetString(PyExc_ValueError, 
                    "illegal end datetime");
    return NULL;
  }
  
  TRAP(error, {
    startCalTime.SetTimeLocalL(startTTime);
    endCalTime.SetTimeLocalL(endTTime);
    self->entry->SetStartAndEndTimeL(startCalTime, endCalTime);
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Sets entry's priority value.
 *
 * the native calendar application uses following priority values:
 * 1, high
 * 2, normal
 * 3, low
 * others, low (exception:in the calendar (the initial value) 0 = normal, but in the todo app it is not). 
 */
extern "C" PyObject *
Entry_set_priority(Entry_object* self, PyObject* args)
{
  TInt priority = 0;
  TInt error = KErrNone;
  if (!PyArg_ParseTuple(args, "i", &priority)){ 
    return NULL;
  }

  TRAP(error, {
    self->entry->SetPriorityL(priority);
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns entry's priority value. 
 */
extern "C" PyObject *
Entry_priority(Entry_object* self, PyObject* /*args*/)
{
  TInt priority = 0;
  TInt error = KErrNone;

  TRAP(error, {
    priority = self->entry->PriorityL();
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return Py_BuildValue("i", priority);
}


/*
 * Returns the datetime this entry was last modified.
 */
extern "C" PyObject *
Entry_last_modified(Entry_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  TTime lastModifiedTTime;
  
  TRAP(error, {
    TCalTime lastModifiedCalTime = self->entry->LastModifiedDateL();
    lastModifiedTTime = lastModifiedCalTime.TimeLocalL();
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  return ttimeAsPythonFloat(lastModifiedTTime);
}


/*
 * Returns entry's replication status.
 */
extern "C" PyObject *
Entry_replication(Entry_object* self, PyObject* /*args*/)
{
  CCalEntry::TReplicationStatus status;
  TInt error = KErrNone;
  
  TRAP(error, {
    status =  self->entry->ReplicationStatusL();
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return Py_BuildValue("i", status);  
}


/*
 * Sets entry's replication status.
 */
extern "C" PyObject *
Entry_set_replication(Entry_object* self, PyObject* args)
{  
  TInt error = KErrNone;
  TInt replicationStatus = 0;
  if (!PyArg_ParseTuple(args, "i", &replicationStatus)){ 
    return NULL;
  }
  
  if(replicationStatus!=CCalEntry::EOpen && 
     replicationStatus!=CCalEntry::EPrivate &&
     replicationStatus!=CCalEntry::ERestricted){
     PyErr_SetString(PyExc_ValueError, 
                    "illegal replication status");
     return NULL;    
  }

  TRAP(error, {
    self->entry->SetReplicationStatusL(static_cast<CCalEntry::TReplicationStatus>(replicationStatus)); 
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Sets the given crossed out datetime for the todo entry.
 */
extern "C" PyObject *
Entry_set_crossed_out(Entry_object* self, PyObject* args)
{
  TInt error = KErrNone;
  TReal crossedOutDate = 0;
  TTime crossedOutTTime;
  TInt completed = 0;
  TBool isCompleted = EFalse;
  TBool isTodo = EFalse;
  if (!PyArg_ParseTuple(args, "id", &completed, &crossedOutDate)){ 
    return NULL;
  }
  
  if(completed != 0){
    isCompleted = ETrue;
  }else{
    isCompleted = EFalse;
  }
  
  TRAP(error, {
    if(self->entry->EntryTypeL()==CCalEntry::ETodo){
      isTodo = ETrue;
    }
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(isTodo==EFalse){ 
    PyErr_SetString(PyExc_TypeError, 
                    "cross out property is valid only for todo entries");
    return NULL;
  }
  
  pythonRealAsTTime(crossedOutDate,crossedOutTTime);
  TRAP(error, {
    TCalTime crossedOutCalTime;
    crossedOutCalTime.SetTimeUtcL(crossedOutTTime);
    self->entry->SetCompletedL(isCompleted,crossedOutCalTime);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns crossed out datetime (only for todos).
 */
extern "C" PyObject *
Entry_crossed_out_date(Entry_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  TTime crossOutTTime;
  TBool isTodo = EFalse;
  
  TRAP(error, {
    if(self->entry->EntryTypeL()==CCalEntry::ETodo){
      isTodo = ETrue;
    }
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(isTodo==EFalse){ 
    PyErr_SetString(PyExc_TypeError, 
                    "cross out property is valid only for todo entries");
  }
 
  TRAP(error, {
    TCalTime crossOutCalTime = self->entry->CompletedTimeL();  
    crossOutTTime = crossOutCalTime.TimeUtcL();
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(crossOutTTime == Time::NullTTime()){
    Py_INCREF(Py_None);
    return Py_None;
  }
  
  return ttimeAsPythonFloat(crossOutTTime);
}


/*
 * Sets an alarm to the entry.
 * -time of the alarm is given as a parameter
 */
extern "C" PyObject *
Entry_set_alarm(Entry_object* self, PyObject* args)
{
  TInt error = KErrNone;
  TReal alarmTime = 0;
  TTime alarmTTime;
  TTime startTTime;
  TCalTime calTime;
  TTimeIntervalMinutes interval;
  CCalAlarm* alarm = NULL;
  TBool timeSet = EFalse;
  
  if (!PyArg_ParseTuple(args, "d", &alarmTime)){ 
    return NULL;
  }
  
  TRAP(error, {
    calTime = self->entry->StartTimeL();
    startTTime = calTime.TimeUtcL();
    if(startTTime != Time::NullTTime()){
      timeSet = ETrue;
    };
  });     
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(timeSet == EFalse){
    PyErr_SetString(PyExc_RuntimeError, "entry's time not set");
    return NULL;
  };
  
  pythonRealUtcAsTTime(alarmTime, alarmTTime);
  
  // must do some checks or crash will follow.
  
  if(EFalse==Check_time_validity(alarmTTime)){
    PyErr_SetString(PyExc_ValueError, 
                    "illegal alarm datetime value");
    return NULL;
  }
  
  if(startTTime.DaysFrom(alarmTTime)>=TTimeIntervalDays(EARLIEST_ALARM_DAY_INTERVAL)){
    PyErr_SetString(PyExc_ValueError, "alarm datetime too early for the entry");
    return NULL;
  }
 
  if(alarmTTime.DateTime().Year()>startTTime.DateTime().Year() ||
    (alarmTTime.DateTime().Year()==startTTime.DateTime().Year() && alarmTTime.DayNoInYear()>startTTime.DayNoInYear())){
    PyErr_SetString(PyExc_ValueError, "alarm datetime too late for the entry");
    return NULL;
  }
 
  // do the actual alarm setting.
  TRAP(error, { 
    startTTime.MinutesFrom(alarmTTime,interval);
    alarm = CCalAlarm::NewL();
    CleanupStack::PushL(alarm);
    alarm->SetTimeOffset(interval);
    self->entry->SetAlarmL(alarm);
    CleanupStack::PopAndDestroy(alarm);
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns the alarm datetime.
 */
extern "C" PyObject *
Entry_alarm_datetime(Entry_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  TTime alarmTTime;
  CCalAlarm* alarm = NULL;
  TBool hasAlarm = EFalse;
  
  TRAP(error, {
    alarm = self->entry->AlarmL();
    if(alarm!=NULL){
      CleanupStack::PushL(alarm);
      hasAlarm = ETrue;
      TTimeIntervalMinutes interval = alarm->TimeOffset();
      TCalTime calTime = self->entry->StartTimeL();
      TTime startTTime=calTime.TimeLocalL();
      alarmTTime = startTTime-interval; 
      CleanupStack::PopAndDestroy(alarm); 
    }
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(hasAlarm!=EFalse){
    return ttimeAsPythonFloat(alarmTTime);  
  }
  Py_INCREF(Py_None);
  return Py_None; 
}


/*
 * Cancels entry's alarm.
 */
extern "C" PyObject *
Entry_cancel_alarm(Entry_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  
  TRAP(error, {
    self->entry->SetAlarmL(NULL);
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Puts the given repeat information into the entry.
 */
extern "C" PyObject *
Entry_install_repeat_data(Entry_object* self, 
                          TReal startDate,
                          TReal endDate,
                          TInt interval,
                          TInt repeatIndicator,
                          RArray<TCalTime>* exceptionArray,
                          PyObject* repeatDays,
                          TBool eternalRepeat)
{
  TInt error = KErrNone;
  TTime startTime;
  TTime endTime;
  TCalTime startCalTime;
  TCalTime endCalTime;
  RArray<TDay>* repeatDayArray = NULL;
  RArray<TInt>* repeatDateArray = NULL;
  RArray<TCalRRule::TDayOfMonth>* repeatDayOfMonthArray = NULL;
  
  switch(repeatIndicator){
    case NOT_REPEATED:
      {
        TRAP(error, {
          self->entry->ClearRepeatingPropertiesL();
        });
        if(error != KErrNone){
          return SPyErr_SetFromSymbianOSErr(error);
        }
      }
    break;
    case DAILY_REPEAT:
      {  
        TCalRRule rpt(TCalRRule::EDaily);
        GET_REP_START_AND_END
        SET_REPEAT_DATES
        ADD_REPEAT  
      }
    break;
    case WEEKLY_REPEAT:
      {        
        TCalRRule rpt(TCalRRule::EWeekly);  
        GET_REP_START_AND_END
        SET_REPEAT_DATES
                
        if(repeatDays){
          if(!PyList_Check(repeatDays)){
            PyErr_SetString(PyExc_SyntaxError, 
                            "weekly repeat days must be given in a list");
            return NULL;
          }
          if(PyList_Size(repeatDays)>0){
            TRAP(error, 
                 repeatDayArray = new (ELeave) RArray<TDay>(PyList_Size(repeatDays)));
            if(error!=KErrNone){
              return SPyErr_SetFromSymbianOSErr(error);
            }
          }
          for(TInt i=0;i<PyList_Size(repeatDays);i++){
            PyObject* day = PyList_GetItem(repeatDays, i);
            if(!PyInt_Check(day) || 
               0>PyInt_AsLong(day) || 
               EFalse==isWeekday(PyInt_AsLong(day))){
              PyErr_SetString(PyExc_ValueError, 
                              "bad value for weekly repeat day");
              repeatDayArray->Close();
              delete repeatDayArray;
              return NULL;
            }
            TRAP(error, {
              repeatDayArray->AppendL((static_cast<TDay>(PyInt_AsLong(day))));
            });
            if(error!=KErrNone){
              repeatDayArray->Close();
              delete repeatDayArray;
              return SPyErr_SetFromSymbianOSErr(error);
            }
          }
        }
                
        if(repeatDayArray!=NULL){
          rpt.SetByDay(*repeatDayArray); 
          repeatDayArray->Close();
        }else{
          // must set some day or the program will crash.
          TRAP(error, {
            repeatDayArray = new (ELeave) RArray<TDay>(1);
            CleanupClosePushL(*repeatDayArray);
            repeatDayArray->AppendL(startTime.DayNoInWeek());
            rpt.SetByDay(*repeatDayArray);
            CleanupStack::PopAndDestroy(repeatDayArray);
          });
        };
        delete repeatDayArray;
        if(error!=KErrNone){
          return SPyErr_SetFromSymbianOSErr(error);
        }     
              
        ADD_REPEAT 
      }
    break;
    case MONTHLY_BY_DATES_REPEAT:
      { 
         TCalRRule rpt(TCalRRule::EMonthly);
         GET_REP_START_AND_END         
         SET_REPEAT_DATES
         
         if(repeatDays){
           if(!PyList_Check(repeatDays)){
             PyErr_SetString(PyExc_SyntaxError, 
                             "monthly repeat dates must be given in a list");
             return NULL;
           }
           if(PyList_Size(repeatDays)>0){
             TRAP(error, 
                  repeatDateArray = new (ELeave) RArray<TInt>(PyList_Size(repeatDays)));
             if(error!=KErrNone){
               return SPyErr_SetFromSymbianOSErr(error);
             }
           }
           for(TInt i=0;i<PyList_Size(repeatDays);i++){
             PyObject* day = PyList_GetItem(repeatDays, i);
             if(!PyInt_Check(day) || 
                0>PyInt_AsLong(day) || 
                PyInt_AsLong(day)>=DAYS_IN_MONTH){
               PyErr_SetString(PyExc_ValueError, 
                               "monthly repeat dates must be integers (0-30)");
               repeatDateArray->Close();                
               delete repeatDateArray;
               return NULL;
             }
             TRAP(error, {
               repeatDateArray->AppendL(PyInt_AsLong(day));
             });
             if(error!=KErrNone){
               repeatDateArray->Close();
               delete repeatDateArray;
               return SPyErr_SetFromSymbianOSErr(error);               
             }
           }
         }
         if(repeatDateArray!=NULL){
           rpt.SetByMonthDay(*repeatDateArray);
           repeatDateArray->Close();
         }else{
           TRAP(error, {
             repeatDateArray = new (ELeave) RArray<TInt>(1);
             CleanupClosePushL(*repeatDateArray);
             repeatDateArray->AppendL(startTime.DayNoInMonth());
             rpt.SetByMonthDay(*repeatDateArray);
             CleanupStack::PopAndDestroy(repeatDateArray);
           });
         }
         delete repeatDateArray;
         if(error!=KErrNone){
           return SPyErr_SetFromSymbianOSErr(error);                
         };
         ADD_REPEAT 
      }
    break;
    case MONTHLY_BY_DAYS_REPEAT:
      { 
         TCalRRule rpt(TCalRRule::EMonthly);
         GET_REP_START_AND_END 
         SET_REPEAT_DATES
         
          // Add repeat days.
         if(repeatDays){
           if(!PyList_Check(repeatDays)){
             PyErr_SetString(PyExc_SyntaxError, 
                             "monthly repeat days must be given in a list");
             return NULL;
           }
           if(PyList_Size(repeatDays)>0){
             TRAP(error, 
                  repeatDayOfMonthArray = new (ELeave) RArray<TCalRRule::TDayOfMonth>(PyList_Size(repeatDays)));
             if(error!=KErrNone){
               return SPyErr_SetFromSymbianOSErr(error);
             }
           }
           PyObject* weekKey = Py_BuildValue("s", (const char*)(&KWeek)->Ptr());
           PyObject* dayKey = Py_BuildValue("s", (const char*)(&KDay)->Ptr());
           if(!(weekKey && dayKey)){
             Py_XDECREF(weekKey);
             Py_XDECREF(dayKey);
             repeatDayOfMonthArray->Close();
             delete repeatDayOfMonthArray;
             return NULL;
           } 
           for(TInt i=0;i<PyList_Size(repeatDays);i++){
             PyObject* day = PyList_GetItem(repeatDays, i);
             if(!PyDict_Check(day)){
               Py_DECREF(weekKey);
               Py_DECREF(dayKey);
               PyErr_SetString(PyExc_TypeError, 
                               "repeat day must be dictionary"); 
               repeatDayOfMonthArray->Close();
               delete repeatDayOfMonthArray;
               return NULL;
             }             
             PyObject* weekNum = PyDict_GetItem(day, weekKey);
             PyObject* dayNum = PyDict_GetItem(day, dayKey);
             if(!(weekNum && dayNum && 
                PyInt_Check(weekNum) && PyInt_Check(dayNum))){
               Py_DECREF(weekKey);
               Py_DECREF(dayKey);
               PyErr_SetString(PyExc_SyntaxError, 
                               "week and day must be given and they must be integers");
               repeatDayOfMonthArray->Close();
               delete repeatDayOfMonthArray;
               return NULL;
             }
             TInt weekNumInt = PyInt_AsLong(weekNum);
             TInt dayNumInt = PyInt_AsLong(dayNum);
             if(isWeekInMonth(weekNumInt)==EFalse || 
                isWeekday(dayNumInt)==EFalse){
               Py_DECREF(weekKey);
               Py_DECREF(dayKey);
               PyErr_SetString(PyExc_ValueError, "bad value for week or day");
               repeatDayOfMonthArray->Close();
               delete repeatDayOfMonthArray;
               return NULL;
             }
             weekNumInt++;
             if(weekNumInt==5){
               weekNumInt=-1; // because of >=3.0 SDK.
             }
             TCalRRule::TDayOfMonth dayOfMonth(static_cast<TDay>(dayNumInt),weekNumInt);
             TRAP(error, {
               repeatDayOfMonthArray->AppendL(dayOfMonth);
             });
             if(error!=KErrNone){
               Py_DECREF(weekKey);
               Py_DECREF(dayKey);
               repeatDayOfMonthArray->Close();
               delete repeatDayOfMonthArray;
               return SPyErr_SetFromSymbianOSErr(error);               
             }
           }
           Py_DECREF(weekKey);
           Py_DECREF(dayKey);
         }
         if(repeatDayOfMonthArray!=NULL){
           rpt.SetByDay(*repeatDayOfMonthArray);
           repeatDayOfMonthArray->Close();
         }else{
           TInt week = 0;
           TRAP(error, {
             repeatDayOfMonthArray = new (ELeave) RArray<TCalRRule::TDayOfMonth>(1);
             CleanupClosePushL(*repeatDayOfMonthArray);
             week = (startTime.DayNoInMonth()/DAYS_IN_WEEK)+1;
             if(week==5){
               week = -1;
             }
             TCalRRule::TDayOfMonth dayOfMonth(startTime.DayNoInWeek(),week);
             repeatDayOfMonthArray->AppendL(dayOfMonth);
             rpt.SetByDay(*repeatDayOfMonthArray);
             CleanupStack::PopAndDestroy(repeatDateArray);
           });           
         }
         delete repeatDayOfMonthArray;
         if(error!=KErrNone){
           return SPyErr_SetFromSymbianOSErr(error);                 
         }
         ADD_REPEAT 
      }
    break;
    case YEARLY_BY_DATE_REPEAT:
      { 
        TCalRRule rpt(TCalRRule::EYearly);
        GET_REP_START_AND_END 
        SET_REPEAT_DATES
        ADD_REPEAT 
      }
    break;
    case YEARLY_BY_DAY_REPEAT:
      { 
        TCalRRule rpt(TCalRRule::EYearly);
        GET_REP_START_AND_END 
        SET_REPEAT_DATES
       
        // Add repeat day.
        if(repeatDays){           
          if(!PyDict_Check(repeatDays)){
            PyErr_SetString(PyExc_SyntaxError, 
              "yearly repeat day must be given in a dictionary");
            return NULL;
          }
          PyObject* weekKey = Py_BuildValue("s", (const char*)(&KWeek)->Ptr());
          PyObject* dayKey = Py_BuildValue("s", (const char*)(&KDay)->Ptr());
          PyObject* monthKey = Py_BuildValue("s", (const char*)(&KMonth)->Ptr());
          if(!(weekKey && dayKey && monthKey)){
            Py_XDECREF(weekKey);
            Py_XDECREF(dayKey);
            Py_XDECREF(monthKey);
            return NULL;
          }        
          PyObject* day = PyDict_GetItem(repeatDays, dayKey);
          PyObject* week = PyDict_GetItem(repeatDays, weekKey);
          PyObject* month = PyDict_GetItem(repeatDays, monthKey);
          Py_DECREF(weekKey);
          Py_DECREF(dayKey);
          Py_DECREF(monthKey);
        
          if(!day || !week || !month){
            PyErr_SetString(PyExc_SyntaxError, 
              "day, week and month must be given");
            return NULL;
          } 
        
          if(!(PyInt_Check(day) && PyInt_Check(week) && 
             PyInt_Check(month))){
            PyErr_SetString(PyExc_TypeError, 
                            "day, week and month must be integers");
            return NULL;
          } 
        
          TInt dayInt = PyInt_AsLong(day);
          TInt weekInt = PyInt_AsLong(week);
          TInt monthInt = PyInt_AsLong(month);
                 
          if(isWeekday(dayInt)==EFalse || 
             isWeekInMonth(weekInt)==EFalse ||
             isMonth(monthInt)==EFalse){
            PyErr_SetString(PyExc_ValueError, "bad value for day, week or month");
            return NULL;
          }
          weekInt++;
          if(weekInt==5){
            weekInt=-1; // because of >=3.0 SDK.
          }
          
          TCalRRule::TDayOfMonth dayOfMonth(static_cast<TDay>(dayInt),weekInt);
          RArray<TCalRRule::TDayOfMonth> arr;
          RArray<TMonth> monthArr;
          TRAP(error, {
            arr.AppendL(dayOfMonth);
            rpt.SetByDay(arr);
            monthArr.AppendL(static_cast<TMonth>(monthInt));
            rpt.SetByMonth(monthArr);
          });
          monthArr.Close();
          arr.Close();
          if(error!=KErrNone){
            return SPyErr_SetFromSymbianOSErr(error);
          };
        }         
        ADD_REPEAT 
      }
    break;
    default:
      PyErr_SetString(PyExc_SyntaxError, "illegal repeat definition");
      return NULL; 
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Parses and installs given repeat data into the entry.
 */
extern "C" PyObject *
Entry_set_repeat_data(Entry_object* self, PyObject* args)
{   
  TInt error = KErrNone;
  PyObject* retVal = NULL;
  PyObject* repeatDict = NULL;
  PyObject* key = NULL;
  PyObject* value = NULL;
  PyObject* repeatDays = NULL;
  TReal startDate = 0;
  TReal endDate = 0;
  TInt interval = 1;
  TInt repeatIndicator = -1;
  RArray<TCalTime>* exceptionArray = NULL;
  TBool eternalRepeat = EFalse;
  TBool timeSet = EFalse;
  
  
  if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &repeatDict)){ 
    return NULL;
  }
  
  TRAP(error, {
    TCalTime startTime = self->entry->StartTimeL();
    if(startTime.TimeUtcL() != Time::NullTTime()){
      timeSet = ETrue;
    };
  });     
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(timeSet == EFalse){
    PyErr_SetString(PyExc_RuntimeError, "entry's time not set");
    return NULL;
  };
      
  // Start date
  GET_VALUE_FROM_DICT(KStartDate, repeatDict)
  if(value){
    if(!PyFloat_Check(value)){
      PyErr_SetString(PyExc_TypeError, "start date must be float");
      return NULL; 
    }
    startDate = PyFloat_AsDouble(value);
  }
  
  // End date
  GET_VALUE_FROM_DICT(KEndDate, repeatDict)
  if(value){
    if(PyFloat_Check(value)){
      endDate = PyFloat_AsDouble(value);
    }else{
      if(value==Py_None){
        // None given as end date.
        eternalRepeat=ETrue;
      }else{
        PyErr_SetString(PyExc_TypeError, "end date must be float (or None)");
        return NULL; 
      }
    }
  }
  
  // Interval
  GET_VALUE_FROM_DICT(KInterval, repeatDict)
  if(value){
    if(!PyInt_Check(value) || PyInt_AsLong(value)<=0){
      PyErr_SetString(PyExc_TypeError, "interval must be int and >0");
      return NULL; 
    }
    interval = PyInt_AsLong(value);
  }
  
  // Repeat days
  GET_VALUE_FROM_DICT(KRepeatDays, repeatDict)
  repeatDays = value;
  
  // Repeat type
  GET_VALUE_FROM_DICT(KRepeatType, repeatDict)
  if(value){
    if(!PyString_Check(value)){
      PyErr_SetString(PyExc_TypeError, "repeat type must be string");
      return NULL; 
    }
    TPtrC8 valuePtr((TUint8*)PyString_AsString(value), PyString_Size(value));
    if(0 == valuePtr.Compare(KDaily)){
      repeatIndicator = DAILY_REPEAT;
    }else if(0 == valuePtr.Compare(KWeekly)){
      repeatIndicator = WEEKLY_REPEAT;
    }else if(0 == valuePtr.Compare(KMonthlyByDates)){
      repeatIndicator = MONTHLY_BY_DATES_REPEAT;
    }else if(0 == valuePtr.Compare(KMonthlyByDays)){
      repeatIndicator = MONTHLY_BY_DAYS_REPEAT;
    }else if(0 == valuePtr.Compare(KYearlyByDate)){
      repeatIndicator = YEARLY_BY_DATE_REPEAT;
    }else if(0 == valuePtr.Compare(KYearlyByDay)){
      repeatIndicator = YEARLY_BY_DAY_REPEAT;
    }else if(0 == valuePtr.Compare(KRepeatNone)){
      repeatIndicator = NOT_REPEATED;
    }else{
      PyErr_SetString(PyExc_ValueError, "illegal repeat type");
      return NULL; 
    }
  }  
     
  // Exceptions
  GET_VALUE_FROM_DICT(KExceptions, repeatDict)
  if(value){
    if(!PyList_Check(value)){
      PyErr_SetString(PyExc_SyntaxError, "exceptions must be given in a list");
      return NULL; 
    }
    if(PyList_Size(value)>0){
      TRAP(error, 
            exceptionArray = new (ELeave) RArray<TCalTime>(PyList_Size(value)));
      if(error!=KErrNone){
        return SPyErr_SetFromSymbianOSErr(error);
      }
    }
    for(TInt i=0;i<PyList_Size(value);i++){
      PyObject* listItem = PyList_GetItem(value, i);
      if(PyFloat_Check(listItem)){
        TCalTime calTime;
        TTime ttime;
        pythonRealUtcAsTTime(PyFloat_AsDouble(listItem),ttime);
        if(Check_time_validity(ttime) == EFalse){
          exceptionArray->Close();
          delete exceptionArray;
          PyErr_SetString(PyExc_ValueError, "illegal value for exception date");
          return NULL;
        }
        TRAP(error, {
          calTime.SetTimeUtcL(ttime);
          exceptionArray->AppendL(calTime);
        });
        if(error!=KErrNone){
          exceptionArray->Close();
          delete exceptionArray;
          return SPyErr_SetFromSymbianOSErr(error); 
        }
      }else{
        exceptionArray->Close();
        delete exceptionArray;
        PyErr_SetString(PyExc_TypeError, "exception dates must be floats");
        return NULL; 
      }
    }
  }
       
  retVal = Entry_install_repeat_data(self, startDate, endDate, interval, 
                                     repeatIndicator, exceptionArray, 
                                     repeatDays, eternalRepeat);

  if(exceptionArray!=NULL){
    exceptionArray->Close();                                                                     
    delete exceptionArray;  
  }
  
  return retVal;
}


/*
 * Returns entry's recurrence (="days") information.
 */
extern "C" PyObject *
Entry_recurrence_data(Entry_object* self, PyObject* repeatDataDict, TCalRRule& rpt)
{
  TInt error = KErrNone;
  PyObject* dayList = NULL;
  PyObject* key = NULL;
  PyObject* repeatType = NULL;
  
  key = Py_BuildValue("s", (const char*)(&KRepeatType)->Ptr());
  if(key==NULL){
    return NULL;
  }
  repeatType = PyDict_GetItem(repeatDataDict, key);
  Py_DECREF(key);
  TPtrC8 typePtr((TUint8*)PyString_AsString(repeatType), PyString_Size(repeatType));
  

  if(0 == typePtr.Compare(KWeekly)){ 
    RArray<TDay> days;
    TRAP(error, {
      rpt.GetByDayL(days);  
    });
    if(error!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(error); 
    }
    dayList = PyList_New(days.Count());
    CHECK_DAYLIST_CREATION
    for(TInt i=0;i<days.Count();i++){
      PyObject* dayNum = Py_BuildValue("i", days[i]);
      SET_ITEM_TO_DAYLIST 
    }
    days.Close();
  }else if(0 == typePtr.Compare(KMonthlyByDates)){
    RArray<TInt> days;
    TRAP(error, {
      rpt.GetByMonthDayL(days);
    });
    if(error!=KErrNone){
      days.Close();
      return SPyErr_SetFromSymbianOSErr(error); 
    }
    dayList = PyList_New(days.Count());
    CHECK_DAYLIST_CREATION
    for(TInt i=0;i<days.Count();i++){
      PyObject* dayNum = Py_BuildValue("i", days[i]);            
      SET_ITEM_TO_DAYLIST
    }
    days.Close();
  }else if(0 == typePtr.Compare(KMonthlyByDays)){
    RArray<TCalRRule::TDayOfMonth> days;
    TRAP(error, {
      rpt.GetByDayL(days);
    });
    if(error!=KErrNone){
      days.Close();
      return SPyErr_SetFromSymbianOSErr(error); 
    }
    dayList = PyList_New(days.Count());
    CHECK_DAYLIST_CREATION
    for(TInt i=0;i<days.Count();i++){
      TInt week = days[i].WeekInMonth();
      if(week == -1){
        week = 5; // compatible with old python api this way.
      }
      week--;
      PyObject* dayNum = Py_BuildValue("{s:i,s:i}", (const char*)(&KDay)->Ptr(), days[i].Day(), (const char*)(&KWeek)->Ptr(), week);   
      SET_ITEM_TO_DAYLIST
    }
    days.Close();
  }else if(0 == typePtr.Compare(KYearlyByDay)){
    RArray<TCalRRule::TDayOfMonth> day;
    RArray<TMonth> month;
    rpt.GetByMonthL(month);
    rpt.GetByDayL(day);
    if(month.Count()>0 && day.Count()>0){
      TInt week = day[0].WeekInMonth();
      if(week == -1){
        week = 5; // compatible with old python api this way.
      }
      week--;
      dayList = 
        Py_BuildValue("{s:i,s:i,s:i}", 
                      (const char*)(&KDay)->Ptr(), day[0].Day(), 
                      (const char*)(&KWeek)->Ptr(), week,
                      (const char*)(&KMonth)->Ptr(), month[0]); 
      day.Close();
      month.Close();
    }else{
      day.Close();
      month.Close();
      PyErr_SetString(PyExc_RuntimeError, "month and day information missing");
      return NULL;
    }
  }else {
    Py_INCREF(Py_None);
    return Py_None;
  }  

  return dayList;
}


/*
 * Returns entry's repeat information.
 */
extern "C" PyObject *
Entry_repeat_data(Entry_object* self, PyObject* /*args*/)
{ 
  TInt err = 0;
  PyObject* value = NULL;
  PyObject* key = NULL;
  TCalRRule rpt;
  RArray<TInt>* dateArr = NULL;
  RArray<TCalRRule::TDayOfMonth>* monthDayArr = NULL;
  RArray<TMonth>* monthArr = NULL;
  TTime endTime;
  RArray<TCalTime>* exceptionList = NULL;
    
  if(self->entry->GetRRuleL(rpt) == EFalse){
    // Not repeated.
    return Py_BuildValue("{s:s}", (const char*)(&KRepeatType)->Ptr(), 
                                  (const char*)(&KRepeatNone)->Ptr());    
  };

  PyObject* repeatDataDict = PyDict_New();
  if (repeatDataDict == NULL){
    return PyErr_NoMemory();
  }   

  // Repeat type
  key = Py_BuildValue("s", (const char*)(&KRepeatType)->Ptr());
  if(key == NULL){
    Py_DECREF(repeatDataDict);
    return NULL;
  }  
  
  switch(rpt.Type()){
    case TCalRRule::EDaily:
      value = Py_BuildValue("s", (const char*)(&KDaily)->Ptr());
    break;
    case TCalRRule::EWeekly:
      value = Py_BuildValue("s", (const char*)(&KWeekly)->Ptr());
    break;
    case TCalRRule::EMonthly:
      TRAP(err,{
        dateArr = new (ELeave) RArray<TInt>(5); 
        monthDayArr = new (ELeave) RArray<TCalRRule::TDayOfMonth>(5);
        rpt.GetByDayL(*monthDayArr);
        rpt.GetByMonthDayL(*dateArr);
        if(monthDayArr->Count() > 0){
          value = Py_BuildValue("s", (const char*)(&KMonthlyByDays)->Ptr());
        }else{
          value = Py_BuildValue("s", (const char*)(&KMonthlyByDates)->Ptr()); 
        }
      });
      if(dateArr!=NULL){
        dateArr->Close();
        delete dateArr;
      };
      if(monthDayArr!=NULL){
        monthDayArr->Close();
        delete monthDayArr;
      }
      if(err!=KErrNone){
        Py_DECREF(repeatDataDict);
        Py_DECREF(key);
        return SPyErr_SetFromSymbianOSErr(err); 
      }
    break;
    case TCalRRule::EYearly:
      TRAP(err, {
        monthDayArr = new (ELeave) RArray<TCalRRule::TDayOfMonth>(5);
        monthArr = new (ELeave) RArray<TMonth>(5);
        rpt.GetByMonthL(*monthArr);
        rpt.GetByDayL(*monthDayArr);
        if(monthDayArr->Count() > 0){
          value = Py_BuildValue("s", (const char*)(&KYearlyByDay)->Ptr());
        }else{
          value = Py_BuildValue("s", (const char*)(&KYearlyByDate)->Ptr());  
        }
      });
      if(monthDayArr!=NULL){
        monthDayArr->Close();
        delete monthDayArr;
      };
      if(monthArr!=NULL){
        monthArr->Close();
        delete monthArr;
      };
      if(err!=KErrNone){
        Py_DECREF(repeatDataDict);
        Py_DECREF(key);
        return SPyErr_SetFromSymbianOSErr(err);
      }
    break;
    default:
      value = Py_BuildValue("s", "unknown");
    break;
  }

  ADD_ITEM_TO_REP_DICT(key, value)
  
  // Start date.
  TRAP(err, {
    value = 
      Py_BuildValue("d", time_as_UTC_TReal(rpt.DtStart().TimeLocalL()));
  });
  if(err!=KErrNone){
    Py_DECREF(repeatDataDict);
    return SPyErr_SetFromSymbianOSErr(err);
  }
  key = Py_BuildValue("s", (const char*)(&KStartDate)->Ptr());
  ADD_ITEM_TO_REP_DICT(key, value)

  // End date.
  TRAP(err, {
    endTime = rpt.Until().TimeLocalL();
  });
  if(err!=KErrNone){
    Py_DECREF(repeatDataDict);
    return SPyErr_SetFromSymbianOSErr(err);
  }
  key = Py_BuildValue("s", (const char*)(&KEndDate)->Ptr());

  if(endTime == Time::NullTTime()){
    // Repeats forever. Set end date to None.
    Py_INCREF(Py_None);
    value = Py_None;
  }else{
    value = 
      Py_BuildValue("d", time_as_UTC_TReal(endTime));
  }
  ADD_ITEM_TO_REP_DICT(key, value)

  // Interval.
  key = Py_BuildValue("s", (const char*)(&KInterval)->Ptr());
  value = Py_BuildValue("i", rpt.Interval());
  ADD_ITEM_TO_REP_DICT(key, value)

  // Exceptions.
  TRAP(err, {
    exceptionList = new (ELeave) RArray<TCalTime>(5);
    self->entry->GetExceptionDatesL(*exceptionList);
  });
  if(err!=KErrNone){
    Py_DECREF(repeatDataDict);
    if(exceptionList!=NULL){
      exceptionList->Close();
    }
    delete exceptionList;
    return SPyErr_SetFromSymbianOSErr(err);    
  }
   
  if(exceptionList && exceptionList->Count()>0){
    PyObject* exceptionPyList = PyList_New(exceptionList->Count());
    if (exceptionPyList == NULL){
      exceptionList->Close();
      delete exceptionList;
      Py_DECREF(repeatDataDict);
      return PyErr_NoMemory();
    } 
    for(TInt i=0;i<exceptionList->Count();i++){
      const TCalTime& aException = (*exceptionList)[i];      
      PyObject* exceptionDate = NULL;
      TRAP(err, {
        exceptionDate = Py_BuildValue("d", time_as_UTC_TReal(aException.TimeLocalL()));
      });
      if(err!=KErrNone){
        Py_DECREF(repeatDataDict);
        exceptionList->Close();
        delete exceptionList;
        return SPyErr_SetFromSymbianOSErr(err); 
      }
      if (exceptionDate == NULL){
        Py_DECREF(repeatDataDict);
        Py_DECREF(exceptionPyList);
        exceptionList->Close();
        delete exceptionList;
        return NULL;
      } 
      if(PyList_SetItem(exceptionPyList, i, exceptionDate)){
        Py_DECREF(repeatDataDict);
        Py_DECREF(exceptionPyList);
        exceptionList->Close();
        delete exceptionList;
        return NULL;
      }      
    }
    exceptionList->Close();
    delete exceptionList;
    key = Py_BuildValue("s", (const char*)(&KExceptions)->Ptr());
    if(key==NULL){
      Py_DECREF(repeatDataDict);
      Py_DECREF(exceptionPyList);
      return NULL;
    }
    err = PyDict_SetItem(repeatDataDict, key, exceptionPyList);
    Py_DECREF(key);
    Py_DECREF(exceptionPyList);
    if(err){
      Py_DECREF(repeatDataDict);
      return NULL;
    }
  }else{
    if(exceptionList!=NULL){
      exceptionList->Close();
    }
    delete exceptionList;
    PyObject* exceptionPyList = PyList_New(0);
    if (exceptionPyList == NULL){
      Py_DECREF(repeatDataDict);
      return PyErr_NoMemory();
    } 
    key = Py_BuildValue("s", (const char*)(&KExceptions)->Ptr());
    if(key==NULL){
      Py_DECREF(repeatDataDict);
      Py_DECREF(exceptionPyList);
      return NULL;
    }
    err = PyDict_SetItem(repeatDataDict, key, exceptionPyList);
    Py_DECREF(key);
    Py_DECREF(exceptionPyList);
    if(err){
      Py_DECREF(repeatDataDict);
      return NULL;
    }
  }
  
  // Recurrence.
  PyObject* recurrenceData = Entry_recurrence_data(self,repeatDataDict,rpt);
  if(recurrenceData==NULL){
    Py_DECREF(repeatDataDict);
    return NULL;
  }

  if(recurrenceData != Py_None){
    key = Py_BuildValue("s", (const char*)(&KRepeatDays)->Ptr());
    if(key==NULL){
      Py_DECREF(recurrenceData);
      Py_DECREF(repeatDataDict);
      return NULL;
    }
    err = PyDict_SetItem(repeatDataDict, key, recurrenceData);
    Py_DECREF(key);
    Py_DECREF(recurrenceData);
    if(err){
      Py_DECREF(repeatDataDict);
      return NULL;
    }
  }else{
    Py_DECREF(recurrenceData);
  }

  return repeatDataDict;
}


/*
 * Deallocate the entry object.
 */
extern "C" {
  static void Entry_dealloc(Entry_object *entry)
  {
    delete entry->entry;
    entry->entry = NULL;
    Py_DECREF(entry->calendarDb); // must decrease the reference count, 
                                  // since it was increased when the entry was created.
    entry->calendarDb = NULL;
    PyObject_Del(entry);   
  }
}



/*
 * EntryIterator methods.
 */

 

/*
 * Creates new entry iterator object.
 */
extern "C" PyObject *
new_entryIterator(CalendarDb_object* self, PyObject* /*args*/)
{ 
  EntryIterator_object* ei =
    PyObject_New(EntryIterator_object, EntryIterator_type);
 
  if (ei == NULL){
    return PyErr_NoMemory();
  }

  ei->calendarDb = self;
  ei->ids = NULL;
  ei->index = 0;
  ei->Initialized = EFalse; 
  
  Py_INCREF(self); // increase the database's reference count so that it cannot be deleted
                   // until all the iterators have been deleted (otherwise a crash will follow).  
  
  return (PyObject*)ei;  
}


/*
 * Returns next entry in the database (entry can be of any type).
 */
extern "C" PyObject *
entryIterator_next(EntryIterator_object* self, PyObject* /*args*/)
{   
  TInt error = KErrNone;
  PyObject* ret = NULL;
  
  if(self->Initialized==EFalse){
    TCalTime calTime;
    TRAP(error, {
      calTime.SetTimeUtcL(calTime.MinTime());
      self->ids = new (ELeave) RArray<TCalLocalUid>(10);
      self->calendarDb->entryViewAdapter->iEntryView->GetIdsModifiedSinceDateL(calTime, *(self->ids));
    });
    if(error != KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);
    }
    if(self->ids->Count() == 0){
      PyErr_SetObject(PyExc_StopIteration, Py_None);
      return NULL;
    }
    self->index++;
    TCalLocalUid retId = (*(self->ids))[0];
    ret = Py_BuildValue("i", retId);   
    self->Initialized = ETrue;
  }else{
    if(self->ids->Count() <= self->index){
      PyErr_SetObject(PyExc_StopIteration, Py_None);
      return NULL;
    }
    TCalLocalUid retId = (*(self->ids))[self->index];
    ret = Py_BuildValue("i", retId); 
    self->index++;
  }
  return ret;
}


/*
 * Deallocates entry iterator object.
 */
extern "C" {
  static void EntryIterator_dealloc(EntryIterator_object *entryIterator)
  {
    if(entryIterator->ids != NULL){
      entryIterator->ids->Close();
      delete entryIterator->ids;
      entryIterator->ids = NULL;
    }
    Py_DECREF(entryIterator->calendarDb); // must decrease the reference count, 
                                          // since it was increased when the iterator was created.
    entryIterator->calendarDb = NULL;
    PyObject_Del(entryIterator);
  }
}


/*
 * Returns next entry in the database (entry can be of any type).
 */
extern "C" PyObject *
trunc_test(EntryIterator_object* self, PyObject* args)
{
  TReal taimi;
  TTime ttime;
  
  if (!PyArg_ParseTuple(args, "d", &taimi)){ 
    return NULL;
  }

  pythonRealAsTTime(taimi, ttime);   
 
  RTz tz;
  tz.Connect();
  tz.ConvertToLocalTime(ttime);
  ttime = truncToDate(ttime);
  tz.ConvertToUniversalTime(ttime);
  tz.Close();
 /* tz.ConvertToLocalTime(ttime);
  ttime = truncToDate(ttime);
  tz.ConvertToUniversalTime(ttime);*/
  return Py_BuildValue("d", time_as_UTC_TReal(ttime));

}


 
//////////////TYPE SET



extern "C" {

  const static PyMethodDef CalendarDb_methods[] = {
    {"add_entry", (PyCFunction)CalendarDb_add_entry, METH_VARARGS},
    {"monthly_instances", (PyCFunction)CalendarDb_monthly_instances, METH_VARARGS},
    {"daily_instances", (PyCFunction)CalendarDb_daily_instances, METH_VARARGS},
    {"get_entry", (PyCFunction)CalendarDb_getitem, METH_VARARGS},
    {"export_vcals", (PyCFunction)CalendarDb_export_vcals, METH_VARARGS},
    {"import_vcals", (PyCFunction)CalendarDb_import_vcals, METH_VARARGS},
    {"find_instances", (PyCFunction)CalendarDb_find_instances, METH_VARARGS},
    {"delete_entry", (PyCFunction)CalendarDb_delete_entry, METH_VARARGS},
    {"entry_count", (PyCFunction)CalendarDb_entry_count, METH_NOARGS},
    {"trunc_test", (PyCFunction)trunc_test, METH_VARARGS},
    {NULL, NULL}  
  };  

  const static PyMethodDef EntryIterator_methods[] = {
    {NULL, NULL}  
  };

  const static PyMethodDef Entry_methods[] = {
    {"content", (PyCFunction)Entry_content, METH_NOARGS},
    {"set_content", (PyCFunction)Entry_set_content, METH_VARARGS},
   	{"description", (PyCFunction)Entry_description, METH_NOARGS},
    {"set_description", (PyCFunction)Entry_set_description, METH_VARARGS},
    {"type", (PyCFunction)Entry_entry_type, METH_NOARGS},
    {"commit", (PyCFunction)Entry_commit, METH_NOARGS},
    {"set_repeat_data", (PyCFunction)Entry_set_repeat_data, METH_VARARGS},
    {"repeat_data", (PyCFunction)Entry_repeat_data, METH_NOARGS},
    {"location", (PyCFunction)Entry_location, METH_NOARGS},
    {"set_location", (PyCFunction)Entry_set_location, METH_VARARGS},
    {"unique_id", (PyCFunction)Entry_unique_id, METH_NOARGS},
    {"start_datetime", (PyCFunction)Entry_start_datetime, METH_NOARGS},
    {"end_datetime", (PyCFunction)Entry_end_datetime, METH_NOARGS},
    {"set_start_and_end_datetime", (PyCFunction)Entry_set_start_and_end_datetime, METH_VARARGS},
    {"set_priority", (PyCFunction)Entry_set_priority, METH_VARARGS},
    {"priority", (PyCFunction)Entry_priority, METH_NOARGS},
    {"last_modified", (PyCFunction)Entry_last_modified, METH_NOARGS},
    {"set_replication", (PyCFunction)Entry_set_replication, METH_VARARGS},
    {"replication", (PyCFunction)Entry_replication, METH_NOARGS},
    {"set_crossed_out", (PyCFunction)Entry_set_crossed_out, METH_VARARGS},
    {"crossed_out_date", (PyCFunction)Entry_crossed_out_date, METH_NOARGS},
    {"set_alarm", (PyCFunction)Entry_set_alarm, METH_VARARGS},
    {"alarm_datetime", (PyCFunction)Entry_alarm_datetime, METH_NOARGS},
    {"cancel_alarm", (PyCFunction)Entry_cancel_alarm, METH_NOARGS},
    {"originating_entry", (PyCFunction)Entry_originating_entry, METH_NOARGS},
    {NULL, NULL}  
  };


  static PyObject *
  CalendarDb_getattr(CalendarDb_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)CalendarDb_methods, (PyObject *)op, name);
  }

  static PyObject *
  EntryIterator_getattr(EntryIterator_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)EntryIterator_methods, (PyObject *)op, name);
  }

  static PyObject *
  Entry_getattr(Entry_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)Entry_methods, (PyObject *)op, name);
  }

    
  static const PyTypeObject c_CalendarDb_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_calendar.CalendarDb",                   /*tp_name*/
    sizeof(CalendarDb_object),                /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)CalendarDb_dealloc,           /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)CalendarDb_getattr,          /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0, //&calendarDb_as_mapping,              /*tp_as_mapping*/
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
    (getiterfunc)new_entryIterator,           /*tp_iter */
  };

  static const PyTypeObject c_EntryIterator_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_calendar.EntryIterator",                /*tp_name*/
    sizeof(EntryIterator_object),             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)EntryIterator_dealloc,        /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)EntryIterator_getattr,       /*tp_getattr*/
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
    (iternextfunc)entryIterator_next,         /*tp_iternext */
  };

  static const PyTypeObject c_Entry_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_calendar.Entry",                        /*tp_name*/
    sizeof(Entry_object),                     /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    /* methods */
    (destructor)Entry_dealloc,                /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)Entry_getattr,               /*tp_getattr*/
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
   
} //extern C


//////////////INIT//////////////


extern "C" {
  static const PyMethodDef calendar_methods[] = {
    {"open", (PyCFunction)open_db, METH_VARARGS, NULL},
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initcalendar(void)
  {
    PyTypeObject* calendar_db_type = PyObject_New(PyTypeObject, &PyType_Type);
    *calendar_db_type = c_CalendarDb_type;
    calendar_db_type->ob_type = &PyType_Type;
    SPyAddGlobalString("CalendarDbType", (PyObject*)calendar_db_type);    

    PyTypeObject* entry_type = PyObject_New(PyTypeObject, &PyType_Type);
    *entry_type = c_Entry_type;
    entry_type->ob_type = &PyType_Type;
    SPyAddGlobalString("EntryType", (PyObject*)entry_type);    

    PyTypeObject* entry_iterator_type = PyObject_New(PyTypeObject, &PyType_Type);
    *entry_iterator_type = c_EntryIterator_type;
    entry_iterator_type->ob_type = &PyType_Type;
    SPyAddGlobalString("EntryIteratorType", (PyObject*)entry_iterator_type);    

    PyObject *m, *d;

    m = Py_InitModule("_calendar", (PyMethodDef*)calendar_methods);
    d = PyModule_GetDict(m);

    // Replication statuses.
    PyDict_SetItemString(d,"rep_open", PyInt_FromLong(CCalEntry::EOpen));
    PyDict_SetItemString(d,"rep_private", PyInt_FromLong(CCalEntry::EPrivate));
    PyDict_SetItemString(d,"rep_restricted", PyInt_FromLong(CCalEntry::ERestricted));  
    
    // Entry types. 
    PyDict_SetItemString(d,"entry_type_appt", PyInt_FromLong(CCalEntry::EAppt));
    PyDict_SetItemString(d,"entry_type_todo", PyInt_FromLong(CCalEntry::ETodo));
    PyDict_SetItemString(d,"entry_type_event", PyInt_FromLong(CCalEntry::EEvent));
    PyDict_SetItemString(d,"entry_type_anniv", PyInt_FromLong(CCalEntry::EAnniv));
    PyDict_SetItemString(d,"entry_type_reminder", PyInt_FromLong(CCalEntry::EReminder));
    
    // Filters.
    PyDict_SetItemString(d,"appts_inc_filter", PyInt_FromLong(CalCommon::EIncludeAppts));
    PyDict_SetItemString(d,"events_inc_filter", PyInt_FromLong(CalCommon::EIncludeEvents));
    PyDict_SetItemString(d,"annivs_inc_filter", PyInt_FromLong(CalCommon::EIncludeAnnivs));
    PyDict_SetItemString(d,"reminders_inc_filter", PyInt_FromLong(CalCommon::EIncludeReminder));
    PyDict_SetItemString(d,"todos_inc_filter", PyInt_FromLong(CalCommon::EIncludeCompletedTodos | CalCommon::EIncludeIncompletedTodos));
    

    return;
  }
} /* extern "C" */



