/**
 * ====================================================================
 * eventdata.h
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

#ifndef __EVENTDATA_H
#define __EVENTDATA_H

#include <cntdef.h>
#include <logwrap.h>

#ifndef EKA2
class CEventData : public CBase
#else
NONSHARABLE_CLASS (CEventData) : public CBase
#endif
{
protected:
	CEventData();

	void ConstructL();


public:
	static CEventData *NewL();
	static CEventData *NewLC();

	~CEventData();

	void SetNumberL(const TDesC& aNumber);
	const TDesC *GetNumber() const;
	
	void SetDataL(const TDesC8& aData);
	const TDesC8 *GetData() const;


protected:
	HBufC*	iNumber;	// large maximum length, (probably) therefore heap allocated
	HBufC8*	iData;		// no given maximum length, therefore heap allocated
	

public: 
	//Public members for easy assignment.
	TBuf<KLogMaxRemotePartyLength>	iName;
	TBuf<KLogMaxDescriptionLength>	iDescription;
	TBuf<KLogMaxDirectionLength>	iDirection;
	TBuf<KLogMaxStatusLength>		iStatus;
	TBuf<KLogMaxSubjectLength>		iSubject;
	//
	TLogId							iId;
	TContactItemId					iContactId;
	TLogDuration					iDuration;
	TLogDurationType				iDurationType;
	TLogFlags						iFlags;
	TLogLink						iLink;
	TTime							iTime;
};
#endif
