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

import e32db
import unittest
import test.test_support


class E32DbTest(unittest.TestCase):

    def __init__(self, method_name='runTest'):
        unittest.TestCase.__init__(self, methodName=method_name)
        self.db_file = u'c:\\data\\temp_db.db'
        self.db = e32db.Dbms()
        self.db.create(self.db_file)

    def setUp(self):
        self.db.open(self.db_file)

    def tearDown(self):
        self.db.close()

    def test_autocommit_rollback(self):
        query_output_expected = ([(6, )], [5, 1, "'\\x06\\x00\\x00\\x00'"])
        self.db.begin()
        self.db.execute(u'create table t100 (x integer)')
        self.db.execute(u'insert into t100 values (6)')
        self.db.commit()
        query_output = self.query(u'select * from t100')
        self.assertEqual(query_output, query_output_expected)

        self.db.begin()
        self.db.execute(u'create table t101 (x integer)')
        # Execute an invalid statement and then rollback the transaction
        try:
            self.db.execute(u'insert into t101 val (6)')
        except:
            self.db.rollback()
        else:
            self.fail("Invalid arguments did not raise exception")
        finally:
            # Verify that the table 101 is deleted as a result of rollback
            try:
                query_output = self.query(u'select * from t101')
            except SymbianError, e:
                self.assertEqual(str(e), "[Errno -1] KErrNotFound")

    def query(self, statement):
        self.db_view = e32db.Db_view()
        self.db_view.prepare(self.db, unicode(statement))
        no_of_rows = self.db_view.count_line()
        self.db_view.first_line()
        data = []
        for row_no in xrange(no_of_rows):
            self.db_view.get_line()
            line = []
            # Will contain col_type, col_raw, col_rawtime, format_rawtime
            misc_testdata = []
            for column_no in xrange(1, self.db_view.col_count() + 1):
                line.append(self.db_view.col(column_no))
                column_type = self.db_view.col_type(column_no)
                misc_testdata.append(column_type)
                if not self.db_view.is_col_null(column_no):
                    misc_testdata.append(self.db_view.col_length(column_no))
                if column_type != 15:
                    misc_testdata.append(repr(self.db_view.col_raw(column_no)))
                if column_type == 10:
                    misc_testdata.append(
                                     repr(self.db_view.col_rawtime(column_no)))
                    misc_testdata.append(repr(e32db.format_rawtime(
                                         self.db_view.col_rawtime(column_no))))
            data.append(tuple(line))
            self.db_view.next_line()
        del self.db_view
        return data, misc_testdata

    def test_query(self):
        query_output_expected = ([(u'xxxxxxxxxx', 42)],
                                 [15, 10, 5, 1, "'*\\x00\\x00\\x00'"])
        self.db.execute(u'create table data (a long varchar, b integer)')
        self.db.execute(u"insert into data values('" + ('x' * 10) + "', 42)")
        query_output = self.query(u'select * from data')
        self.assertEqual(query_output_expected, query_output)

    def test_data_types(self):
        data_types_testdata = \
              (('bit', '1'),
               ('tinyint', '2'),
               ('unsigned tinyint', '3'),
               ('smallint', '4'),
               ('unsigned smallint', '5'),
               ('integer', '6'),
               ('unsigned integer', '7'),
               ('counter', '8'),
               ('bigint', str(2**60)),
               ('real', '4.224'),
               ('float', '5.784'),
               ('double', '4.135'),
               ('double precision', '2.5664'),
               ('timestamp', "#20-oct/2004#"),
               ('time', "#20-oct/2004#"),
               ('date', "#20-oct/2004#"),
               ('date', "#%s#" % e32db.format_time(0)),
               ('date', "#%s#" % e32db.format_time(12345678)),
               ('char(8)', "'foo'"),
               ('varchar(8)', "'bar'"),
               ('long varchar', "'baz'"))

        # Format : ([(<query output>)], [misc_testdata])
        # where misc_testdata has : col_type, col_length, col_raw values.
        # For the time and date formats output of col_rawtime and
        # format_rawtime values are also appended.
        query_expected_output = \
           (([(1, )], [0, 1, "'\\x01\\x00\\x00\\x00'"]),
            ([(2, )], [1, 1, "'\\x02\\x00\\x00\\x00'"]),
            ([(3, )], [2, 1, "'\\x03\\x00\\x00\\x00'"]),
            ([(4, )], [3, 1, "'\\x04\\x00\\x00\\x00'"]),
            ([(5, )], [4, 1, "'\\x05\\x00\\x00\\x00'"]),
            ([(6, )], [5, 1, "'\\x06\\x00\\x00\\x00'"]),
            ([(7, )], [6, 1, "'\\x07\\x00\\x00\\x00'"]),
            ([(8, )], [6, 1, "'\\x08\\x00\\x00\\x00'"]),
            ([(1152921504606846976L, )],
             [7, 1, "'\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x10'"]),
            ([(4.2239999771118164,)], [8, 1, "'\\x02+\\x87@'"]),
            ([(5.784, )], [9, 1, '\'\\x89A`\\xe5\\xd0"\\x17@\'']),
            ([(4.135, )], [9, 1, "'\\n\\xd7\\xa3p=\\x8a\\x10@'"]),
            ([(2.5664, )], [9, 1, "'\\x9c\\xa2#\\xb9\\xfc\\x87\\x04@'"]),
            ([(1098210600.0, )],
             [10, 1, "'\\x00 \\x0f\\xc0\\x88\\xc4\\xe0\\x00'",
             '63266486400000000L', "u'20-10-2004 12:00:00.000000 am'"]),
            ([(1098210600.0, )],
             [10, 1, "'\\x00 \\x0f\\xc0\\x88\\xc4\\xe0\\x00'",
             '63266486400000000L', "u'20-10-2004 12:00:00.000000 am'"]),
            ([(1098210600.0, )],
             [10, 1, "'\\x00 \\x0f\\xc0\\x88\\xc4\\xe0\\x00'",
             '63266486400000000L', "u'20-10-2004 12:00:00.000000 am'"]),
            ([(0.0, )], [10, 1, "'\\x00\\x86[\\xab\\xb7\\xdd\\xdc\\x00'",
                   '62168275800000000L', "u'01-01-1970 5:30:00.000000 am'"]),
            ([(12345678.0, )],
             [10, 1, "'\\x80\\xf5\\x1b\\x1f\\xf2\\xe8\\xdc\\x00'",
                   '62180621478000000L', "u'24-05-1970 2:51:18.000000 am'"]),
            ([(u'foo', )], [12, 3, "'f\\x00o\\x00o\\x00'"]),
            ([(u'bar', )], [12, 3, "'b\\x00a\\x00r\\x00'"]),
            ([(u'baz', )], [15, 3]))

        for k in range(len(data_types_testdata)):
            self.db.execute(u'create table t%d (x %s)' %
                             (k, data_types_testdata[k][0]))
            self.db.execute(u'insert into t%d values (%s)' %
                             (k, data_types_testdata[k][1]))
            query_output = self.query(u'select * from t%d' % k)
            self.assertEqual(query_output, query_expected_output[k])


def test_main():
    test.test_support.run_unittest(E32DbTest)


if __name__ == "__main__":
    test_main()
