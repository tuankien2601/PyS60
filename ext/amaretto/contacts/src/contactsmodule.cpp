/*
 * ====================================================================
 * contactsmodule.cpp
 * 
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
 *
 * ====================================================================
 */


 
 
#include "contactsmodule.h"

#ifdef CONTACTS_MAINTENANCE
#include <cpbkcontactengine.h> 
#endif


//////////////GENERAL FUNCTIONS///////////////


/*
 * Tests if the contact entry is a contact group.
 */
TBool IsContactGroup(CContactItem& item)
{
  if(item.Type().iUid==KContactGroupTypeId){
    return ETrue;
  }
  return EFalse;
};



//////////////TYPE METHODS///////////////



/*
 * Module methods.
 */

 
 
/*
 * Opens the database and creates and returns a ContactsDb-object.
 *
 * open() - opens the default contact database file
 * open(u'filename') - opens file if it exists.
 * open(u'filename', 'c') - opens file, creates if the file does not exist.
 * open(u'filename', 'n') - creates new empty database file.
 */
extern "C" PyObject *
open_db(PyObject* /*self*/, PyObject *args)
{
  PyObject* filename = NULL;
  char* flag = NULL;
  TInt userError = KErrNone;
  CContactDatabase* contactDatabase = NULL;

  if (!PyArg_ParseTuple(args, "|Us", &filename, &flag)){ 
    return NULL;
  }
   
  TRAPD(error, { 
    if(!flag){
      if(!filename){
        // Open default db file.
        if(CContactDatabase::DefaultContactDatabaseExistsL()==EFalse){
          contactDatabase = CContactDatabase::CreateL();
        }else{
          contactDatabase = CContactDatabase::OpenL();
        }
      }else{
        // Open given db file, raise exception if the file does not exist.                   
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), PyUnicode_GetSize(filename));
        contactDatabase = CContactDatabase::OpenL(filenamePtr);
      }
    }else{
      if(filename && flag[0] == 'c'){
        // Open, create if file doesn't exist.
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), PyUnicode_GetSize(filename));
        if(EFalse != CContactDatabase::ContactDatabaseExistsL(filenamePtr)){
          contactDatabase = CContactDatabase::OpenL(filenamePtr);
        }else{
          contactDatabase = CContactDatabase::CreateL(filenamePtr);
        }
      }else if(filename && flag[0] == 'n'){
        // Create a new empty file.
        TPtrC filenamePtr((TUint16*) PyUnicode_AsUnicode(filename), PyUnicode_GetSize(filename));
        contactDatabase = CContactDatabase::ReplaceL(filenamePtr);
      }else{
        // Illegal parameter combination.
        userError = 1;
      } 
    }
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(userError == 1){
    PyErr_SetString(PyExc_SyntaxError, "illegal parameter combination");
    return NULL;
  }
  return new_ContactsDb_object(contactDatabase);
} 
  


#ifdef CONTACTS_MAINTENANCE

/*
 * For maintenance and testing only.
 */
extern "C" PyObject *
_create_api_mappings()
{
  TInt error = KErrNone;
  CPbkContactEngine* contactEngine = NULL;  
  PyObject* fieldDataList = NULL;
  
  TRAP(error, {
    contactEngine = CPbkContactEngine::NewL();
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  const CPbkFieldsInfo& fieldsInfo = contactEngine->FieldsInfo();
  
  fieldDataList = PyList_New(fieldsInfo.Count()); 
  if (fieldDataList == NULL){
    delete contactEngine;
    return PyErr_NoMemory();
  }
  
  for(TInt i=0; i<fieldsInfo.Count();i++){ 
    PyObject* idList = NULL;
    idList = PyList_New(fieldsInfo[i]->ContentType().FieldTypeCount());
    if(idList==NULL){
      Py_DECREF(fieldDataList);
      delete contactEngine;
      return PyErr_NoMemory();
    }
    for(TInt j=0;j<fieldsInfo[i]->ContentType().FieldTypeCount();j++){
      PyObject* id = NULL;
      id = Py_BuildValue("i",fieldsInfo[i]->ContentType().FieldType(j));
      if((id==NULL) || (PyList_SetItem(idList,j,id)!=0)){
        Py_DECREF(idList);
        Py_DECREF(fieldDataList);
        delete contactEngine;
        return NULL;
      }
    }
    
    PyObject* infoDict = NULL;    
    infoDict = 
      Py_BuildValue("{s:u#,s:i,s:i,s:i,s:O}",
        (const char*)(&KKeyStrFieldName)->Ptr(), fieldsInfo[i]->FieldName().Ptr(), 
                                                 fieldsInfo[i]->FieldName().Length(),
        (const char*)(&KKeyStrFieldId)->Ptr(), fieldsInfo[i]->FieldId(),
        (const char*)(&KKeyStrFieldLocation)->Ptr(), fieldsInfo[i]->Location(),
        "vcard_mapping", fieldsInfo[i]->ContentType().Mapping(),
        "ids", idList
        );
        
    Py_DECREF(idList);
    if (infoDict == NULL){
      Py_DECREF(fieldDataList);
      delete contactEngine;
      return NULL;
    }
 
    error = PyList_SetItem(fieldDataList, i, infoDict);
    if(error!=0){
      Py_DECREF(fieldDataList);
      delete contactEngine;
      return NULL;
    }  
  }
  delete contactEngine;
  
  return fieldDataList;
}
 
#endif


/*
 * Create new ContactsDb object.
 */
extern "C" PyObject *
new_ContactsDb_object(CContactDatabase* contactDatabase)
{
  ContactsDb_object* contactsDb = 
    PyObject_New(ContactsDb_object, ContactsDb_type);
  if (contactsDb == NULL){
    delete contactDatabase;
    return PyErr_NoMemory();
  }

  contactsDb->contactDatabase = contactDatabase;

  return (PyObject*) contactsDb;
}



/*
 * ContactsDb methods.
 */

 

/*
 * Returns contact object (indicated by unique id given)
 */ 
static PyObject *
ContactsDb_get_entry(ContactsDb_object *self, PyObject *args)
{
  TInt32 uniqueId = -1;
  
  if (!PyArg_ParseTuple(args, "i", &uniqueId)){ 
    return NULL;
  }
  
  if((uniqueId == KNullContactId) || (uniqueId == self->contactDatabase->TemplateId())){     
    PyErr_SetString(PyExc_ValueError, "illegal contact id");
    return NULL;
  }
  
  return new_Contact_object(self, uniqueId);
}


/*
 * Create new contact entry. 
 */ 
static PyObject *
ContactsDb_create_entry(ContactsDb_object *self, PyObject* /*args*/)
{
  return new_Contact_object(self, KNullContactId);
}


/*
 * Searches contacts by string (match is detected if the string is
 * a substring of some field value of the contact).
 * Returns tuple of unique contact ID:s representing contact entrys
 * matching the criteria.
 */
extern "C" PyObject *
ContactsDb_find(ContactsDb_object* self, PyObject *args)
{
  TInt error = KErrNone;
  PyObject* searchString = NULL;
  CContactIdArray* idArray = NULL;
  PyObject* fieldIdTuple = NULL;
  CContactItemFieldDef* selectedFieldIds = NULL;
  PyObject* idArrayTuple = NULL;

  if (!PyArg_ParseTuple(args, "UO!", 
                        &searchString, &PyTuple_Type, &fieldIdTuple)){ 
    return NULL;
  }
  
  if(PyTuple_Size(fieldIdTuple)<=0){
    PyErr_SetString(PyExc_RuntimeError, "no field types specified");
    return NULL;
  }

  TPtrC searchStringPtr((TUint16*) PyUnicode_AsUnicode(searchString), 
                         PyUnicode_GetSize(searchString));
   
  TRAP(error, {                          
    selectedFieldIds = new (ELeave) CContactItemFieldDef;                      
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  } 
   
  TRAP(error, {                                  
    for(TInt i=0;i<PyTuple_Size(fieldIdTuple);i++){
      PyObject* fieldIdItem = PyTuple_GetItem(fieldIdTuple, i);
      TInt fieldId = PyInt_AsLong(fieldIdItem);
      selectedFieldIds->AppendL(TUid::Uid(fieldId));
    }                       
    idArray = self->contactDatabase->FindLC(searchStringPtr, selectedFieldIds);  
    CleanupStack::Pop(idArray);
  });
  selectedFieldIds->Reset();
  delete selectedFieldIds;
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
 
  idArrayTuple = PyTuple_New(idArray->Count());
  if (idArrayTuple == NULL){
    idArray->Reset();
    delete idArray;
    return PyErr_NoMemory();
  }
  
  for(TInt i=0; i<idArray->Count(); i++){
    PyObject* entryId = Py_BuildValue("i", (*idArray)[i]);
    if (entryId == NULL){
      idArray->Reset();
      delete idArray;
      Py_DECREF(idArrayTuple);
      return NULL;
    }
    if(PyTuple_SetItem(idArrayTuple, i, entryId)<0){
      Py_DECREF(idArrayTuple);
      idArray->Reset();
      delete idArray;
      return NULL;
    }
  }
  idArray->Reset();
  delete idArray;
  return (PyObject*)idArrayTuple;
}


/*
 * Returns id:s of contact groups in a list.
 */
extern "C" PyObject *
ContactsDb_contact_groups(ContactsDb_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  CContactIdArray* idArray = NULL;
  PyObject* idList = NULL;
  
  TRAP(error, {
    idArray = self->contactDatabase->GetGroupIdListL();
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  idList = PyList_New(idArray->Count());
  if(idList==NULL){
    idArray->Reset();
    delete idArray;
    return PyErr_NoMemory();
  }
  
  for(TInt i=0;i<idArray->Count();i++){
    PyObject* id = Py_BuildValue("i", (*idArray)[i]);
    if(id==NULL){
      idArray->Reset();
      delete idArray;
      Py_DECREF(idList);
      return NULL;
    }
    if(PyList_SetItem(idList, i, id)!=0){
      idArray->Reset();
      delete idArray;
      Py_DECREF(idList);
      return NULL;
    }
  }
  
  idArray->Reset();
  delete idArray;
  return idList;
}


/*
 * Returns name/label of specified contact group.
 */
extern "C" PyObject *
ContactsDb_contact_group_label(ContactsDb_object* self, PyObject *args)
{
  TInt error = KErrNone;
  PyObject* label = NULL;
  TInt32 entryId = -1;
  CContactItem* item = NULL;
  if (!PyArg_ParseTuple(args, "i", &entryId)){ 
    return NULL;
  }
    
  TRAP(error, {
    item = self->contactDatabase->ReadContactL(entryId);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(IsContactGroup(*item)==EFalse){
    delete item;
    PyErr_SetString(PyExc_RuntimeError, "not a contact group");
    return NULL;
  }
  
  CContactGroup* group = dynamic_cast<CContactGroup*>(item);

  TRAP(error, {
    label = Py_BuildValue("u#",group->GetGroupLabelL().Ptr(),group->GetGroupLabelL().Length());
  });
  delete item;
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  return label;
}


/*
 * Sets name/label of the specified contact group.
 */
extern "C" PyObject *
ContactsDb_contact_group_set_label(ContactsDb_object* self, PyObject *args)
{
  TInt error = KErrNone;
  PyObject* label = NULL;
  TInt32 entryId = -1;
  CContactItem* item = NULL;
  if (!PyArg_ParseTuple(args, "iU", &entryId, &label)){ 
    return NULL;
  }
    
  TRAP(error, {
    item = self->contactDatabase->OpenContactL(entryId);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(IsContactGroup(*item)==EFalse){
    delete item;
    PyErr_SetString(PyExc_RuntimeError, "not a contact group");
    return NULL;
  }
  
  CContactGroup* group = dynamic_cast<CContactGroup*>(item);

  TPtrC labelPtr((TUint16*)PyUnicode_AsUnicode(label),PyUnicode_GetSize(label));  
  TRAP(error, {
    group->SetGroupLabelL(labelPtr);
    self->contactDatabase->CommitContactL(*group);
  });
  delete item;
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns id:s of contacts that are members of the specified contact group.
 */
extern "C" PyObject *
ContactsDb_contact_group_ids(ContactsDb_object* self, PyObject *args)
{
  TInt error = KErrNone;
  TInt32 entryId = -1;
  CContactItem* item = NULL;
  PyObject* idList = NULL;
  const CContactIdArray* idArray = NULL;
  
  if (!PyArg_ParseTuple(args, "i", &entryId)){ 
    return NULL;
  }
    
  TRAP(error, {
    item = self->contactDatabase->ReadContactL(entryId);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(IsContactGroup(*item)==EFalse){
    delete item;
    PyErr_SetString(PyExc_RuntimeError, "not a contact group");
    return NULL;
  }
  
  CContactGroup* group = dynamic_cast<CContactGroup*>(item);
  idArray = group->ItemsContained();
  
  idList = PyList_New(idArray->Count());
  if(idList==NULL){
    delete item;
    return PyErr_NoMemory();
  }
  
  for(TInt i=0;i<idArray->Count();i++){
    PyObject* id = Py_BuildValue("i", (*idArray)[i]);
    if(id==NULL){
      delete item;
      Py_DECREF(idList);
      return NULL;
    }
    if(PyList_SetItem(idList, i, id)!=0){
      delete item;
      Py_DECREF(idList);
      return NULL;
    }
  }
  delete item;
  return idList;
}


/*
 * Adds contact entry to the specified contact group.
 */
extern "C" PyObject *
ContactsDb_add_contact_to_group(ContactsDb_object* self, PyObject *args)
{
  TInt error = KErrNone;
  TInt32 entryId = -1;
  TInt32 groupId = -1;
  CContactItem* item = NULL;
  
  if (!PyArg_ParseTuple(args, "ii", &entryId, &groupId)){ 
    return NULL;
  }
 
  TRAP(error, {
    item = self->contactDatabase->ReadContactL(groupId);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(IsContactGroup(*item)==EFalse){
    delete item;
    PyErr_SetString(PyExc_RuntimeError, "not a contact group");
    return NULL;
  }
  delete item;
  
  
  TRAP(error, {
    self->contactDatabase->AddContactToGroupL(entryId,groupId);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Removes contact from the group.
 */
extern "C" PyObject *
ContactsDb_remove_contact_from_group(ContactsDb_object* self, PyObject *args)
{
  TInt error = KErrNone;
  TInt32 entryId = -1;
  TInt32 groupId = -1;
  CContactItem* item = NULL;
  CContactGroup* group = NULL;
  
  if (!PyArg_ParseTuple(args, "ii", &entryId, &groupId)){ 
    return NULL;
  }
  
  TRAP(error, {
    item = self->contactDatabase->ReadContactL(groupId);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(IsContactGroup(*item)==EFalse){
    delete item;
    PyErr_SetString(PyExc_RuntimeError, "not a contact group");
    return NULL;
  }
  group = dynamic_cast<CContactGroup*>(item);
  if(group->ContainsItem(entryId)==EFalse){
    delete item;
    PyErr_SetString(PyExc_RuntimeError, "contact is not a member of the group");
    return NULL;
  }
  delete item;
 
  TRAP(error, {
    self->contactDatabase->RemoveContactFromGroupL(entryId,groupId);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Creates a new contact group.
 */
extern "C" PyObject *
ContactsDb_create_contact_group(ContactsDb_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  CContactItem* item = NULL;
  PyObject* ret = NULL;
    
  TRAP(error, {
    item = self->contactDatabase->CreateContactGroupL();
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  ret = Py_BuildValue("i", item->Id());
  delete item;
  
  return ret;
}


/*
 * Returns number of contact groups.
 */
extern "C" PyObject *
ContactsDb_contact_group_count(ContactsDb_object* self, PyObject* /*args*/)
{    
  return Py_BuildValue("i", self->contactDatabase->GroupCount());
}


/*
 * ContactsDb_object deletes given contact entry from the database.
 */
extern "C" PyObject *
ContactsDb_delete_entry(ContactsDb_object* self, PyObject* args)
{
  TInt32 entryId = -1;
  if (!PyArg_ParseTuple(args, "i", &entryId)){ 
    return NULL;
  }
    
  
  TInt error = KErrNone;
  TRAP(error, self->contactDatabase->DeleteContactL(entryId));
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Py_INCREF(Py_None);
  return Py_None;   
}


/*
 * Returns number of contacts.
 */
extern "C" PyObject *
ContactsDb_entry_count(ContactsDb_object *self, PyObject* /*args*/)
{
  TInt length = -1;

  TRAPD(error, {
    length = self->contactDatabase->CountL();
  });
  if(error != KErrNone){
    SPyErr_SetFromSymbianOSErr(error);
  }

  return Py_BuildValue("i", length);
}


/*
 * Tests whether a compressing is recommended.
 */
extern "C" PyObject *
ContactsDb_compact_recommended(ContactsDb_object* self, PyObject* /*args*/)
{
  return Py_BuildValue("i", self->contactDatabase->CompressRequired());
}


/*
 * Compresses the database.
 */
extern "C" PyObject *
ContactsDb_compact(ContactsDb_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;

  Py_BEGIN_ALLOW_THREADS
  TRAP(error, {
    self->contactDatabase->CompactL();
  });
  Py_END_ALLOW_THREADS

  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns information about the specified field type.
 */
extern "C" PyObject *
ContactsDB_field_type_info(ContactsDb_object* self, PyObject* args)
{
  TInt error = KErrNone;
  TInt index = -1;
  CContactItem* templateItem = NULL;
  PyObject* ret = NULL;
    
  if (!PyArg_ParseTuple(args, "i", &index)){ 
    return NULL;
  }
  
  TRAP(error, {
    templateItem = self->contactDatabase->ReadContactL(self->contactDatabase->TemplateId());
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  CContactItemFieldSet& templateFields = templateItem->CardFields(); 
  if(templateFields.Count()<=index){
    delete templateItem;
    PyErr_SetString(PyExc_ValueError,   "illegal index");
    return NULL;
  }
  
  CContactItemField& field = templateFields[index];
  
  ret = Py_BuildValue("{s:u#,s:i,s:i}", "name", field.Label().Ptr(),
                                                field.Label().Length(),
                                        "storagetype", field.StorageType(),
                                        "vcard_mapping", field.ContentType().Mapping());
                                    
  delete templateItem;
  return ret;  
}


/*
 * Returns ids of all field types (all that exist in the system template).
 */
extern "C" PyObject *
ContactsDB_field_types(ContactsDb_object* self, PyObject* /*args*/)
{
  TInt error = KErrNone;
  CContactItem* templateItem = NULL;
  PyObject* fieldTypes = NULL;
  PyObject* vcardMapping = NULL;
  PyObject* packedLists = NULL;
    
  TRAP(error, {
    templateItem = self->contactDatabase->ReadContactL(self->contactDatabase->TemplateId());
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
    
  CContactItemFieldSet& templateFields = templateItem->CardFields(); 
  TInt templateFieldsCount = templateFields.Count();
  fieldTypes = PyList_New(templateFieldsCount);
  if(fieldTypes==NULL){
    delete templateItem;
    return PyErr_NoMemory();
  }
  vcardMapping = PyList_New(templateFieldsCount);
  if(vcardMapping==NULL){
    delete templateItem;
    delete fieldTypes;
    return PyErr_NoMemory();
  }
  
  for(TInt i=0;i<templateFieldsCount;i++){
    CContactItemField& field = templateFields[i];  
    TInt fieldTypesCount = field.ContentType().FieldTypeCount();
    PyObject* idList = PyList_New(fieldTypesCount);
    if(idList==NULL){
      Py_DECREF(fieldTypes);
      delete templateItem;
      return PyErr_NoMemory();
    }
    for(TInt j=0;j<fieldTypesCount;j++){
      PyObject* id = Py_BuildValue("i",field.ContentType().FieldType(j));
      if(id==NULL){
        Py_DECREF(idList);
        Py_DECREF(fieldTypes);
        delete templateItem;
        return NULL;
      }
      if(PyList_SetItem(idList,j,id)!=0){
        Py_DECREF(idList);
        Py_DECREF(fieldTypes);
        delete templateItem;
        return NULL;
      }
    }
    if(PyList_SetItem(fieldTypes,i,idList)!=0){
      Py_DECREF(idList);
      Py_DECREF(fieldTypes);
      delete templateItem;
      return NULL;
    }
    PyObject* vcard_id = Py_BuildValue("i",field.ContentType().Mapping());
    if(vcard_id==NULL){
        Py_DECREF(fieldTypes);
        Py_DECREF(vcardMapping);
        delete templateItem;
        return NULL;
      }
    if(PyList_SetItem(vcardMapping,i,vcard_id)!=0){
      Py_DECREF(vcardMapping);
      Py_DECREF(fieldTypes);
      delete templateItem;
      return NULL;
    }
  }   
  
  delete templateItem;
  packedLists = PyList_New(2*templateFieldsCount);
  if(packedLists==NULL){
    Py_DECREF(fieldTypes);
    Py_DECREF(vcardMapping);
    return PyErr_NoMemory();
  }
  if(PyList_SetItem(packedLists,0,fieldTypes)!=0){
    Py_DECREF(packedLists);
    delete fieldTypes;
    delete vcardMapping;
    return NULL;
  }
  if(PyList_SetItem(packedLists,1,vcardMapping)!=0){
    Py_DECREF(packedLists);
    delete fieldTypes;
    delete vcardMapping;
    return NULL;
  }
  return packedLists;
}


/*
 * Imports VCards (vcards are given in a string).
 * Returns tuple that containts unique id:s of imported
 * contact entries.
 */
extern "C" PyObject *
ContactsDb_import_vcards(ContactsDb_object* self, PyObject* args)
{
  char* vCardStr = NULL;
  TInt vCardStrLength = 0;
  TInt flags = CContactDatabase::EIncludeX 
              |CContactDatabase::ETTFormat;
  PyObject* idTuple = NULL;
  CArrayPtr<CContactItem>* imported = NULL;
   
  if (!PyArg_ParseTuple(args, "s#|i",
                        &vCardStr, &vCardStrLength, &flags)){ 

    return NULL;
  }

  TPtrC8 vCardStrPtr((TUint8*)vCardStr, vCardStrLength); 
  
  TUid uid;
  uid.iUid = KVersitEntityUidVCard;
  TBool success = EFalse;
 
  TRAPD(error, {
    RMemReadStream inputStream(vCardStrPtr.Ptr(), vCardStrPtr.Length());  
    CleanupClosePushL(inputStream);  
    imported = 
      self->contactDatabase->ImportContactsL(uid, inputStream, 
                                               success, flags); 
    CleanupStack::PopAndDestroy(); // Close inputStream.
  });

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  idTuple = PyTuple_New(imported->Count());

  if(idTuple==NULL){
    imported->ResetAndDestroy(); 
    delete imported;
    return PyErr_NoMemory();
  }

  for(TInt i=0;i<imported->Count();i++){
    PyObject* idObj = Py_BuildValue("i", ((*imported)[i])->Id());
    if(idObj==NULL){
      Py_DECREF(idTuple);
      imported->ResetAndDestroy(); 
      delete imported;
      return NULL;
    }
    if(PyTuple_SetItem(idTuple, i, idObj)<0){
      Py_DECREF(idTuple);
      imported->ResetAndDestroy(); 
      delete imported;
      return NULL;
    }
  }

  imported->ResetAndDestroy(); 
  delete imported;

  return idTuple;
}


/*
 * Writes VCards that represent specified contact entries into the unicode string.
 */
extern "C" PyObject *
ContactsDb_export_vcards(ContactsDb_object* self, PyObject* args)
{

  TInt error = KErrNone;
  PyObject* idTuple;
  CContactIdArray* idArray = NULL;
  TInt flags = CContactDatabase::EIncludeX
              |CContactDatabase::ETTFormat;
  PyObject* ret = NULL;
  
  if (!PyArg_ParseTuple(args, "O!|i", 
                        &PyTuple_Type, &idTuple, &flags)){ 
    return NULL;
  }

  if(PyTuple_Size(idTuple)<1){
    PyErr_SetString(PyExc_SyntaxError, "no contact id:s given in the tuple");
    return NULL;
  }

  TRAP(error, { 
    idArray = CContactIdArray::NewL();
  });

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  // Put the unique contact id:s into the idArray.
  TInt idCount = PyTuple_Size(idTuple);
  for(TInt i=0;i<idCount;i++){
    PyObject* idItem = PyTuple_GetItem(idTuple, i);

    if(!PyInt_Check(idItem)){
      delete idArray;
      PyErr_SetString(PyExc_TypeError, "illegal argument in the tuple");
      return NULL;
    };

    TInt uniqueId = PyInt_AsLong(idItem);
    TRAPD(error, {
      idArray->AddL(uniqueId);
    });

    if(error != KErrNone){
      delete idArray;
      return SPyErr_SetFromSymbianOSErr(error);
    }
  }

  // Do the export.
  TRAP(error, {
    CleanupStack::PushL(idArray);

    CBufFlat* flatBuf = CBufFlat::NewL(4);
    CleanupStack::PushL(flatBuf);
    RBufWriteStream outputStream(*flatBuf);
    CleanupClosePushL(outputStream);

    TUid uid;
    uid.iUid = KVersitEntityUidVCard;
   
    // Export contacts into the stream.
    self->contactDatabase->ExportSelectedContactsL(uid,
                                                   *idArray, 
                                                   outputStream,
                                                   flags);  
    outputStream.CommitL();        
    CleanupStack::PopAndDestroy(); // Close outputStream.  
    
    ret = Py_BuildValue("s#", (char*)flatBuf->Ptr(0).Ptr(), flatBuf->Size());
    
    CleanupStack::PopAndDestroy(); // flatBuf.
    CleanupStack::PopAndDestroy(); // idArray.
  });

  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }    
  return ret;
}


/*
 * Quesses the default label of the field (in case of incomplete field definition
 * that has no match in the template).
 * This is used to determine the corresponding field type.
 */
extern "C" PyObject *
ContactsDb_determine_field_name(ContactsDb_object* self, PyObject *args)
{  
  _LIT(KTempText,"123456789");
  _LIT8(KTempText8,"123456789");
  TTime tempTime;
  tempTime.HomeTime();
  
  TInt error=KErrNone;
  CContactItem* tempContact = NULL;
  CContactItemField* field = NULL;
  PyObject* label = NULL;
  TContactItemId id = KNullContactId;
  TInt userError = 0;
  
  PyObject* idTuple = NULL;
  TInt mapping = 0;
  TInt storageType = 0;
  
  if (!PyArg_ParseTuple(args, "iiO", &storageType, &mapping, &idTuple)){ 
    return NULL;
  }
  
  if(!PyTuple_Check(idTuple)){
    PyErr_SetString(PyExc_TypeError, "ids must be given in a tuple");
    return NULL;
  }
  
  TRAP(error,{  
    tempContact = (CContactItem*) CContactCard::NewL(); 
    CleanupStack::PushL(tempContact);
    
    field = CContactItemField::NewL(storageType);
    CleanupStack::PushL(field);   
    
    for(TInt i=0;i<PyTuple_Size(idTuple);i++){
      PyObject* fieldIdObj = PyTuple_GetItem(idTuple,i);
      field->AddFieldTypeL(TUid::Uid(PyInt_AsLong(fieldIdObj)));
    }
    
    field->SetMapping(TUid::Uid(mapping));
    
    if(storageType==KStorageTypeText){
      field->TextStorage()->SetTextL(KTempText);
    }else if(storageType==KStorageTypeDateTime){
      field->DateTimeStorage()->SetTime(tempTime);
    }else if(storageType==KStorageTypeStore){
      field->StoreStorage()->SetThingL(KTempText8);
    }else if(storageType==KStorageTypeContactItemId){
      field->AgentStorage()->SetAgentId(0);
    }else{
      // this can never happen unless additional storage types will be specified in the SDK.
      User::Leave(KErrNotSupported);
    }
    
    tempContact->AddFieldL(*field);
    CleanupStack::Pop(); // field.
    CleanupStack::Pop(); // tempContact.
  });  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }   
  
  TRAP(error, {
    id=self->contactDatabase->AddNewContactL(*tempContact);
  });
  delete tempContact;
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  TRAP(error, {
    tempContact = NULL;  
    tempContact = self->contactDatabase->OpenContactL(id); 
    CleanupStack::PushL(tempContact); 
    if(tempContact->CardFields().Count()==1){
      label = Py_BuildValue("u#", tempContact->CardFields()[0].Label().Ptr(),tempContact->CardFields()[0].Label().Length());
    }else{
      userError = 1;
    }
    self->contactDatabase->CloseContactL(id);
    CleanupStack::PopAndDestroy(tempContact);
    self->contactDatabase->DeleteContactL(id);
  });
  if(error!=KErrNone){
    Py_XDECREF(label);
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(userError==1){
    PyErr_SetString(PyExc_RuntimeError, "field type check failed");
    return NULL;
  }
      
  return label;  
}


/*
 * Deallocate ContactsDb_object.
 */
extern "C" {
  static void ContactsDb_dealloc(ContactsDb_object *contactsDb)
  {
    delete contactsDb->contactDatabase;
    PyObject_Del(contactsDb);
  }
} 



/*
 * ContactIterator methods.
 */


 
/*
 * Create new ContactIterator object.
 */
extern "C" PyObject *
new_ContactIterator_object(ContactsDb_object* self, PyObject* /**args*/)
{
  TContactIter* iterator = NULL;
  ContactIterator_object* ci = NULL;
  
  TRAPD(error, {
    iterator = new (ELeave) TContactIter(*(self->contactDatabase));
  });
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  ci = PyObject_New(ContactIterator_object, ContactIterator_type);
  if (ci == NULL){
    delete iterator;
    return PyErr_NoMemory();
  }
  
  ci->initialized = false;
  ci->contactsDb = self;
  ci->iterator = iterator; 
  
  ci->contactsDb = self;
  Py_INCREF(ci->contactsDb);
  return (PyObject*)ci;
}


/*
 * Get the uniqueId of the next contact entry object.
 */
extern "C" PyObject *
ContactIterator_next(ContactIterator_object* self, PyObject* /**args*/)
{
  TContactItemId uniqueId = KNullContactId;
  TInt error = KErrNone;
  CContactIdArray* groupIdArray = NULL; // prevent "group entries" from iterating.
  
  TRAP(error, {
    // note that this returns NULL if there are no groups.
    groupIdArray = self->contactsDb->contactDatabase->GetGroupIdListL();
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  do{
    if (self->initialized){
      TRAP(error, uniqueId = self->iterator->NextL());   
    }else{
      TRAP(error, uniqueId = self->iterator->FirstL());
      self->initialized = true;
    }
    if(error != KErrNone){   
      if(groupIdArray!=NULL){
        groupIdArray->Reset();
        delete groupIdArray;
      }
      return SPyErr_SetFromSymbianOSErr(error);
    }
  
    // Check if the iteration ended.
    if(uniqueId == KNullContactId){
      if(groupIdArray!=NULL){
        groupIdArray->Reset();
        delete groupIdArray;
      }
      PyErr_SetObject(PyExc_StopIteration, Py_None);
      return NULL;
    }
  }while((groupIdArray!=NULL) && (groupIdArray->Find(uniqueId)!=KErrNotFound));  
    
  if(groupIdArray!=NULL){
    groupIdArray->Reset();
    delete groupIdArray;
  }
  
  return Py_BuildValue("i", uniqueId);
}


/*
 * Deallocate ContactIterator_object.
 */
extern "C" {
  static void ContactIterator_dealloc(ContactIterator_object *contactIterator)
  {  
    delete contactIterator->iterator;
    contactIterator->iterator = NULL;
  
    Py_DECREF(contactIterator->contactsDb);
    PyObject_Del(contactIterator);
  }
}



/*
 * Contact methods.
 */


 
/*
 * Create new Contact object.
 * if (uniqueId == KNullContactId) completely new contact entry is created.
 * Else only new (python) contact object is created (wrapper object to 
 * existing native contact entry that the uniqueId identifies).
 */
extern "C" PyObject *
new_Contact_object(ContactsDb_object* self, TContactItemId uniqueId)
{
  Contact_object* contact = PyObject_New(Contact_object, Contact_type);
  if (contact == NULL){
    return PyErr_NoMemory();
  }

  // Initialize the contact struct.
  contact->mode = CONTACT_NOT_OPEN;
  contact->contactItem = NULL;
  contact->contactsDb = NULL;
  contact->uniqueID = KNullContactId;
  
  if(uniqueId == KNullContactId){        
    // a new contact entry must be created into the database.
    TRAPD(error, {
      contact->contactItem = (CContactItem*) CContactCard::NewL();
      contact->mode = CONTACT_READ_WRITE;
    });

    if(error != KErrNone){  
      PyObject_Del(contact);
      return SPyErr_SetFromSymbianOSErr(error);
    } 
  }else{
    // contact entry that corresponds to given unique id exists.
    TRAPD(error, {
      contact->contactItem = self->contactDatabase->ReadContactL(uniqueId); 
      contact->mode = CONTACT_READ_ONLY;
    });
    if(error != KErrNone){  
      PyObject_Del(contact);
      return SPyErr_SetFromSymbianOSErr(error);
    }
  }

  contact->contactsDb = self;
  contact->uniqueID = uniqueId;
  Py_INCREF(contact->contactsDb);
  return (PyObject*)contact;
}


/*
 * Deletes the CContactItem inside the contact object.
 */
void Contact_delete_entry(Contact_object* self)
{
  if((self->mode==CONTACT_READ_WRITE) && (self->uniqueID!=KNullContactId)){
    self->contactsDb->contactDatabase->CloseContactL(self->uniqueID);
  }
  delete self->contactItem;
  self->contactItem = NULL;
  self->mode = CONTACT_NOT_OPEN;
}


/*
 * Sets the contact to the read-only mode.
 */
extern "C" PyObject *
Contact_open_ro(Contact_object* self)
{
  if(self->mode == CONTACT_READ_ONLY){
    // Already in read-only mode.
    Py_INCREF(Py_None);
    return Py_None;
  }

  TRAPD(error, {
    self->contactItem = 
      self->contactsDb->contactDatabase->ReadContactL(self->uniqueID);
    self->mode = CONTACT_READ_ONLY;
  });  

  RETURN_ERROR_OR_PYNONE(error);
}


/*
 * Sets the contact to the read-write mode.
 */
extern "C" PyObject *
Contact_open_rw(Contact_object* self)
{
  if(self->mode == CONTACT_READ_WRITE){
    // Already in read-write mode.
    Py_INCREF(Py_None);
    return Py_None;
  }

  TRAPD(error, {
    self->contactItem = 
      self->contactsDb->contactDatabase->OpenContactL(self->uniqueID);
    self->mode = CONTACT_READ_WRITE;
  });

  RETURN_ERROR_OR_PYNONE(error);
}


/*
 * Returns contact data (attributes of contact, not it's field data).
 */
extern "C" PyObject *
Contact_entry_data(Contact_object* self, PyObject* /*args*/)
{
  ASSERT_CONTACTOPEN
  
  PyObject* entryData = 
    Py_BuildValue("{s:i,s:d}", 
                   (const char*)(&KKeyStrUniqueId)->Ptr(), self->contactItem->Id(),
                   (const char*)(&KKeyStrLastModified)->Ptr(), 
                   time_as_UTC_TReal(self->contactItem->LastModified()));

  return  entryData;
}


/*
 * Opens contact for writing.
 */
extern "C" PyObject *
Contact_begin(Contact_object* self, PyObject* /*args*/)
{
  if(self->mode != CONTACT_READ_WRITE){
    Contact_delete_entry(self);
    return Contact_open_rw(self);
  }

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Commits the contact.
 */
extern "C" PyObject *
Contact_commit(Contact_object* self, PyObject* /*args*/)
{
  ASSERT_CONTACT_READ_WRITE

  TInt error = KErrNone;
  
  if(self->uniqueID == KNullContactId){
    // New contact.
    TRAP(error, 
      self->uniqueID = 
        self->contactsDb->contactDatabase->AddNewContactL(*self->contactItem));
  } else {
    // Old contact (begin() called).
    TRAP(error, 
      self->contactsDb->contactDatabase->CommitContactL(*self->contactItem));
  }  
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Contact_delete_entry(self);

  return Contact_open_ro(self);
}


/*
 * Closes (but does not commit) the contact.
 */
extern "C" PyObject *
Contact_rollback(Contact_object* self, PyObject* /*args*/)
{
  if(self->mode != CONTACT_READ_WRITE || self->uniqueID == KNullContactId){
    PyErr_SetString(PyExc_RuntimeError, "begin not called");
    return NULL; 
  }

  TInt error = KErrNone;

  TRAP(error, 
    self->contactsDb->contactDatabase->CloseContactL(self->uniqueID));
  
  if(error != KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }

  Contact_delete_entry(self);

  return Contact_open_ro(self);
}


/*
 * Returns indexes of the fields that represent given fieldtype.
 */
extern "C" PyObject *
Contact_find_field_indexes(Contact_object* self, PyObject* args)
{
  ASSERT_CONTACTOPEN
  
  TInt error = KErrNone;
  CArrayVarFlat<TInt>* indexArray = NULL;
  TInt fieldIndex=0;
  TInt fieldId = -1;

  if (!PyArg_ParseTuple(args, "i", &fieldId)){ 
    return NULL;
  }

  CContactItemFieldSet& fieldSet = self->contactItem->CardFields();

  TRAP(error, {
    indexArray = new (ELeave) CArrayVarFlat<TInt>(2); // 2 is the granularity (not the size..)
    TUid fieldUid;
    fieldUid = TUid::Uid(fieldId);
    do{
      fieldIndex = fieldSet.FindNext(fieldUid, fieldIndex);
      if(fieldIndex!=KErrNotFound){
        indexArray->AppendL(fieldIndex, sizeof(fieldIndex));  
        fieldIndex++;
      }
    }while(fieldIndex!=KErrNotFound);
  });
  if(error!=KErrNone){
    if(indexArray!=NULL){
      indexArray->Reset();
    }
    delete indexArray;
    return SPyErr_SetFromSymbianOSErr(error);
  }

  PyObject* indexList = PyList_New(indexArray->Count());
  if(indexList==NULL){
    indexArray->Reset();
    delete indexArray;
    return PyErr_NoMemory();
  }
  for(TInt i=0;i<indexArray->Count();i++){
    PyObject* theIndex = Py_BuildValue("i", indexArray->At(i));
    if(theIndex==NULL){
      indexArray->Reset();
      delete indexArray;
      Py_DECREF(indexList);
      return NULL;
    }
    if(0>PyList_SetItem(indexList,i,theIndex)){
      indexArray->Reset();
      delete indexArray;
      Py_DECREF(indexList);
      return NULL;
    }
  }

  indexArray->Reset();
  delete indexArray;
  return indexList;
}  


/*
 * Test if the contact entry is a contact group.
 */
extern "C" PyObject *
Contact_is_contact_group(Contact_object* self, PyObject* /*args*/)
{ 
  TBool isGroup = EFalse; 
  if(EFalse!=IsContactGroup(*(self->contactItem))){
    isGroup = ETrue;
  }
  
  return Py_BuildValue("i", isGroup);
}


/*
 * Modifies contact field.
 */
extern "C" PyObject *
Contact_modify_field(Contact_object* self, PyObject *args, PyObject *keywds)
{
  TInt error = KErrNone;
  PyObject* valueObj = NULL;
  PyObject* label = NULL;
  TInt fieldIndex = -1;
  TReal dateVal = -1;
  char* value = NULL; 
  TInt valueLength = 0;
  
  static const char *const kwlist[] = 
    {"field_index", "value", "label", NULL};
 
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|OU", (char**)kwlist,
                                   &fieldIndex, 
                                   &valueObj, 
                                   &label)){ 
    return NULL;
  }
  
  if(valueObj){ 
    if(PyFloat_Check(valueObj)){
      // DateTime type.
      dateVal = PyFloat_AsDouble(valueObj);
    } else if(PyUnicode_Check(valueObj)){
      // Text type.
      value = (char*)PyUnicode_AsUnicode(valueObj);
      valueLength = PyUnicode_GetSize(valueObj);
    } else {
      PyErr_SetString(PyExc_TypeError, "illegal value parameter");
      return NULL;
    }
  }
  
  if(fieldIndex >= self->contactItem->CardFields().Count()){
    PyErr_SetString(PyExc_ValueError, "illegal field index");
    return NULL;
  }
  
  CContactItemField& field = self->contactItem->CardFields()[fieldIndex];  
  if((field.StorageType()==KStorageTypeText) && (value !=NULL)){
    TRAP(error, {
      field.TextStorage()->SetTextL(TPtrC((TUint16*)value, valueLength));
    });
    if(error!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);
    }
  }else if((field.StorageType()==KStorageTypeDateTime) && (dateVal > 0)){
    TTime datetime;
    pythonRealAsTTime(dateVal, datetime);
    field.DateTimeStorage()->SetTime(datetime);
  }else if((value != NULL) || (dateVal > 0)){
    PyErr_SetString(PyExc_ValueError, "illegal field type or value");
    return NULL;
  } 
      
  if(label!=NULL){    
    TRAP(error, {
      field.SetLabelL(TPtrC((TUint16*)PyUnicode_AsUnicode(label),PyUnicode_GetSize(label)));
    }); 
    if(error!=KErrNone){
      return SPyErr_SetFromSymbianOSErr(error);
    }  
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Adds field to the contact.
 */
extern "C" PyObject *
Contact_add_field(Contact_object* self, PyObject *args, PyObject *keywds)
{  
  ASSERT_CONTACT_READ_WRITE
  
  TInt error = KErrNone;
  TInt fieldInfoIndex = -1;
  PyObject* valueObj = NULL;
  char* value = NULL; 
  TInt valueLength = 0;
  PyObject* label = NULL;
  TReal dateVal = -1;
  TInt userError = 0;
  CContactItemField* field = NULL;
 
  static const char *const kwlist[] = 
    {"fieldtype", "value", "label", NULL};
 
  if (!PyArg_ParseTupleAndKeywords(args, keywds, "i|OU", (char**)kwlist,
                                   &fieldInfoIndex, 
                                   &valueObj, 
                                   &label)){ 
    return NULL;
  }

  if(valueObj){ 
    if(PyFloat_Check(valueObj)){
      // DateTime type.
      dateVal = PyFloat_AsDouble(valueObj);
    } else if(PyUnicode_Check(valueObj)){
      // Text type.
      value = (char*)PyUnicode_AsUnicode(valueObj);
      valueLength = PyUnicode_GetSize(valueObj);
    } else {
      PyErr_SetString(PyExc_TypeError, "illegal value parameter");
      return NULL;
    }
  }
  
  TRAP(error, {
    CContactItem* templateItem = 
      self->contactsDb->contactDatabase->ReadContactL(self->contactsDb->contactDatabase->TemplateId());
    CleanupStack::PushL(templateItem);
    
    CContactItemFieldSet& templateFields = templateItem->CardFields(); 
    if(templateFields.Count() > fieldInfoIndex){
      field = CContactItemField::NewL(templateFields[fieldInfoIndex]);
    }else{
      userError = 1;
    }
    CleanupStack::PopAndDestroy(templateItem);
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  if(userError == 1){
    PyErr_SetString(PyExc_ValueError, "illegal fieldtype index");
    return NULL;
  }
  
  TRAP(error, {
    CleanupStack::PushL(field);
    if((field->StorageType()==KStorageTypeText) && (value !=NULL)){
      field->TextStorage()->SetTextL(TPtrC((TUint16*)value, valueLength));
    }else if((field->StorageType()==KStorageTypeDateTime) && (dateVal > 0)){
      TTime datetime;
      pythonRealAsTTime(dateVal, datetime);
      field->DateTimeStorage()->SetTime(datetime);
    }else{
      userError = 1; 
    }   
    if(userError == 0){
      if(label!=NULL){    
        field->SetLabelL(TPtrC((TUint16*)PyUnicode_AsUnicode(label),PyUnicode_GetSize(label)));
      }
      self->contactItem->AddFieldL(*field);
      CleanupStack::Pop(field);
    }else{
      CleanupStack::PopAndDestroy(field);
    }
  });
  if(error!=KErrNone){
    return SPyErr_SetFromSymbianOSErr(error);
  }
  
  if(userError == 1){
    PyErr_SetString(PyExc_ValueError, "illegal field type or value");
    return NULL;
  }
  
  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Returns field data of the field indicated by the fieldIndex.
 */
extern "C" PyObject *
Contact_build_field_data_object(Contact_object* self, TInt fieldIndex)
{
  ASSERT_CONTACTOPEN
  PyObject* fieldValue = NULL;
  PyObject* idList = NULL;
  
  CContactItemFieldSet& fieldArray = self->contactItem->CardFields();

  if (fieldIndex < 0 || fieldIndex >= fieldArray.Count()){
    PyErr_SetString(PyExc_IndexError, "illegal field index");
    return NULL; 
  }  

  CContactItemField& field = fieldArray[fieldIndex];
  
  if(KStorageTypeText == field.StorageType()){
    fieldValue = 
      Py_BuildValue("u#", field.TextStorage()->Text().Ptr(), field.TextStorage()->Text().Length()); 
  }else if(KStorageTypeDateTime == field.StorageType()){
    fieldValue = ttimeAsPythonFloat(field.DateTimeStorage()->Time());
  }else{
    fieldValue = Py_BuildValue("u", ""); // binary support ???
  }

  if(fieldValue == NULL){      
    return NULL;
  }
  
  idList = PyList_New(field.ContentType().FieldTypeCount());
  if(idList==NULL){
    Py_DECREF(fieldValue);
    return PyErr_NoMemory();
  }
  for(int i=0;i<field.ContentType().FieldTypeCount();i++){
    PyList_SetItem(idList,i,Py_BuildValue("i",field.ContentType().FieldType(i)));
  }

  PyObject* ret =  
    Py_BuildValue("{s:u#,s:O,s:i,s:i,s:i,s:i,s:O,s:i}",
                  "label",field.Label().Ptr(), field.Label().Length(),
                  "value", fieldValue,
                  "fieldindex", fieldIndex,
                  (const char*)(&KKeyStrFieldId)->Ptr(), 
                    field.Id(),
                  "storagetype", field.StorageType(),
                  "maxlength", KCntMaxTextFieldLength,
                  "field_ids",idList,
                  "vcard_mapping",field.ContentType().Mapping());

  Py_DECREF(fieldValue);
  Py_DECREF(idList);

  return ret;
}


/*
 * Returns indices of the fields that represent given fieldtype. 
 */
extern "C" PyObject *
Contact_get_field(Contact_object* self, PyObject* args)
{  
  TInt fieldIndex = -1;

  if (!PyArg_ParseTuple(args, "i", &fieldIndex)){ 
    return NULL;
  }
  
  return Contact_build_field_data_object(self,fieldIndex);
}


/*
 * Returns the number of fields in the contact object.
 */
extern "C" PyObject *
Contact_num_of_fields(Contact_object* self, PyObject* /*args*/)
{   
  return Py_BuildValue("i", self->contactItem->CardFields().Count());
}


/*
 * Removes specified field of the contact.
 */
extern "C" PyObject *
Contact_remove_field(Contact_object* self, PyObject* args)
{  
  ASSERT_CONTACT_READ_WRITE
  
  TInt fieldIndex = -1;
  
  if (!PyArg_ParseTuple(args, "i", &fieldIndex)){ 
    return NULL;
  }

  CContactItemFieldSet& fieldArray = self->contactItem->CardFields();
  if (fieldIndex < 0 || fieldIndex >= fieldArray.Count()){
    PyErr_SetString(PyExc_IndexError, "illegal field index");
    return NULL; 
  }

  self->contactItem->RemoveField(fieldIndex);

  Py_INCREF(Py_None);
  return Py_None;
}


/*
 * Deallocate Contact_object.
 */
extern "C" {
  static void Contact_dealloc(Contact_object *contact)
  {
    Contact_delete_entry(contact);
    Py_DECREF(contact->contactsDb); 
    PyObject_Del(contact);    
  }
}



/*
 * Field iterator methods.
 */

 

/*
 * Create new FieldIterator object.
 */
extern "C" PyObject *
new_FieldIterator_object(Contact_object* self, PyObject /**args*/)
{ 
  ASSERT_CONTACTOPEN

  FieldIterator_object* fi = 
    PyObject_New(FieldIterator_object, FieldIterator_type);
  if (fi == NULL){
    return PyErr_NoMemory();
  }

  fi->contact = self;
  fi->initialized = false;
  fi->iterationIndex = 0;
  Py_INCREF(fi->contact);
  return (PyObject*)fi; 
}


/*
 * Returns data of the next field of the contact.
 */
extern "C" PyObject *
FieldIterator_next_field(FieldIterator_object* self, PyObject /**args*/)
{  
  ASSERT_CONTACTOPEN_FI

  if (!self->initialized) {
    self->iterationIndex = 0;
    self->initialized = true;   
  }

  CContactItemFieldSet& fieldArray = self->contact->contactItem->CardFields();

  if(self->iterationIndex < fieldArray.Count()){
    PyObject* ret = 
      Contact_build_field_data_object(self->contact, self->iterationIndex);
    self->iterationIndex++;
    return ret;
  }

  PyErr_SetObject(PyExc_StopIteration, Py_None);
  return NULL;
}


/*
 * Deallocate FieldIterator_object.
 */
extern "C" {
  static void FieldIterator_dealloc(FieldIterator_object *fieldIterator)
  {
    Py_DECREF(fieldIterator->contact);
    fieldIterator->contact = NULL;
    PyObject_Del(fieldIterator);  
  }
}


 
extern "C" {
  
  const static PyMethodDef ContactsDb_methods[] = {
    {"get_entry", (PyCFunction)ContactsDb_get_entry, METH_VARARGS},
    {"create_entry", (PyCFunction)ContactsDb_create_entry, METH_NOARGS},
    {"field_type_info", (PyCFunction)ContactsDB_field_type_info, METH_VARARGS, NULL},
    {"field_types", (PyCFunction)ContactsDB_field_types, METH_NOARGS, NULL},
    {"export_vcards", (PyCFunction)ContactsDb_export_vcards, METH_VARARGS},
    {"import_vcards", (PyCFunction)ContactsDb_import_vcards, METH_VARARGS},
    {"delete_entry", (PyCFunction)ContactsDb_delete_entry, METH_VARARGS},
    {"entry_count", (PyCFunction)ContactsDb_entry_count, METH_VARARGS},
    {"find", (PyCFunction)ContactsDb_find, METH_VARARGS},
    {"contact_groups", (PyCFunction)ContactsDb_contact_groups, METH_NOARGS},    
    {"contact_group_label", (PyCFunction)ContactsDb_contact_group_label, METH_VARARGS},   
    {"contact_group_set_label", (PyCFunction)ContactsDb_contact_group_set_label, METH_VARARGS},     
    {"contact_group_ids", (PyCFunction)ContactsDb_contact_group_ids, METH_VARARGS},  
    {"add_contact_to_group", (PyCFunction)ContactsDb_add_contact_to_group, METH_VARARGS},   
    {"remove_contact_from_group", (PyCFunction)ContactsDb_remove_contact_from_group, METH_VARARGS}, 
    {"create_contact_group", (PyCFunction)ContactsDb_create_contact_group, METH_NOARGS},   
    {"contact_group_count", (PyCFunction)ContactsDb_contact_group_count, METH_NOARGS}, 
    {"compact", (PyCFunction)ContactsDb_compact, METH_NOARGS},
    {"compact_recommended", (PyCFunction)ContactsDb_compact_recommended, METH_NOARGS}, 
    {"_determine_field_name", (PyCFunction)ContactsDb_determine_field_name, METH_VARARGS}, 
    {NULL, NULL}  
  };
  
  const static PyMethodDef ContactIterator_methods[] = {
    {"next", (PyCFunction)ContactIterator_next, METH_NOARGS},
    {NULL, NULL}  
  };
  
  const static PyMethodDef Contact_methods[] = {
    {"num_of_fields", (PyCFunction)Contact_num_of_fields, METH_NOARGS, NULL},    
    {"entry_data", (PyCFunction)Contact_entry_data, METH_NOARGS, NULL},
    {"get_field", (PyCFunction)Contact_get_field, METH_VARARGS, NULL},
    {"begin", (PyCFunction)Contact_begin, METH_NOARGS, NULL}, 
    {"commit", (PyCFunction)Contact_commit, METH_NOARGS, NULL}, 
    {"rollback", (PyCFunction)Contact_rollback, METH_NOARGS, NULL}, 
    {"add_field", (PyCFunction)Contact_add_field, METH_VARARGS | METH_KEYWORDS, NULL},
    {"modify_field", (PyCFunction)Contact_modify_field, METH_VARARGS | METH_KEYWORDS, NULL},
    {"find_field_indexes", (PyCFunction)Contact_find_field_indexes, METH_VARARGS, NULL},
    {"remove_field", (PyCFunction)Contact_remove_field, METH_VARARGS, NULL},
    {"is_contact_group", (PyCFunction)Contact_is_contact_group, METH_NOARGS},  
    {NULL, NULL, 0 , NULL}  
  };
  
  const static PyMethodDef FieldIterator_methods[] = {
    {"next", (PyCFunction)FieldIterator_next_field, METH_NOARGS},
    {NULL, NULL}  
  };
  
  
  static PyObject *
  ContactsDb_getattr(ContactsDb_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)ContactsDb_methods, (PyObject *)op, name);
  }
  
  static PyObject *
  ContactIterator_getattr(ContactIterator_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)ContactIterator_methods, (PyObject *)op, name);
  }
  
  static PyObject *
  Contact_getattr(Contact_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)Contact_methods, (PyObject *)op, name);
  }
  
  static PyObject *
  FieldIterator_getattr(FieldIterator_object *op, char *name)
  {
    return Py_FindMethod((PyMethodDef*)FieldIterator_methods, (PyObject *)op, name);
  }

  
  const PyTypeObject c_ContactsDb_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_contacts.ContactsDb",                   /*tp_name*/
    sizeof(ContactsDb_object),                /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)ContactsDb_dealloc,           /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)ContactsDb_getattr,          /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    (getiterfunc)new_ContactIterator_object,  /*tp_iter */
  };
  
  
  static const PyTypeObject c_ContactIterator_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_contacts.ContactIterator",              /*tp_name*/
    sizeof(ContactIterator_object),           /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)ContactIterator_dealloc,      /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)ContactIterator_getattr,     /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    0,                                        /*tp_iter*/  
    (iternextfunc)ContactIterator_next,       /*tp_iternext */
  };
  

  const PyTypeObject c_Contact_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_contacts.Contact",                      /*tp_name*/
    sizeof(Contact_object),                   /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)Contact_dealloc,              /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)Contact_getattr,             /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    (getiterfunc)new_FieldIterator_object,    /*tp_iter */
  };
  
  
  static const PyTypeObject c_FieldIterator_type = {
    PyObject_HEAD_INIT(NULL)
    0,                                        /*ob_size*/
    "_contacts.FieldIterator",                /*tp_name*/
    sizeof(FieldIterator_object),             /*tp_basicsize*/
    0,                                        /*tp_itemsize*/
    (destructor)FieldIterator_dealloc,        /*tp_dealloc*/
    0,                                        /*tp_print*/
    (getattrfunc)FieldIterator_getattr,       /*tp_getattr*/
    0,                                        /*tp_setattr*/
    0,                                        /*tp_compare*/
    0,                                        /*tp_repr*/
    0,                                        /*tp_as_number*/
    0,                                        /*tp_as_sequence*/
    0,                                        /*tp_as_mapping*/
    0,                                        /*tp_hash */
    0,                                        /*tp_call*/
    0,                                        /*tp_str*/
    0,                                        /*tp_getattro*/
    0,                                        /*tp_setattro*/
    0,                                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                       /*tp_flags*/
    "",                                       /*tp_doc */
    0,                                        /*tp_traverse */
    0,                                        /*tp_clear */
    0,                                        /*tp_richcompare */
    0,                                        /*tp_weaklistoffset */
    0,                                        /*tp_iter */
    (iternextfunc)FieldIterator_next_field,   /*tp_iternext */
  };
  
  
} //extern C
 
 
 
 //////////////INIT//////////////


extern "C" {
  static const PyMethodDef contacts_methods[] = {
    {"open", (PyCFunction)open_db, METH_VARARGS, NULL},
#ifdef CONTACTS_MAINTENANCE
    {"_create_api_mappings", (PyCFunction)_create_api_mappings, METH_NOARGS, NULL},
#endif
    {NULL,              NULL}           /* sentinel */
  };

  DL_EXPORT(void) initcontacts(void)
  {
    PyTypeObject* contacts_db_type = PyObject_New(PyTypeObject, &PyType_Type);
    *contacts_db_type = c_ContactsDb_type;
    contacts_db_type->ob_type = &PyType_Type;
    SPyAddGlobalString("ContactsDbType", (PyObject*)contacts_db_type);

    
    PyTypeObject* contact_iterator_type = PyObject_New(PyTypeObject, &PyType_Type);
    *contact_iterator_type = c_ContactIterator_type;
    contact_iterator_type->ob_type = &PyType_Type;
    SPyAddGlobalString("ContactIteratorType", (PyObject*)contact_iterator_type);

    
    PyTypeObject* contact_type = PyObject_New(PyTypeObject, &PyType_Type);
    *contact_type = c_Contact_type;
    contact_type->ob_type = &PyType_Type;
    SPyAddGlobalString("ContactType", (PyObject*)contact_type);

    
    PyTypeObject* field_iterator_type = PyObject_New(PyTypeObject, &PyType_Type);
    *field_iterator_type = c_FieldIterator_type;
    field_iterator_type->ob_type = &PyType_Type;
    SPyAddGlobalString("FieldIteratorType", (PyObject*)field_iterator_type);
   
    
    PyObject *m, *d;

    m = Py_InitModule("_contacts", (PyMethodDef*)contacts_methods);
    d = PyModule_GetDict(m);
    
    
    // Storage types.
    PyDict_SetItemString(d,"storage_type_text", PyInt_FromLong(KStorageTypeText));
    PyDict_SetItemString(d,"storage_type_store", PyInt_FromLong(KStorageTypeStore));
    PyDict_SetItemString(d,"storage_type_contact_item_id", PyInt_FromLong(KStorageTypeContactItemId));
    PyDict_SetItemString(d,"storage_type_datetime", PyInt_FromLong(KStorageTypeDateTime));
        
    // Field types
    PyDict_SetItemString(d,"address_value", PyInt_FromLong(KUidContactFieldAddressValue));
    PyDict_SetItemString(d,"post_office_value", PyInt_FromLong(KUidContactFieldPostOfficeValue));
    PyDict_SetItemString(d,"extended_address_value", PyInt_FromLong(KUidContactFieldExtendedAddressValue));
    PyDict_SetItemString(d,"locality_value", PyInt_FromLong(KUidContactFieldLocalityValue));
    PyDict_SetItemString(d,"post_code_value", PyInt_FromLong(KUidContactFieldPostCodeValue));
    PyDict_SetItemString(d,"company_name_value", PyInt_FromLong(KUidContactFieldCompanyNameValue));
    PyDict_SetItemString(d,"company_name_pronunciation_value", PyInt_FromLong(KUidContactFieldCompanyNamePronunciationValue));
    PyDict_SetItemString(d,"phone_number_value", PyInt_FromLong(KUidContactFieldPhoneNumberValue));
    PyDict_SetItemString(d,"given_name_value", PyInt_FromLong(KUidContactFieldGivenNameValue));
    PyDict_SetItemString(d,"family_name_value", PyInt_FromLong(KUidContactFieldFamilyNameValue));
    PyDict_SetItemString(d,"given_name_pronunciation_value", PyInt_FromLong(KUidContactFieldGivenNamePronunciationValue));
    PyDict_SetItemString(d,"family_name_pronunciation_value", PyInt_FromLong(KUidContactFieldFamilyNamePronunciationValue));
    PyDict_SetItemString(d,"additional_name_value", PyInt_FromLong(KUidContactFieldAdditionalNameValue));
    PyDict_SetItemString(d,"suffix_name_value", PyInt_FromLong(KUidContactFieldSuffixNameValue));
    PyDict_SetItemString(d,"prefix_name_value", PyInt_FromLong(KUidContactFieldPrefixNameValue));  
    PyDict_SetItemString(d,"hidden_value", PyInt_FromLong(KUidContactFieldHiddenValue));
    PyDict_SetItemString(d,"defined_text_value", PyInt_FromLong(KUidContactFieldDefinedTextValue));
    PyDict_SetItemString(d,"email_value", PyInt_FromLong(KUidContactFieldEMailValue));
    PyDict_SetItemString(d,"gsg_value", PyInt_FromLong(KUidContactFieldMsgValue));
    PyDict_SetItemString(d,"sms_value", PyInt_FromLong(KUidContactFieldSmsValue));
    PyDict_SetItemString(d,"fax_value", PyInt_FromLong(KUidContactFieldFaxValue));
    PyDict_SetItemString(d,"note_value", PyInt_FromLong(KUidContactFieldNoteValue));
    PyDict_SetItemString(d,"storage_value", PyInt_FromLong(KUidContactFieldStorageInlineValue));
    PyDict_SetItemString(d,"birthday_value", PyInt_FromLong(KUidContactFieldBirthdayValue));
    PyDict_SetItemString(d,"url_value", PyInt_FromLong(KUidContactFieldUrlValue));
    PyDict_SetItemString(d,"template_label_value", PyInt_FromLong(KUidContactFieldTemplateLabelValue));
    PyDict_SetItemString(d,"picture_value", PyInt_FromLong(KUidContactFieldPictureValue));
    PyDict_SetItemString(d,"dtmf_value", PyInt_FromLong(KUidContactFieldDTMFValue));
    PyDict_SetItemString(d,"ring_tone_value", PyInt_FromLong(KUidContactFieldRingToneValue));
    PyDict_SetItemString(d,"job_title_value", PyInt_FromLong(KUidContactFieldJobTitleValue));
    PyDict_SetItemString(d,"im_address_value", PyInt_FromLong(KUidContactFieldIMAddressValue));
    PyDict_SetItemString(d,"second_name_value", PyInt_FromLong(KUidContactFieldSecondNameValue));
    PyDict_SetItemString(d,"sip_id_value", PyInt_FromLong(KUidContactFieldSIPIDValue));
    PyDict_SetItemString(d,"icc_phonebook_value", PyInt_FromLong(KUidContactFieldICCPhonebookValue));
    PyDict_SetItemString(d,"icc_group_value", PyInt_FromLong(KUidContactFieldICCGroupValue));
    PyDict_SetItemString(d,"voice_dial_value", PyInt_FromLong(KUidContactsVoiceDialFieldValue));
    PyDict_SetItemString(d,"none_value", PyInt_FromLong(KUidContactFieldNoneValue));
    PyDict_SetItemString(d,"match_all_value", PyInt_FromLong(KUidContactFieldMatchAllValue));
    PyDict_SetItemString(d,"country_value", PyInt_FromLong(KUidContactFieldCountryValue));
    PyDict_SetItemString(d,"region_value", PyInt_FromLong(KUidContactFieldRegionValue));
     
    // Field mappings
    PyDict_SetItemString(d,"map_post_office", PyInt_FromLong(KIntContactFieldVCardMapPOSTOFFICE));
    PyDict_SetItemString(d,"map_extended_address", PyInt_FromLong(KIntContactFieldVCardMapEXTENDEDADR));
    PyDict_SetItemString(d,"map_adr", PyInt_FromLong(KIntContactFieldVCardMapADR));
    PyDict_SetItemString(d,"map_locality", PyInt_FromLong(KIntContactFieldVCardMapLOCALITY));
    PyDict_SetItemString(d,"map_region", PyInt_FromLong(KIntContactFieldVCardMapREGION));
    PyDict_SetItemString(d,"map_postcode", PyInt_FromLong(KIntContactFieldVCardMapPOSTCODE));
    PyDict_SetItemString(d,"map_country", PyInt_FromLong(KIntContactFieldVCardMapCOUNTRY));
    PyDict_SetItemString(d,"map_agent", PyInt_FromLong(KIntContactFieldVCardMapAGENT));
    PyDict_SetItemString(d,"map_bday", PyInt_FromLong(KIntContactFieldVCardMapBDAY));
    PyDict_SetItemString(d,"map_email_internet", PyInt_FromLong(KIntContactFieldVCardMapEMAILINTERNET));
    PyDict_SetItemString(d,"map_geo", PyInt_FromLong(KIntContactFieldVCardMapGEO));
    PyDict_SetItemString(d,"map_label", PyInt_FromLong(KIntContactFieldVCardMapLABEL));
    PyDict_SetItemString(d,"map_logo", PyInt_FromLong(KIntContactFieldVCardMapLOGO));
    PyDict_SetItemString(d,"map_mailer", PyInt_FromLong(KIntContactFieldVCardMapMAILER));
    PyDict_SetItemString(d,"map_note", PyInt_FromLong(KIntContactFieldVCardMapNOTE));
    PyDict_SetItemString(d,"map_org", PyInt_FromLong(KIntContactFieldVCardMapORG));
    PyDict_SetItemString(d,"map_org_pronounciation", PyInt_FromLong(KIntContactFieldVCardMapORGPronunciation));
    PyDict_SetItemString(d,"map_photo", PyInt_FromLong(KIntContactFieldVCardMapPHOTO));
    PyDict_SetItemString(d,"map_role", PyInt_FromLong(KIntContactFieldVCardMapROLE));
    PyDict_SetItemString(d,"map_sound", PyInt_FromLong(KIntContactFieldVCardMapSOUND));
    PyDict_SetItemString(d,"map_tel", PyInt_FromLong(KIntContactFieldVCardMapTEL));
    PyDict_SetItemString(d,"map_telfax", PyInt_FromLong(KIntContactFieldVCardMapTELFAX));
    PyDict_SetItemString(d,"map_title", PyInt_FromLong(KIntContactFieldVCardMapTITLE));
    PyDict_SetItemString(d,"map_url", PyInt_FromLong(KIntContactFieldVCardMapURL));
    PyDict_SetItemString(d,"map_unusedn", PyInt_FromLong(KIntContactFieldVCardMapUnusedN));
    PyDict_SetItemString(d,"map_unusedfn", PyInt_FromLong(KIntContactFieldVCardMapUnusedFN));
    PyDict_SetItemString(d,"map_not_required", PyInt_FromLong(KIntContactFieldVCardMapNotRequired));
    PyDict_SetItemString(d,"map_unknown_xdash", PyInt_FromLong(KIntContactFieldVCardMapUnknownXDash));
    PyDict_SetItemString(d,"map_unknown", PyInt_FromLong(KIntContactFieldVCardMapUnknown));
    PyDict_SetItemString(d,"map_uid", PyInt_FromLong(KIntContactFieldVCardMapUID));
    PyDict_SetItemString(d,"map_work", PyInt_FromLong(KIntContactFieldVCardMapWORK));
    PyDict_SetItemString(d,"map_home", PyInt_FromLong(KIntContactFieldVCardMapHOME));
    PyDict_SetItemString(d,"map_msg", PyInt_FromLong(KIntContactFieldVCardMapMSG));
    PyDict_SetItemString(d,"map_voice", PyInt_FromLong(KIntContactFieldVCardMapVOICE));
    PyDict_SetItemString(d,"map_fax", PyInt_FromLong(KIntContactFieldVCardMapFAX));
    PyDict_SetItemString(d,"map_pref", PyInt_FromLong(KIntContactFieldVCardMapPREF));
    PyDict_SetItemString(d,"map_cell", PyInt_FromLong(KIntContactFieldVCardMapCELL));
    PyDict_SetItemString(d,"map_pager", PyInt_FromLong(KIntContactFieldVCardMapPAGER));
    PyDict_SetItemString(d,"map_bbs", PyInt_FromLong(KIntContactFieldVCardMapBBS));
    PyDict_SetItemString(d,"map_modem", PyInt_FromLong(KIntContactFieldVCardMapMODEM));
    PyDict_SetItemString(d,"map_car", PyInt_FromLong(KIntContactFieldVCardMapCAR));
    PyDict_SetItemString(d,"map_isdn", PyInt_FromLong(KIntContactFieldVCardMapISDN));
    PyDict_SetItemString(d,"map_video", PyInt_FromLong(KIntContactFieldVCardMapVIDEO));
    PyDict_SetItemString(d,"map_dom", PyInt_FromLong(KIntContactFieldVCardMapDOM));
    PyDict_SetItemString(d,"map_intl", PyInt_FromLong(KIntContactFieldVCardMapINTL));
    PyDict_SetItemString(d,"map_postal", PyInt_FromLong(KIntContactFieldVCardMapPOSTAL));
    PyDict_SetItemString(d,"map_parcel", PyInt_FromLong(KIntContactFieldVCardMapPARCEL));
    PyDict_SetItemString(d,"map_gif", PyInt_FromLong(KIntContactFieldVCardMapGIF));
    PyDict_SetItemString(d,"map_cgm", PyInt_FromLong(KIntContactFieldVCardMapCGM));
    PyDict_SetItemString(d,"map_wmf", PyInt_FromLong(KIntContactFieldVCardMapWMF));
    PyDict_SetItemString(d,"map_bmp", PyInt_FromLong(KIntContactFieldVCardMapBMP));
    PyDict_SetItemString(d,"map_met", PyInt_FromLong(KIntContactFieldVCardMapMET));
    PyDict_SetItemString(d,"map_pmb", PyInt_FromLong(KIntContactFieldVCardMapPMB));
    PyDict_SetItemString(d,"map_dib", PyInt_FromLong(KIntContactFieldVCardMapDIB));
    PyDict_SetItemString(d,"map_pict", PyInt_FromLong(KIntContactFieldVCardMapPICT));
    PyDict_SetItemString(d,"map_tiff", PyInt_FromLong(KIntContactFieldVCardMapTIFF));
    PyDict_SetItemString(d,"map_pdf", PyInt_FromLong(KIntContactFieldVCardMapPDF));
    PyDict_SetItemString(d,"map_ps", PyInt_FromLong(KIntContactFieldVCardMapPS));
    PyDict_SetItemString(d,"map_jpeg", PyInt_FromLong(KIntContactFieldVCardMapJPEG));
    PyDict_SetItemString(d,"map_mpeg", PyInt_FromLong(KIntContactFieldVCardMapMPEG));
    PyDict_SetItemString(d,"map_mpeg2", PyInt_FromLong(KIntContactFieldVCardMapMPEG2));
    PyDict_SetItemString(d,"map_avi", PyInt_FromLong(KIntContactFieldVCardMapAVI));
    PyDict_SetItemString(d,"map_qtime", PyInt_FromLong(KIntContactFieldVCardMapQTIME));
    PyDict_SetItemString(d,"map_tz", PyInt_FromLong(KIntContactFieldVCardMapTZ));
    PyDict_SetItemString(d,"map_key", PyInt_FromLong(KIntContactFieldVCardMapKEY));
    PyDict_SetItemString(d,"map_x509", PyInt_FromLong(KIntContactFieldVCardMapX509));
    PyDict_SetItemString(d,"map_pgp", PyInt_FromLong(KIntContactFieldVCardMapPGP));
    PyDict_SetItemString(d,"map_smime", PyInt_FromLong(KIntContactFieldVCardMapSMIME));
    PyDict_SetItemString(d,"map_wv", PyInt_FromLong(KIntContactFieldVCardMapWV));
    PyDict_SetItemString(d,"map_second_name", PyInt_FromLong(KIntContactFieldVCardMapSECONDNAME));
    PyDict_SetItemString(d,"map_sip_id", PyInt_FromLong(KIntContactFieldVCardMapSIPID));
    PyDict_SetItemString(d,"map_poc", PyInt_FromLong(KIntContactFieldVCardMapPOC));
    PyDict_SetItemString(d,"map_swis", PyInt_FromLong(KIntContactFieldVCardMapSWIS));
    PyDict_SetItemString(d,"map_voip", PyInt_FromLong(KIntContactFieldVCardMapVOIP));

    // phonebook field types
    // pbkfields.hrh
    PyDict_SetItemString(d,"none", PyInt_FromLong(EPbkFieldIdNone));
    PyDict_SetItemString(d,"last_name", PyInt_FromLong(EPbkFieldIdLastName));
    PyDict_SetItemString(d,"first_name", PyInt_FromLong(EPbkFieldIdFirstName));
    PyDict_SetItemString(d,"phone_number_general", PyInt_FromLong(EPbkFieldIdPhoneNumberGeneral));
    PyDict_SetItemString(d,"phone_number_standard", PyInt_FromLong(EPbkFieldIdPhoneNumberStandard));
    PyDict_SetItemString(d,"phone_number_home", PyInt_FromLong(EPbkFieldIdPhoneNumberHome));
    PyDict_SetItemString(d,"phone_number_work", PyInt_FromLong(EPbkFieldIdPhoneNumberWork));
    PyDict_SetItemString(d,"phone_number_mobile", PyInt_FromLong(EPbkFieldIdPhoneNumberMobile));
    PyDict_SetItemString(d,"fax_number", PyInt_FromLong(EPbkFieldIdFaxNumber));
    PyDict_SetItemString(d,"pager_number", PyInt_FromLong(EPbkFieldIdPagerNumber));
    PyDict_SetItemString(d,"email_address", PyInt_FromLong(EPbkFieldIdEmailAddress));
    PyDict_SetItemString(d,"postal_address", PyInt_FromLong(EPbkFieldIdPostalAddress));
    PyDict_SetItemString(d,"url", PyInt_FromLong(EPbkFieldIdURL));
    PyDict_SetItemString(d,"job_title", PyInt_FromLong(EPbkFieldIdJobTitle));
    PyDict_SetItemString(d,"company_name", PyInt_FromLong(EPbkFieldIdCompanyName));
    PyDict_SetItemString(d,"company_address", PyInt_FromLong(EPbkFieldIdCompanyAddress));
    PyDict_SetItemString(d,"dtmf_string", PyInt_FromLong(EPbkFieldIdDTMFString));
    PyDict_SetItemString(d,"date", PyInt_FromLong(EPbkFieldIdDate));
    PyDict_SetItemString(d,"note", PyInt_FromLong(EPbkFieldIdNote));
    PyDict_SetItemString(d,"po_box", PyInt_FromLong(EPbkFieldIdPOBox));
    PyDict_SetItemString(d,"extended_address", PyInt_FromLong(EPbkFieldIdExtendedAddress));
    PyDict_SetItemString(d,"street_address", PyInt_FromLong(EPbkFieldIdStreetAddress));
    PyDict_SetItemString(d,"postal_code", PyInt_FromLong(EPbkFieldIdPostalCode));
    PyDict_SetItemString(d,"city", PyInt_FromLong(EPbkFieldIdCity));
    PyDict_SetItemString(d,"state", PyInt_FromLong(EPbkFieldIdState));
    PyDict_SetItemString(d,"country", PyInt_FromLong(EPbkFieldIdCountry));
    PyDict_SetItemString(d,"wvid", PyInt_FromLong(EPbkFieldIdWVID));
    
    // Location id:s.
    PyDict_SetItemString(d,"location_none", PyInt_FromLong(EPbkFieldLocationNone));
    PyDict_SetItemString(d,"location_home", PyInt_FromLong(EPbkFieldLocationHome));
    PyDict_SetItemString(d,"location_work", PyInt_FromLong(EPbkFieldLocationWork)); 
    
    // vcard options.
    PyDict_SetItemString(d,"vcard_include_x", PyInt_FromLong(CContactDatabase::EIncludeX));
    PyDict_SetItemString(d,"vcard_ett_format", PyInt_FromLong(CContactDatabase::ETTFormat));
    PyDict_SetItemString(d,"vcard_exclude_uid", PyInt_FromLong(CContactDatabase::EExcludeUid));
    PyDict_SetItemString(d,"vcard_dec_access_count", PyInt_FromLong(CContactDatabase::EDecreaseAccessCount));
    PyDict_SetItemString(d,"vcard_import_single_contact", PyInt_FromLong(CContactDatabase::EImportSingleContact));
    PyDict_SetItemString(d,"vcard_inc_access_count", PyInt_FromLong(CContactDatabase::EIncreaseAccessCount));
    
    // additional phonebook field types
    PyDict_SetItemString(d,"picture", PyInt_FromLong(EPbkFieldIdPicture));
    PyDict_SetItemString(d,"thumbnail_image", PyInt_FromLong(EPbkFieldIdThumbnailImage));
    PyDict_SetItemString(d,"voice_tag_indication", PyInt_FromLong(EPbkFieldIdVoiceTagIndication));
    PyDict_SetItemString(d,"speed_dial_indication", PyInt_FromLong(EPbkFieldIdSpeedDialIndication));
    PyDict_SetItemString(d,"ringing_tone_indication", PyInt_FromLong(EPbkFieldIdPersonalRingingToneIndication));
    PyDict_SetItemString(d,"second_name", PyInt_FromLong(EPbkFieldIdSecondName));
    PyDict_SetItemString(d,"phone_number_video", PyInt_FromLong(EPbkFieldIdPhoneNumberVideo));
    PyDict_SetItemString(d,"last_name_reading", PyInt_FromLong(EPbkFieldIdLastNameReading));
    PyDict_SetItemString(d,"first_name_reading", PyInt_FromLong(EPbkFieldIdFirstNameReading));
    PyDict_SetItemString(d,"location_id_indication", PyInt_FromLong(EPbkFieldIdLocationIdIndication));
    PyDict_SetItemString(d,"voip", PyInt_FromLong(EPbkFieldIdVOIP));
    PyDict_SetItemString(d,"push_to_talk", PyInt_FromLong(EPbkFieldIdPushToTalk));
    PyDict_SetItemString(d,"share_view", PyInt_FromLong(EPbkFieldIdShareView));
    PyDict_SetItemString(d,"sip_id", PyInt_FromLong(EPbkFieldIdSIPID));
    PyDict_SetItemString(d,"cod_text_id", PyInt_FromLong(EPbkFieldIdCodTextID));
    PyDict_SetItemString(d,"cod_image_id", PyInt_FromLong(EPbkFieldIdCodImageID));
    PyDict_SetItemString(d,"prefix", PyInt_FromLong(EPbkFieldIdPrefix));
    PyDict_SetItemString(d,"suffix", PyInt_FromLong(EPbkFieldIdSuffix));
    PyDict_SetItemString(d,"mask", PyInt_FromLong(KPbkFieldIdMask));
    
    return;
  }
} /* extern "C" */
