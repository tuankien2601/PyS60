/**
 * ====================================================================
 * logengine.h
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

#ifndef __LOGENGINE
#define __LOGENGINE

#include <e32base.h>
#include <logcli.h>
#include <logwrap.h>
#include <logview.h>
#include "eventarray.h"

#ifndef EKA2
class CLogEngine : public CActive
#else
NONSHARABLE_CLASS (CLogEngine) : public CActive
#endif
{
public:
	static CLogEngine* NewL(CEventArray& /*aEventArray*/, TInt /*aDirection*/, TUid /*aType*/);
	static CLogEngine* NewLC(CEventArray& /*aEventArray*/, TInt /*aDirection*/, TUid /*aType*/);
	virtual ~CLogEngine();
	void StartL();
	void SetLogFilter(TInt /*aDirection*/, TUid /*aType*/);
public:
	void DoCancel();
	void RunL();
	TInt RunError(TInt /*aError*/);
private:
	CLogEngine(CEventArray& /*aEventArray*/);
	void ConstructL(TInt /*aDirection*/, TUid /*aType*/);
private:
	//Creates
	CLogClient*			iLogClient; 
	CLogViewEvent*	iLogView;
	CLogFilter*			iLogFilter;
	CActiveSchedulerWait* iWait;
	//Connects
	RFs	iFs;
	//Has reference
	CEventArray&	iEventArray;
private:
	enum TEngineState
		{
		ECreate,
		EAddEntry
	};
	//
	TEngineState	iEngineState;
};
#endif


