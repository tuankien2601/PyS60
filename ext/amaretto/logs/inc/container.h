/**
 * ====================================================================
 * container.h
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
 
#ifndef __CCONTAINER
#define __CCONTAINER

#include <e32base.h>
#include <e32std.h>
#include "logengine.h"
#include "eventarray.h"
#include "eventdata.h"

#ifndef EKA2
class CLogContainer : public CBase
#else
NONSHARABLE_CLASS (CLogContainer) : public CBase
#endif
{
public:
	static CLogContainer* NewL(TInt /*aDirection*/, TUid /*aType*/);
	virtual ~CLogContainer();
	void StartL();
	TInt Count();
	CEventData& Get(TInt /*aIndex*/) const;
private:
	void ConstructL(TInt /*aDirection*/, TUid /*aType*/);
	CLogContainer();
private:
	CLogEngine*		iLog;
	CEventArray*	iEventArray;
};

#endif
