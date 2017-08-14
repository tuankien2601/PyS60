#
# contacts_maintenance.py
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


# create dictionary for converting field types between contacts and phonebook apis.
# format: key represents field identification data in the contacts api and value
# represents field identification data in the phonebook api.
# {((id_0, .., id_n), vcard_mapping):(id, location), ..}
#
# note that to access the _create_api_mappings()-method the contacts C-module must be 
# compiled with CONTACTS_MAINTENANCE flag. 
# 
import contacts

mappings = contacts._contacts._create_api_mappings()

typedict =  {}
for item in mappings:
    typedict[(tuple(item['ids']),item['vcard_mapping'])] = (item['fieldid'],item['fieldlocation'])    

mappings_file_name = "c:\\mappings.txt"
api_mappings_file = open(mappings_file_name, 'wb')
api_mappings_file.write(str(typedict))
api_mappings_file.close()


