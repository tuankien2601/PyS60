/*
 * ====================================================================
 *  posdatafetcher.cpp
 *
 *  Python API to Series 60 Location Acquisition API.
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

#include "locationacqmodule.h"

/*
 * Convert epoch to TReal.
 */
static TReal epochToReal()
{
  _LIT(KEpoch, "19700000:");
  TTime epochTime;
  epochTime.Set(KEpoch);
  #ifndef EKA2
  return epochTime.Int64().GetTReal();
  #else
  return TReal64(epochTime.Int64());
  #endif
}

/*
 * TTime to python real. 
 */
static TReal timeToReal(const TTime& aTime)
{
  TLocale locale;
  #ifndef EKA2
  return (((aTime.Int64().GetTReal()-epochToReal())/
          (1000.0*1000.0))-
          TInt64(locale.UniversalTimeOffset().Int()).GetTReal());
  #else
  return (((TReal64(aTime.Int64())-epochToReal())/(1000.0*1000.0)) 
        - TInt64(TReal64(locale.UniversalTimeOffset().Int())));
  #endif
}

// A helper function for the implementation of callbacks
// from C/C++ code to Python callables (modified from appuifwmodule.cpp)
TInt TPyPosCallBack::PositionUpdated(TPositionInfo* aPositionInfo, TInt aFlags)
  {
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject* pyPosition=NULL;
  PyObject* pyCourse=NULL;
  PyObject* pySatellites=NULL;
  TInt error = KErrNone;

  // get the position information.
  TPosition position; 
  aPositionInfo->GetPosition(position); 
  pyPosition = Py_BuildValue("{s:d,s:d,s:d,s:d,s:d}",
                             "latitude", position.Latitude(),
                             "longitude", position.Longitude(),
                             "altitude", position.Altitude(),
                             "vertical_accuracy", position.VerticalAccuracy(),
                             "horizontal_accuracy", position.HorizontalAccuracy()); 
  // XXX
  if(pyPosition==NULL){
    Py_INCREF(Py_None);
    pyPosition = Py_None;
  }
                          
  if(aFlags & COURSE_INFO){
    TCourse course;
    static_cast<TPositionCourseInfo*>(aPositionInfo)->GetCourse(course);
    pyCourse = Py_BuildValue("{s:d,s:d,s:d,s:d}",
                             "speed",course.Speed(),
                             "heading",course.Heading(),
                             //"course",course.Course(), ??? // 2.6 does not support
                             "speed_accuracy",course.SpeedAccuracy(),
                             "heading_accuracy",course.HeadingAccuracy()
                             //"course_accuracy",course.CourseAccuracy()  ??? // 2.6 does not support
                             );  
    if(pyCourse==NULL){
      Py_INCREF(Py_None);
      pyCourse = Py_None;
    }   
  }else{
    Py_INCREF(Py_None);
    pyCourse = Py_None;
  }
   
  if(aFlags & SATELLITE_INFO){
    TPositionSatelliteInfo* satInfo = static_cast<TPositionSatelliteInfo*>(aPositionInfo);
    pySatellites = Py_BuildValue("{s:i,s:i,s:d,s:d,s:d,s:d}",
                                 "satellites", satInfo->NumSatellitesInView(),
                                 "used_satellites", satInfo->NumSatellitesUsed(),
                                 "time",timeToReal(satInfo->SatelliteTime()),
                                 "horizontal_dop",satInfo->HorizontalDoP(), 
                                 "vertical_dop",satInfo->VerticalDoP(),
                                 "time_dop",satInfo->TimeDoP()); 
                                 
    if(pySatellites==NULL){
      Py_INCREF(Py_None);
      pySatellites = Py_None;
    }
  }else{
    Py_INCREF(Py_None);
    pySatellites = Py_None;
  } 
  
  PyObject* arg = Py_BuildValue("({s:N,s:N,s:N})",
                       "position", pyPosition,
                       "course", pyCourse,
                       "satellites", pySatellites);
  PyObject* rval = PyEval_CallObject(iCb, arg);
  
  Py_DECREF(arg);
  if (!rval) {
    error = KErrPython;
    if (PyErr_Occurred() == PyExc_OSError) {
      PyObject *type, *value, *traceback;
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

  PyGILState_Release(gstate);
  return error;
  }

CPosDataFetcher* CPosDataFetcher::NewL(RPositioner& aPositioner)
{
  CPosDataFetcher* self=new (ELeave) CPosDataFetcher(aPositioner);
  CleanupStack::PushL(self);
  self->ConstructL();
  CleanupStack::Pop(self);
  return self;
};
	
void CPosDataFetcher::ConstructL()
{
  iCallBackSet = EFalse;
  iFlags = 0;
  CActiveScheduler::Add(this);
};

CPosDataFetcher::CPosDataFetcher(RPositioner& aPositioner):CActive(CActive::EPriorityStandard),
                                                          iPositioner(aPositioner)
{
};

CPosDataFetcher::~CPosDataFetcher()
{
  Cancel();
  delete iPositionInfo;
};

void CPosDataFetcher::DoCancel()
{
  this->iPositioner.CancelRequest(EPositionerNotifyPositionUpdate);
};

void CPosDataFetcher::RunL()
{   
  this->iCallMe.PositionUpdated(iPositionInfo, iFlags);
  // re-issue the position request query
  this->iPositioner.NotifyPositionUpdate(*iPositionInfo, iStatus);
  this->SetActive();
};

void CPosDataFetcher::FetchData(TRequestStatus& aStatus, 
                                TTimeIntervalMicroSeconds aInterval,
                                TBool aPartial)
{
  // start the position feed
  this->iPositioner.SetUpdateOptions(TPositionUpdateOptions(aInterval, 
                                                            TTimeIntervalMicroSeconds(0), /* timeout */
                                                            TTimeIntervalMicroSeconds(0), /* max age */
                                                            aPartial));
  this->iPositioner.NotifyPositionUpdate(*iPositionInfo, iStatus);
  this->SetActive();
  aStatus=this->iStatus;
}

TInt CPosDataFetcher::CreatePositionType(TInt aFlags)
{
  iFlags = aFlags;
  TRAPD(err,{
    		if(aFlags & SATELLITE_INFO)
    		{
      			this->iPositionInfo = new (ELeave) TPositionSatelliteInfo;
    		}
    		else if(aFlags & COURSE_INFO)
    		{
      			this->iPositionInfo = new (ELeave) TPositionCourseInfo;
    		}
    		else
    		{
      			this->iPositionInfo = new (ELeave) TPositionInfo;
    		}
  			}
  	)
  return err;
};

void CPosDataFetcher::SetCallBack(TPyPosCallBack &aCb) 
  {
  iCallMe = aCb;
  iCallBackSet = ETrue;
  }
