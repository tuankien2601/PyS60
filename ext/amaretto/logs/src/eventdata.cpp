/**
 * ====================================================================
 * eventdata.cpp
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

#include "eventdata.h"


CEventData::CEventData()
	: CBase(), iId(-1), iContactId(-1), iDuration(0), iDurationType(-1), iFlags(0), iLink(0)
{
	iNumber = NULL;
	iData = NULL;
}


void CEventData::ConstructL()
{
	iNumber = HBufC::NewLC(0);
	iData = HBufC8::NewL(0);
	
	CleanupStack::Pop(iNumber);
}


CEventData *CEventData::NewL()
{
	CEventData *self = NewLC();
	CleanupStack::Pop(self);
	
	return self;
}


CEventData *CEventData::NewLC()
{
	CEventData *self = new (ELeave) CEventData();

	CleanupStack::PushL(self);
	self->ConstructL();
	
	return self;
}


CEventData::~CEventData()
{
	if (iData)
	{
		delete iData;
	}
	if (iNumber)
	{
		delete iNumber;
	}
}



void CEventData::SetNumberL(const TDesC& aNumber)
{
	// allocate enough memory for the new number
	HBufC *newNumber = iNumber->ReAllocL(aNumber.Length());
	
	User::LeaveIfNull(newNumber); // in case memory allocation failed
	
	// set the newly allocated HBufC. (Note that the old object was freed.)
	iNumber = newNumber;
	
	// now that there's enough memory in HBufC, set the new contents
	*iNumber = aNumber;
}


const TDesC *CEventData::GetNumber() const
{
	if (!iNumber)
		return NULL;
	
	// The following line does not look nice, but be aware
	// that the * operator is overloaded to return TDesC .
	// Therefore it is _not_ sufficient to write: return iNumber
	
	return &(*iNumber);
}

	
void CEventData::SetDataL(const TDesC8& aData)
{
	// allocate enough memory for the new data
	HBufC8 *newData = iData->ReAllocL(aData.Length());
	
	User::LeaveIfNull(newData); // in case memory allocation failed
	
	// set the newly allocated HBufC. (Note that the old object was freed.)
	iData = newData;

	// now that there's enough memory in HBufC, set the new contents
	*iData = aData;	
}


const TDesC8 *CEventData::GetData() const
{
	if (!iData)
		return NULL;
	
	// The following line does not look nice, but be aware
	// that the * operator is overloaded to return TDesC8 .
	// Therefore it is _not_ sufficient to write: return iData
	
	return &(*iData);
}
