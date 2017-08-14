/*
 * ====================================================================
 *  contactsmodule.h
 *
 *  Python API to Series 60 contacts database.
 *
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


#include "Python.h"
#include "symbian_python_ext_util.h"


#include <CNTDB.H>
#include <CNTITEM.H> 
#include <CNTFLDST.H>
#include <e32cmn.h>
#include <CNTFIELD.H>
#include <S32MEM.H>


#include <CPbkFieldsInfo.h> 


//////////////CONSTANTS//////////////


_LIT8(KKeyStrUniqueId, "uniqueid");
_LIT8(KKeyStrTitle, "title");
_LIT8(KKeyStrLastModified, "lastmodified");

_LIT8(KKeyStrFieldId, "fieldid");
_LIT8(KKeyStrFieldLocation, "fieldlocation");
_LIT8(KKeyStrFieldName, "fieldname");


#define KContactGroupTypeId 268440330


//////////////TYPE DEFINITION//////////////


struct Contact_object;


/*
 * Contact database (database = contacts.open())
 */
#define ContactsDb_type ((PyTypeObject*)SPyGetGlobalString("ContactsDbType"))
struct ContactsDb_object {
  PyObject_VAR_HEAD
  CContactDatabase* contactDatabase;
};


/*
 * Contact (contact = database[unique_contact_id])
 */
#define CONTACT_NOT_OPEN 0x0
#define CONTACT_READ_ONLY 0x1
#define CONTACT_READ_WRITE 0x2
#define Contact_type ((PyTypeObject*)SPyGetGlobalString("ContactType"))
struct Contact_object {
  PyObject_VAR_HEAD
  TInt mode;
  TContactItemId uniqueID;
  CContactItem* contactItem;
  ContactsDb_object* contactsDb;
};


/*
 * Contact iterator (for unique_contact_id in database:)
 */
#define ContactIterator_type ((PyTypeObject*)SPyGetGlobalString("ContactIteratorType"))
struct ContactIterator_object { 
  PyObject_VAR_HEAD
  TContactIter* iterator;
  bool initialized; 
  ContactsDb_object* contactsDb;  
};


/*
 * Field iterator (for field in contact:)
 */
#define FieldIterator_type ((PyTypeObject*)SPyGetGlobalString("FieldIteratorType"))
struct FieldIterator_object {
  PyObject_VAR_HEAD
  Contact_object* contact;
  bool initialized;
  TInt iterationIndex;
};


//////////////MACRO DEFINITION///////////////


#define ASSERT_CONTACTOPEN \
  if (self->mode == CONTACT_NOT_OPEN){ \
    PyErr_SetString(PyExc_RuntimeError, "contact not open"); \
    return NULL; \
  }

#define ASSERT_CONTACT_READ_WRITE \
  if (self->mode != CONTACT_READ_WRITE || \
      self->contactsDb->contactDatabase == NULL) { \
    PyErr_SetString(PyExc_RuntimeError, "contact not open for writing"); \
    return NULL; \
  }

#define ASSERT_CONTACTOPEN_FI \
  if (self->contact->mode == CONTACT_NOT_OPEN){\
    PyErr_SetString(PyExc_RuntimeError, "contact not open"); \
    return NULL; \
  }


//////////////METHODS///////////////


/*
 * Create new Contact object.
 * - if (uniqueId == KNullContactId) completely new contact entry is created.
 * Else only new (python) contact object is created (wrapper object to 
 * existing contact entry that the uniqueId identifies).
 */
extern "C" PyObject *
new_Contact_object(ContactsDb_object* self, TContactItemId uniqueId);


/*
 * Create new ContactsDb object.
 */
extern "C" PyObject *
new_ContactsDb_object(CContactDatabase* contactDatabase);


/*
 * Deletes the CPbkContactItem inside the contact object.
 */
void Contact_delete_entry(Contact_object* self);
