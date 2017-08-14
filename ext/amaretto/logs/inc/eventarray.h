/**
 * ====================================================================
 * eventarray.h
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

#ifndef __CEVENTARRAY
#define __CEVENTARRAY

// System includes
#include <logwrap.h>
#include <e32std.h>
#include "eventdata.h"



//class CEventArray : public CArrayFixFlat<TEventData>

#ifndef EKA2
class CEventArray : public CBase
#else
NONSHARABLE_CLASS (CEventArray) : public CBase
#endif
{
private:
	RPointerArray<CEventData> iArray;
	
public:
	CEventArray();
	CEventArray(TInt /*aGranularity*/);
	virtual ~CEventArray();
	
public:
	void AddEntryL(const CLogEvent& /*aEvent*/);
	
	TInt Count() const;
	
	CEventData& operator[](TInt anIndex);
};

#endif
