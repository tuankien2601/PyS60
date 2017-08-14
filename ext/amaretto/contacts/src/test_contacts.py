# Copyright (c) 2005-2009 Nokia Corporation
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

import test.test_support
import unittest
import contacts
import time


class TestContacts(unittest.TestCase):
    firstname = "Harry"
    lastname = "Potter"
    date = time.time()
    mobile_home = "12345"
    mobile_work = "56789"
    country = "India"
    url = "www.nokia.com"

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)

    def setUp(self):
        self.db = contacts.open('c:test_db', 'n')

    def tearDown(self):
        self.db = contacts.open('c:test_db', 'c')

        for id in self.db:
            del self.db[id]
        del self.db

    def add_contacts(self):
        # Function for adding contacts to the database.
        contact_object = self.db.add_contact()
        contact_object.add_field('first_name', value=self.firstname)
        contact_object.add_field('last_name', self.lastname)
        contact_object.add_field('date', self.date)
        contact_object.add_field('mobile_number', self.mobile_work, 'work')
        contact_object.add_field(type='mobile_number', value=self.mobile_home,
                                 location='home')
        contact_object.commit()
        return contact_object

    def test_add_entry(self):
        contact_obj = self.add_contacts()
        contact_ids = self.db.keys()
        if not contact_obj.id in contact_ids:
            self.fail("Adding of contact to default DB unsuccessful")
        for obj in self.db:
            if self.db[obj].id == contact_obj.id:
                for field in self.db[obj]:
                    if field.type == 'first_name':
                        self.failUnlessEqual(field.value, self.firstname,
                                             'first_name not added properly')
                    elif field.type == 'last_name':
                        self.failUnlessEqual(field.value, self.lastname,
                                             'last_name not added properly')
                    elif field.type == 'mobile_number' and \
                         field.location == 'work':
                        self.failUnlessEqual(field.value, self.mobile_work,
                            'mobile_number not added properly')
                    elif field.type == 'mobile_number' and \
                         field.location == 'home':
                        self.failUnlessEqual(field.value, self.mobile_home,
                            'mobile_number not added properly')
                    else:
                        continue

    def test_schema(self):
        # check if schema of a contact field raises exception if that field
        # is not present.
        contact_obj = self.add_contacts()
        try:
            contact_obj.find('company_name')[0].schema
        except Exception, err:
            self.failUnlessEqual("list index out of range", str(err))

    def test_update_existing(self):
        # Testing if the contact has been added successfully. Then
        # update the same contact by adding some fields and verify the same.
        contact_obj = self.add_contacts()
        # A contact is added. So is_group should return False
        self.failUnlessEqual(contact_obj.is_group, False,
                             "Contact was added, not a group")

        # Search for the mobile number entered and check whether this contact
        # is present in the database.
        contact_list = self.db.find('12345')
        self.failUnless(len(contact_list), "Contact not added successfully")
        # Add some field to the same database
        contact_obj.add_field('country', value=self.country)
        contact_obj.add_field('url', value=self.url)
        # Check if the newly added fields are updated.
        for obj in self.db:
            if self.db[obj].id == contact_obj.id:
                for field in self.db[obj]:
                    if field.type == 'country' and \
                       field.location == 'work':
                        self.failUnlessEqual(field.value, self.country,
                                             'county not updated properly')
                    elif field.type == 'url' and \
                         field.location == 'home':
                        self.failUnlessEqual(field.value, self.url,
                                             'url not updated properly')
                    else:
                        continue

    def test_compact(self):
        # Even if compact_required() returns False compact() shouldn't raise
        # an Exception
        contact_obj = self.add_contacts()
        if not self.db.compact_required():
            try:
                self.db.compact()
            except:
                self.fail("Compacting should be done irrespective of \
                           recommendation for compacting")

    def test_import_export_vcard(self):
        # Testing importing and exporting vcard.
        contact_obj = self.add_contacts()
        # Export all the contacts in db database to vcards
        vcards = self.db.export_vcards(tuple(self.db.keys()))
        # Open a temporary database to which the contacts will be imported.
        temp_db = contacts.open('db_temp', 'n')
        # Import the same vcard into temp_db database and check if the fields
        # are matching in their values
        temp_db.import_vcards(vcards)
        db_id = self.db.keys()[0]
        try:
            for ids in temp_db.keys():
                if ids == db_id:
                    if not (temp_db[ids].find('first_name')[0].value ==
                        self.db[db_id].find('first_name')[0].value and
                        temp_db[ids].find('last_name')[0].value ==
                        self.db[db_id].find('last_name')[0].value and
                        temp_db[ids].find('mobile_number', 'home')[0].value ==
                        self.db[db_id].find('mobile_number', 'home')[0].value
                        and
                        temp_db[ids].find('mobile_number', 'work')[0].value ==
                        self.db[db_id].find('mobile_number', 'work')[0].value):
                        self.fail("Import and export functionality \
                                   not working")
        finally:
            for id in temp_db:
                del temp_db[id]
            del temp_db

    def test_autocommit(self):
        # Checking the autocommit functionality.
        contact_obj = self.add_contacts()
        # Get first of contact's 'first_name' fields.
        field = contact_obj.find('first_name')[0]
        # change the first name.
        field.value = 'Jack'
        # autocommit is on
        self.failUnlessEqual(self.db[contact_obj.id][0].value, 'Jack',
                             "Autocommit not working")
        contact_obj.begin()
        field.value = 'John'
        # autocommit is off
        self.failIfEqual(self.db[contact_obj.id][0].value, 'John',
                             "Autocommit not working")
        # Now the changes are saved.
        contact_obj.commit()
        self.failUnlessEqual(self.db[contact_obj.id][0].value, 'John',
                             "Autocommit not working")
        field.value = 'Henry'
        # autocommit is on again
        self.failUnlessEqual(self.db[contact_obj.id][0].value, 'Henry',
                         "Autocommit not working")

    def test_group_operations(self):
        # Testing operations on group.
        for grp_id in self.db.groups:
            del self.db.groups[grp_id]
        # create group and set name
        test_grp1 = self.db.groups.add_group("test group 1")
        # create group without name
        test_grp2 = self.db.groups.add_group()
        # set name
        test_grp2.name = "test group 2"
        test_grp3 = self.db.groups.add_group("test group 3")
        # rename
        test_grp3.name = "test group 4"
        self.failUnlessEqual(self.db[test_grp1.id].is_group, True,
                             "Should have been a group")
        list_of_ids = []
        for i in self.db.groups:
            list_of_ids.append(self.db.groups[i].id)
        if not (test_grp1.id in list_of_ids and test_grp2.id in list_of_ids and
                test_grp3.id in list_of_ids):
            self.fail("Group id not found")


def test_main():
    test.test_support.run_unittest(TestContacts)


if __name__ == "__main__":
    test_main()
