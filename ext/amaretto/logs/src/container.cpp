/**
 * ====================================================================
 * container.cpp
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

#include "container.h"

//Construction.
CLogContainer* CLogContainer::NewL(TInt aDirection, TUid aType)
	{
	CLogContainer* self = new (ELeave) CLogContainer;
	CleanupStack::PushL(self);
	self->ConstructL(aDirection, aType);
	CleanupStack::Pop(self);
	return self;
	}
void CLogContainer::ConstructL(TInt aDirection, TUid aType)
	{
	//
	iEventArray = new (ELeave) CEventArray(1);
	iLog = CLogEngine::NewL(*iEventArray, aDirection, aType);
	}
CLogContainer::CLogContainer()
	{
	//
	}


//Destructor
CLogContainer::~CLogContainer()
	{
	//Release reserved memory
	if (iLog)					
		{
		delete iLog;
		}
	if (iEventArray)	
		{
		delete iEventArray;
		}
  	}

//Operational methods. Pass the commands to the engine...
void CLogContainer::StartL()
	{
	iLog->StartL();
	}
TInt CLogContainer::Count()
	{
	return iEventArray->Count();
	}
//CLogEvent& CLogContainer::Get(TInt aIndex) const
CEventData& CLogContainer::Get(TInt aIndex) const
	{
	if (aIndex < 0 || aIndex > iEventArray->Count())
		{
		User::Panic(_L("CLogContainer Get method called with invalid index!"), KErrArgument);
		}
	//
	return (*iEventArray)[aIndex];
	}
