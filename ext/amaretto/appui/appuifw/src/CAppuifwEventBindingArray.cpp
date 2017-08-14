/*
* Copyright (c) 2005-2009 Nokia Corporation
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
*  CAppuifwEventBindingArray.cpp
*
*  class CAppuifwEventBindingArray & related utilities
* ====================================================================
*/

#include "CAppuifwEventBindingArray.h"

static TBool IsBoundTo(const TKeyEvent& aEv1, const TKeyEvent& aEv2)
{
  return (aEv1.iModifiers ?
          ((aEv2.iModifiers & aEv1.iModifiers) &&
           (aEv2.iCode == aEv1.iCode)) :
          (aEv2.iCode == aEv1.iCode));
}

static TBool CheckEventBinding(const SAmarettoEventInfo& aSavedEvent, const SAmarettoEventInfo& aCurrentEvent)
{
  // Return True if pointer type matches and if the Event binding does not specify a rect or 
  // the pointer event is within the specified rect.
  return (aSavedEvent.iPointerEvent.iType == aCurrentEvent.iPointerEvent.iType) ?
	      ((aSavedEvent.iPointerRect.IsEmpty()) || 
	      (aSavedEvent.iPointerRect.Contains(aCurrentEvent.iPointerEvent.iPosition))) : EFalse;
}

EXPORT_C CAppuifwEventBindingArray::~CAppuifwEventBindingArray()
{
  TInt i = 0;

  while (i < iKey.Count()) {
    Py_XDECREF(iKey[i].iCb);
    i++;
  } 
}

void CAppuifwEventBindingArray::ClearEventBinding(const SAppuifwEventBinding& aBindInfo)
{
  TInt i = 0;

  while (i < iKey.Count()) {
    if (iKey[i].iPointerEvent.iType == aBindInfo.iPointerEvent.iType) {
  	  //Delete all bindings with this event type
  	  if (aBindInfo.iPointerRect.IsEmpty()) {
        Py_XDECREF(iKey[i].iCb);
        iKey.Delete(i);
        continue;
  	  }
  	  // Delete only if the co-ordinates passed with bind matches
  	  else if (iKey[i].iPointerRect == aBindInfo.iPointerRect) {
  		Py_XDECREF(iKey[i].iCb);
  		iKey.Delete(i);
        break;
  	  }
    }
	i++;
  }
}

EXPORT_C void CAppuifwEventBindingArray::InsertEventBindingL(SAppuifwEventBinding& aBindInfo)
{
  TInt i = 0;
  TBool bindingExists = EFalse;
  switch (aBindInfo.iType) {
  case SAmarettoEventInfo::EKey:
    while (i < iKey.Count()) {
      if (aBindInfo.iKeyEvent.iCode < iKey[i].iKeyEvent.iCode)
        break;
      if (IsBoundTo(aBindInfo.iKeyEvent, iKey[i].iKeyEvent)) {
        Py_XDECREF(iKey[i].iCb);
        iKey.Delete(i);
        if (aBindInfo.iCb == NULL)
          return;
        break;
      }
      i++;
    }
    iKey.InsertL(i, aBindInfo);
    break;
  case SAmarettoEventInfo::EPointer:
    while (i < iKey.Count()) {
      if (aBindInfo.iPointerEvent.iType == iKey[i].iPointerEvent.iType) {
        bindingExists = ETrue;
		if (aBindInfo.iCb == NULL) {
		  //Clear all the binding with this Pointer type
		  ClearEventBinding(aBindInfo);
		  break;
		}
		else {
		  // Different callback was passed. If there is an existing
		  // binding for the same rect delete it. Insert this new one
		  // anyways at the end.
		  ClearEventBinding(aBindInfo);
		  bindingExists = EFalse;
		  i = iKey.Count();
		  break;
	    }
      }
    i++;
    }
    if (!bindingExists)
      iKey.InsertL(i, aBindInfo);
    break;
  default:
    User::Leave(KErrArgument);
    break;
  }
}

TInt CAppuifwEventBindingArray::Callback(SAmarettoEventInfo& aEv)
{
  TInt i = 0;

  switch (aEv.iType) {
  case SAmarettoEventInfo::EKey:
    while (i < iKey.Count()) {
      if (IsBoundTo(iKey[i].iKeyEvent, aEv.iKeyEvent))
        return app_callback_handler(iKey[i].iCb);
      if (aEv.iKeyEvent.iCode < iKey[i].iKeyEvent.iCode)
        break;
      i++;
    }
    break;
  case SAmarettoEventInfo::EPointer:
    // Starting from the end, check if the Pointer falls in any of the bound rects.
	i = iKey.Count() - 1;
    while (i >= 0) {
      if (CheckEventBinding(iKey[i], aEv))
        return app_callback_handler(iKey[i].iCb, Py_BuildValue("((ii))", aEv.iPointerEvent.iPosition.iX, aEv.iPointerEvent.iPosition.iY));
      i--;
    }
    break;
  default:
    break;
  }
  
  return KErrNone;
}
