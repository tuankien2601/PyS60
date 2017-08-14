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

import e32dbm
import os
import random
import unittest
import test.test_support


class E32DbmTest(unittest.TestCase):

    db_file = u'c:\\data\\testdb'

    def __init__(self, method_name='runTest'):
        unittest.TestCase.__init__(self, methodName=method_name)
        if os.path.isfile(self.db_file + ".e32dbm"):
            os.remove(self.db_file + ".e32dbm")

    def get_data(self, datasize):
        testdata = {}
        for k in range(datasize):
            testdata['key%d' % k] = 'value%07d' % \
                                    int(random.random() * 10000000)
        return testdata

    def get_word(self, length=8):
        result = []
        for k in range(length):
            result.append(random.choice('abcdefghijklmnopqrstuvwxyz'))
        return ''.join(result)

    def db_torture_test(self, db, datasize, no_of_operations):
        # Insert a given number of random data into the given db, and
        # then perform the given number of random updates, deletes, inserts,
        # syncs and reorganizes on it. At the end, check that the database
        # contents match with the contents of a normal dictionary where the
        # same updates were made.
        for k in db.keys():
            del db[k]
        temp_data = self.get_data(datasize)
        for k in temp_data:
            db[k] = temp_data[k]
        wordlength = 8

        for i in range(no_of_operations):
            r = random.random()
            if r < 0.20:  # insert
                temp_key = self.get_word()
                temp_value = self.get_word()
                db[temp_key] = temp_value
                temp_data[temp_key] = temp_value
            elif r < 0.4:  # delete
                key = random.choice(temp_data.keys())
                del db[key]
                del temp_data[key]
            elif r < 0.95:  # update
                temp_key_1 = random.choice(temp_data.keys())
                temp_value_1 = self.get_word()
                db[temp_key_1] = temp_value_1
                temp_data[temp_key_1] = temp_value_1
            elif r < 0.975:  # reorganize
                if hasattr(db, 'reorganize'):
                    db.reorganize()
            else:  # sync
                if hasattr(db, 'sync'):
                    db.sync()
        dbcontent = {}
        for k in db.keys():
            dbcontent[k] = db.__getitem__(k)
        self.assertEqual(dbcontent, temp_data)

    def test_db_reset(self):
        db = e32dbm.open(self.db_file, 'cf')
        db['a'] = 'b'
        db['foo'] = 'bar'
        db['foo'] = 'baz'
        self.assertEqual(db['a'], 'b')
        self.assertEqual(db['foo'], 'baz')
        db.close()

    def test_db_delete(self):
        db = e32dbm.open(self.db_file, 'c')
        db['foo'] = 'baz'
        del db['foo']
        self.failIf('foo' in db, "Deleted key must not exist")
        db['foo'] = 'bar'
        self.assert_('foo' in db, "Key must exist after inserting it back")
        db.close()

    def test_db_update_clear(self):
        db = e32dbm.open(self.db_file, 'n')
        db.reorganize()
        temp_data_2 = self.get_data(10)
        db.update(temp_data_2)
        self.assertEqual(db.__len__(), len(temp_data_2),
                         "db len must match temp_data len")
        db.sync()
        self.assertEqual(len(db), 10,
                         "Database does not have all the entries")
        db.clear()
        self.assertEqual(len(db), 0, "Length must be 0 after clear")
        db.close()
        db = e32dbm.open(self.db_file, 'r')
        self.assertRaises(IOError, lambda:db.__setitem__('foo2', 'bar2'))
        db.close()

    def test_db_dict_operations(self):
        db = e32dbm.open(self.db_file, 'nf')

        temp_data_3 = self.get_data(10)
        db.update(temp_data_3)

        temp_keys = temp_data_3.keys()
        temp_keys.sort()
        temp_values = [temp_data_3[x] for x in temp_keys]

        db_keys = list(db.iterkeys())
        db_keys.sort()
        self.assertEqual(db_keys, temp_keys)

        temp_db_values = list(db.itervalues())
        temp_db_values.sort()
        temp_values_sorted = list(temp_values)
        temp_values_sorted.sort()
        self.assertEqual(temp_db_values, temp_values_sorted)

        db_items = db.items()
        self.assertEqual(dict(db_items), temp_data_3)

        db_values = db.values()
        db_values.sort()
        self.assertEqual(db_values, temp_values_sorted)

        db_keys_1 = db.keys()
        db_keys_1.sort()
        self.assertEqual(db_keys_1, temp_keys)

        temp_key = temp_keys[0]
        temp_value = temp_values[0]
        self.assertEqual(db.pop(temp_key), temp_value)
        self.failIf(temp_key in db, "Key must not exist after pop")
        temp_item = db.popitem()
        self.failIf(temp_item[0] in db, "Key must not exist after popitem")

        db['foo1'] = 'bar'
        del db['foo1']
        foo_val = db.setdefault('foo1', 'baz')
        self.assertEqual(foo_val, 'baz')
        self.assertEqual(db['foo1'], 'baz')
        del db['foo1']

        db_keys_2 = []
        try:
            iter_obj = db.__iter__()
            while 1:
                db_keys_2.append(iter_obj.next())
        except StopIteration:
            pass
        self.assertEqual(db_keys_2, db.keys())

        db_items_2 = []
        try:
            iteritems_obj = db.iteritems()
            while 1:
                db_items_2.append(iteritems_obj.next())
        except StopIteration:
            pass
        self.assertEqual(db_items_2, db.items())

        db.close()

    def test_db_torture(self):
        db = e32dbm.open(self.db_file, 'w')
        self.db_torture_test(db, 100, 100)
        db.close()


def test_main():
    test.test_support.run_unittest(E32DbmTest)


if __name__ == "__main__":
    test_main()
