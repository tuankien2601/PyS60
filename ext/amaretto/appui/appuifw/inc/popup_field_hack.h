/*
* ============================================================================
*  Name     : AknPopupFieldText.h
*  Part of  : Avkon
*  Description:
*     Represents a text combo box type control, used to implement the
*     Series 60 text pop-up field.
*     This control can be constructed directly from resource.
*
* Copyright (c) 2001-2005 Nokia Corporation
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
* ============================================================================
*/

#if !defined(__AKNPOPUPFIELDTEXT_H__)
#define __AKNPOPUPFIELDTEXT_H__

#include "AknPopupField.h"
#include "AknQueryValueText.h"


class CAknPopupFieldText : public CAknPopupField
	{
public: 
	friend class CAppuifwForm;

	/**
	* C++ constructor
	*/
	IMPORT_C CAknPopupFieldText();

	/**
	* C++ Destructor
	*/
	IMPORT_C ~CAknPopupFieldText();

	/**
	* Gets the Current Value Text from the CAknQueryValueText member
	*/
	IMPORT_C HBufC* CurrentValueTextLC();

	/**
	* Gets the Current Value index from the CAknQueryValueText member
	*/
	IMPORT_C TInt CurrentValueIndex() const;

	/**
	* Sets the Current Value index using the CAknQueryValueText member
	*/
	IMPORT_C void SetCurrentValueIndex(const TInt aIndex);

	/**
	* Gets the MdcArray from the CAknQueryValueText member
	*/
	IMPORT_C const MDesCArray* MdcArray() const;

	/**
	* Calls the SetAutoAppend method on the CAknQueryValueText member
	*/
	IMPORT_C void SetAutoAppend(TBool aAppend);

public: // from CCoeControl
	IMPORT_C void ConstructFromResourceL(TResourceReader& aReader);

private: // Methods from CAknPopupField that were public
	/**
	* 2nd phase constructor
	*/
	void ConstructL();

	/**
	* This method should not be used. 
	* It is here to hide it from users, this will panic if you use it.
	*/
	void SetQueryValueL(MAknQueryValue* aValue);

private: // from CCoeControl
	IMPORT_C void Reserved_1();
	IMPORT_C void Reserved_2();

private: // personal
	void CommonConstructL(TInt aTextArrayResourceId, TInt aInitialIndex);

private:
	// the following members are owned
	CDesCArray* iArray;	// the array of text items
	CAknQueryValueTextArray* iTextArray;	
	CAknQueryValueText* iTextValue;
	};

#endif
