/**
 * ====================================================================
 * eventarray.cpp
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

#include "eventarray.h"


CEventArray::CEventArray()
	: CBase(), iArray()
	{
	}

CEventArray::CEventArray(TInt aGranularity)
	: CBase(), iArray(aGranularity)
	{
	}

CEventArray::~CEventArray()
	{
	iArray.ResetAndDestroy();
	iArray.Close();
	}


void CEventArray::AddEntryL(const CLogEvent& aEvent)
	{
	CEventData *eventData = CEventData::NewLC();
	//Simple data:
	eventData->iId = aEvent.Id();
	eventData->iContactId = aEvent.Contact();
	TTimeIntervalSeconds secs = aEvent.Duration();
	eventData->iDuration = secs.Int();
	eventData->iDurationType = aEvent.DurationType();
	eventData->iFlags = aEvent.Flags();
	eventData->iLink = aEvent.Link();

	eventData->SetNumberL(aEvent.Number());
	eventData->SetDataL(aEvent.Data());
	
	//These can be copied 1:1 as the max sizes are equal.
	eventData->iDescription = aEvent.Description();
	eventData->iDirection = aEvent.Direction();
	eventData->iStatus = aEvent.Status();
	eventData->iSubject = aEvent.Subject();
	eventData->iTime = aEvent.Time();

	iArray.Append(eventData);
	
	CleanupStack::Pop(eventData);
	}


TInt CEventArray::Count() const
{
	return iArray.Count();
}

CEventData& CEventArray::operator[](TInt anIndex)
{
	return *(iArray[anIndex]);
}


// End of File


