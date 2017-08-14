#
# README.TXT
#
# Copyright (c) 2006-2007 Nokia Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


*******************ABOUT CONTACTS FOR 3.0 SDK***********************

-about compatibility:
 the downward compatibility requirements made python contacts API for 3.0 SDK implementation much 
 more complex. however, python contacts api is used a lot and keeping the new API as much compatible 
 as possible is probably worth of some effort.

-_api_mappings dictionary in contacts.py is used for converting Symbian contacts api field types to 
 Nokia phonebook field types. in the Nokia phonebook c++ API field type is identified by type id and
 location id (both are integers, location can only have 3 values representing 'none','home', or 'work').
 in the Symbian contacts c++ API list of field ids is used to identificate field type (usually one to three
 integer ids, but as far as i know the number of ids is not limited).
 Now, with the Symbian c++ api you can create a field with any combination of ids, but to make the field
 appear in the Nokia Phonebook application you have to use some known combination of ids and also set some 
 vcard mapping (integer) to the field. using valid combinations of ids and vcard mapping you are actually
 adding similar fields as if you would use Nokia phonebook api. Nokia phonebook api seems to use
 the database template entry in its operations and it also seems that all the field types used by Nokia 
 phonebook API are actually defined in the database template entry (template entry's fields seem to define
 the field types the Nokia phonebook API uses). in short: with Symbian contacts API a list of ids and vcard 
 mapping equal to id and location in Nokia phonebook API.
 
 The _api_mappings - dictionary is in fact a mapping of Symbian Contacts API field types to Nokia phonebook
 API field types (only subset of Symbian's field types is included because their number is practically unlimited
 and we have no use for types that are not recognized by Nokia phonebook and phonebook API). 

 To understand the exact structure of _api_mappings dict, see below:
 _api_mappings = {((symbian_field_id_1,), vcard_mapping): (nokia_field_id, location), ..}
 _api_mappings = {((268440333,), 268451878): (13, 0), ((268440334, 268451441, 268450266), 268451882): (6, 2),..}

-generating api mappings:
 you can generate the _api_mappings dictionary with contacts_maintenance.py - script (note that you must also
 compile the contacts c-module with CONTACTS_MAINTENANCE-flag defined and you must use proper certificate to
 execute the contacts_maintenance.py - script on the phone [ReadDeviceData WriteDeviceData capabilities are
 needed because of Nokia phonebook API]). 

-"broken" fields:
 There may also be fields that do not completely match any Phonebook field type, but are still recognized by 
 Phonebook application. In this case we cannot easily determine the corresponding field from template. 
 The _determine_field_name - method in the contacts c-module is used to find the default label of this kind
 of field. The label is then used to find the matching template field.
 
 
 
 
 
 
 



