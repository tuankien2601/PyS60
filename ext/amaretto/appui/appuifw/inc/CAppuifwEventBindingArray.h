/*
* ====================================================================
*  CAppuifwEventBindingArray.h
*
*  class CAppuifwEventBindingArray & related utilities
*
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
*/

#ifndef __CAPPUIFWEVENTBINDINGARRAY_H
#define __CAPPUIFWEVENTBINDINGARRAY_H

#include "appuifwmodule.h"

struct SAppuifwEventBinding : public SAmarettoEventInfo {
  PyObject* iCb;
};

class CAppuifwEventBindingArray : public CBase
{
 public:
  CAppuifwEventBindingArray():iKey(5) {;}
  IMPORT_C ~CAppuifwEventBindingArray();
  IMPORT_C void InsertEventBindingL(SAppuifwEventBinding&);
  void ClearEventBinding(const SAppuifwEventBinding&);
  TInt Callback(SAmarettoEventInfo& aEvent);

 private:
  CArrayFixSeg<SAppuifwEventBinding> iKey;
};

#endif /*  __CAPPUIFWEVENTBINDINGARRAY_H */
