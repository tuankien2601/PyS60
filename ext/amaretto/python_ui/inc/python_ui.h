/* Copyright (c) 2008-2009 Nokia Corporation
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
 */

#ifndef _APPUISTUB_H
#define _APPUISTUB_H

#include <aknapp.h>
#include <eikstart.h>
#include <AknDoc.h>
#include <aknglobalnote.h>
#include <eiknotapi.h>
#include <e32base.h>
#include <e32std.h>

class CPythonDocument : public CAknDocument
{
public:
	CPythonDocument(CEikApplication& aApp);
	~CPythonDocument();
private:
	CEikAppUi* CreateAppUiL();
};

class CPythonApplication : public CAknApplication
{
private:
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
};

#endif
