/**
 * ====================================================================
 * logengine.cpp
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
//
#include "logengine.h"
//
//
//
CLogEngine::CLogEngine(CEventArray& aEventArray) : CActive(CActive::EPriorityStandard), iEventArray(aEventArray)
	{
	//
	}
void CLogEngine::ConstructL(TInt aDirection, TUid aType)
	{
	//
	//Connect to the file server.
	User::LeaveIfError(iFs.Connect());
	//Check if Log Engine available - 
	//NOTE: potentially dangerous if CLogWrapper closes the RFs?
	CLogWrapper* iLogWrapper = CLogWrapper::NewL(iFs, 0);
	if (!iLogWrapper->ClientAvailable())
	{
		//Get out... as the Log API is not supported.
		User::Leave(KErrNotSupported); 
	}
	delete iLogWrapper;
	//Setup LOG elements.
	iLogClient = CLogClient::NewL(iFs);
	iLogView = CLogViewEvent::NewL(*iLogClient);
	iLogFilter = CLogFilter::NewL();
	//Setup filter (call logs handled by default).
	SetLogFilter(aDirection, aType);
	//Create active scheduler waiting.
	iWait = new (ELeave) CActiveSchedulerWait;
	//Add to the active scheduler
	CActiveScheduler::Add(this);
	}
CLogEngine* CLogEngine::NewL(CEventArray& aEventArray, TInt aDirection, TUid aType)
	{
	//
	CLogEngine* self = CLogEngine::NewLC(aEventArray, aDirection, aType);
	CleanupStack::Pop(self);
	return self;
	}
CLogEngine* CLogEngine::NewLC(CEventArray& aEventArray, TInt aDirection, TUid aType)
	{
	//
	CLogEngine* self = new (ELeave) CLogEngine(aEventArray);
	CleanupStack::PushL(self);
	self->ConstructL(aDirection, aType);
	return self;
	}
CLogEngine::~CLogEngine()
	{
	//
	//Cancel all outstanding active object calls.
	Cancel();
	//
	if (iWait)		{ delete iWait; }
	if (iLogView)	{ delete iLogView; }
	if (iLogClient)	{ delete iLogClient; }
	if (iLogFilter)	{ delete iLogFilter; }
	
	iFs.Close();
	}
void CLogEngine::SetLogFilter(TInt aDirection, TUid aType)
	{
	// Find from the log engine, the string for the given direction resource
	TBuf<64> direction;
	iLogClient->GetString(direction, aDirection);
	// Set the direction to be filtered 
	iLogFilter->SetDirection(direction);
	iLogFilter->SetEventType(aType);
	}
void CLogEngine::StartL()
	{
	//
	if (iLogView->SetFilterL(*iLogFilter, iStatus))
		{
		// if there are events in the filtered view
		// issue the asynchronous request
		SetActive();
		//Wait for the last log entry.
		iWait->Start();
		}
	//Change state.
	iEngineState = ECreate;
	}
void CLogEngine::DoCancel()
	{
	//Stop waiting for the result.
	iWait->AsyncStop();
	}
void CLogEngine::RunL()
	{
	//
	if (iStatus != KErrNone)
		{
		// if there has been a problem inform the observer.
		iWait->AsyncStop();
		return;
		}
	//
	switch (iEngineState)
		{
		case ECreate:
			{
			// The filtered view has been successfully created
			// so issue a request to go to the first event
			iLogView->FirstL(iStatus);
			if (iLogView->CountL() > 0)
				{
				iEngineState = EAddEntry;
				SetActive();
				}
			else 
				{
				//Done as the initially logs were cleared...
				Cancel();
				iWait->AsyncStop();
				}
			break;
			}
		case EAddEntry:
			{
			// Add the current event to the array of events
			const CLogEvent& event = iLogView->Event();
			iEventArray.AddEntryL(event);
			// Are there any more entries
			if (iLogView->NextL(iStatus))
				{
				// if there is another entry, issue the request
				// to move to the next entry.
				SetActive();
				}
			else
				{
				//
				Cancel();
				iWait->AsyncStop();
				}
			break;
			}
		default:
			break;
		}
	}
TInt CLogEngine::RunError(TInt aError)
	{
	//
	iWait->AsyncStop();
	return aError;
	}
