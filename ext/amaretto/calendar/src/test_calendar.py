# Copyright (c) 2006-2009 Nokia Corporation
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

import time
import e32
import unittest
import e32calendar
import test.test_support


class CalendarTest(unittest.TestCase):
    week = 7 * 24 * 60 * 60
    day = 24 * 60 * 60
    hour = 60 * 60
    minute = 60
    now = 1243664756  # Sat, 30 May 2009 06:25:56 GMT

    def setUp(self):
        self.db = e32calendar.open('c:cal_test_db.cdb', 'n')

    def tearDown(self):
        self.db = e32calendar.open('c:cal_test_db.cdb', 'c')

        for id in self.db:
            del self.db[id]
        del self.db

    def create_calendar_entry(self, entry_type, entry_content):
        if entry_type == 'appointment':
            new_entry = self.db.add_appointment()
        elif entry_type == 'anniversary':
            new_entry = self.db.add_anniversary()
        elif entry_type == 'event':
            new_entry = self.db.add_event()
        elif entry_type == 'todo':
            new_entry = self.db.add_todo()
        elif entry_type == 'reminder':
            new_entry = self.db.add_reminder()

        new_entry.set_time(self.now + self.week, self.now + self.week +
                           self.hour)
        new_entry.alarm = self.now + self.week - 5 * self.minute
        new_entry.content = entry_content
        new_entry.location = 'right here right now'
        new_entry.replication = 'private'
        new_entry.priority = 1
        new_entry.commit()

        return new_entry

    def test_getitem_delitem(self):
        # Retrieve and delete the calendar entry using the __getitem__ and
        # __delitem__ members of CalendarDb class
        entry_content = 'test_getitem'
        new_entry = self.create_calendar_entry('appointment', entry_content)
        id = iter(self.db).next()
        stored_entry = self.db.__getitem__(id)
        self.assertEqual(stored_entry.content, entry_content)
        self.db.__delitem__(id)
        self.assertRaises(RuntimeError, lambda: self.db.__getitem__(id))

    def test_repeat_types(self):
        # Create different types of entries with repeat rules and then verify
        # that the rules are applied properly
        new_entry_1 = self.create_calendar_entry('appointment', 'daily')
        new_entry_2 = self.create_calendar_entry('anniversary',
                                                 'monthly_by_dates')
        new_entry_3 = self.create_calendar_entry('event', 'monthly_by_days')
        new_entry_4 = self.create_calendar_entry('todo', 'yearly_by_date')
        new_entry_5 = self.create_calendar_entry('reminder', 'yearly_by_day')
        new_entry_6 = self.create_calendar_entry('appointment', 'weekly')
        new_entry_7 = self.create_calendar_entry('anniversary', 'weekly2')

        repeat_1 = {'type': 'daily',
                  'start': new_entry_1.start_time,
                  'end': new_entry_1.start_time + self.week - self.day,
                  'interval': 2,  # on every second day.
                  'exceptions': []}

        # set monthly repeat (by dates) for 90 days.
        repeat_2 = {'type': 'monthly_by_dates',
                    # Sat, 06 Jun 2009 06:25:56 GMT
                    'start': new_entry_2.start_time,
                    # Thu, 03 Sep 2009 06:25:56 GMT
                    'end': new_entry_2.start_time + 90 * self.day - self.day,
                    # set the repeat occur 10th and 20th day of the month.
                    'days': [9, 19],
                    'exceptions': []}
                          # Wed, 10 Jun 2009 06:25:56 GMT
        repeat_2_value = {'start': 1244615156.0,
                          # Thu, 20 Aug 2009 06:25:56 GMT
                          'end': 1250749556.0,
                          'interval': 1,
                          'days': [9, 19],
                          'exceptions': [],
                          'type': 'monthly_by_dates'}

        # set monthly repeat (by days) for 90 days.
        repeat_3 = {'type': 'monthly_by_days',
                    # Sat, 06 Jun 2009 06:25:56 GMT
                    'start': new_entry_3.start_time,
                    # Thu, 03 Sep 2009 06:25:56 GMT
                    'end': new_entry_3.start_time + 90 * self.day - self.day,
                     # second tuesday and last friday of the month.
                    'days': [{'week': 1, 'day': 1}, {'week': 4, 'day': 4}],
                    'exceptions': []}
                          # Tue, 09 Jun 2009 06:25:56 GMT
        repeat_3_value = {'start': 1244528756.0,
                          # Fri, 28 Aug 2009 06:25:56
                          'end': 1251440756.0,
                          'interval': 1,
                          'days': [{'week': 4, 'day': 4},
                                   {'week': 1, 'day': 1}],
                          'exceptions': [],
                          'type': 'monthly_by_days'}

        # set yearly repeat (by date) for 3 years.
        repeat_4 = {'type': 'yearly_by_date',
                    # Sat, 06 Jun 2009 07:25:56 GMT
                    'start': new_entry_4.start_time,
                     # Mon, 04 Jun 2012 07:25:56 GMT
                    'end': new_entry_4.start_time + 3 * 365 * self.day - \
                                                                      self.day,
                    'exceptions': []}
                          # Sat, 06 Jun 2009 07:25:56 GMT
        repeat_4_value = {'start': 1244273156.0,
                          # Mon, 06 Jun 2011 07:25:56 GMT
                          'end': 1307345156.0,
                          'type': 'yearly_by_date',
                          'interval': 1,
                          'exceptions': []}

        # set yearly repeat (by day) on third thursday of june
        # during time interval new_entry.start_time -->
        # new_entry.start_time + 3 * 365 * day - day.
        repeat_5 = {'type': 'yearly_by_day',
                    # Sat, 06 Jun 2009 06:25:56 GMT
                    'start': new_entry_5.start_time,
                     # Mon, 04 Jun 2012 06:25:56 GMT
                    'end': new_entry_5.start_time + 3 * 365 * self.day - \
                                                                      self.day,
                    'days': {'day': 3, 'week': 2, 'month': 5},
                    'exceptions': []}
                          # Thu, 18 Jun 2009 06:25:56 GMT
        repeat_5_value = {'start': 1245306356.0,
                          # Thu, 21 Jun 2012 06:25:56 GMT
                          'end': 1340259956.0,
                          'interval': 1,
                          'days': {'week': 2, 'day': 3, 'month': 5},
                          'exceptions': [],
                          'type': 'yearly_by_day'}

        # repeat on tuesdays and thursdays every second week except on the
        # exception dates ('exceptions').
        repeat_6 = {'type': 'weekly',
                    # Sat, 06 Jun 2009 06:25:56 GMT
                    'start': new_entry_6.start_time,
                    # Sat, 15 Aug 2009 06:25:56 GMT
                    'end': new_entry_6.start_time + 10 * self.week,
                    'days': [1, 3],  # repeat on tuesday and thursday.
                    # 'exceptions':
                    # [1244874356.0, Sat, 13 Jun 2009 06:25:56 GMT
                    #  1244960756.0, Sun, 14 Jun 2009 06:25:56 GMT
                    #  1245047156.0, Mon, 15 Jun 2009 06:25:56 GMT
                    #  1245133556.0, Tue, 16 Jun 2009 06:25:56 GMT
                    #  1245219956.0, Wed, 17 Jun 2009 06:25:56 GMT
                    #  1245306356.0, Thu, 18 Jun 2009 06:25:56 GMT
                    #  1245392756.0] Fri, 19 Jun 2009 06:25:56 GMT
                    # --------------------------------------------
                    # no repeats on these days.
                    'exceptions': list([new_entry_6.start_time + self.week +
                                        i * self.day for i in range(7)]),
                    # repeat every second week.
                    'interval': 2}
                          # Tue, 16 Jun 2009 06:25:56 GMT
        repeat_6_value = {'start': 1245133556.0,
                          # Thu, 13 Aug 2009 06:25:56 GMT
                          'end': 1250144756.0,
                          'interval': 2,
                          'days': [1, 3],
                          # Tue, 16 Jun 2009 06:25:56 GMT &
                          # Thu, 18 Jun 2009 06:25:56 GMT
                          'exceptions': [1245133556.0, 1245306356.0],
                          'type': 'weekly'}

        # make it repeat weekly for 4 weeks.
        repeat_7 = {'type': 'weekly',
                    # which days in a week (monday, tuesday)
                    'days': [0, 1],
                    # Sat, 06 Jun 2009 06:25:56 GMT
                    'start': new_entry_7.start_time,
                    # Fri, 03 Jul 2009 06:25:56 GMT
                    'end': new_entry_7.start_time + 4 * self.week - self.day,
                    'exceptions': []}
                          # Mon, 08 Jun 2009 06:25:56 GMT
        repeat_7_value = {'start': 1244442356.0,
                          # Tue, 30 Jun 2009 06:25:56 GMT
                          'end': 1246343156.0,
                          'interval': 1,
                          'days': [0, 1],
                          'exceptions': [],
                          'type': 'weekly'}
        new_entry_1.begin()
        new_entry_1.set_repeat(repeat_1)
        new_entry_1.commit()

        new_entry_2.begin()
        new_entry_2.set_repeat(repeat_2)
        new_entry_2.commit()

        new_entry_3.begin()
        new_entry_3.set_repeat(repeat_3)
        new_entry_3.commit()

        new_entry_4.begin()
        new_entry_4.set_repeat(repeat_4)
        new_entry_4.commit()

        new_entry_5.begin()
        new_entry_5.set_repeat(repeat_5)
        new_entry_5.commit()

        new_entry_6.begin()
        new_entry_6.set_repeat(repeat_6)
        new_entry_6.commit()

        new_entry_7.begin()
        new_entry_7.set_repeat(repeat_7)
        new_entry_7.commit()

        self.assertEqual(repeat_1, new_entry_1.get_repeat())
        self.assertEqual(repeat_2_value, new_entry_2.get_repeat())
        self.assertEqual(repeat_3_value, new_entry_3.get_repeat())
        self.assertEqual(repeat_4_value, new_entry_4.get_repeat())
        self.assertEqual(repeat_5_value, new_entry_5.get_repeat())
        self.assertEqual(repeat_6_value, new_entry_6.get_repeat())
        self.assertEqual(repeat_7_value, new_entry_7.get_repeat())

    def test_export_import(self):
        # Exports an appointment entry and then imports the same and checks
        # the Entry ID
        db_id = []
        new_entry = self.create_calendar_entry('appointment', 'import/export')
        db_id.append(iter(self.db).next())
        vcals = self.db.export_vcalendars(tuple(db_id))
        imported_id = self.db.import_vcalendars(vcals)
        self.assertEqual(imported_id, db_id)

    def test_autocommit_rollback(self):
        # Autocommit and rollback feature tested by changing the entry
        # content
        new_entry = self.create_calendar_entry('appointment', 'autocommit')

        for id in self.db:
            entry = self.db[id]

        # autocommit is now on since the content has changed in the database.
        entry.content = 'TEXT I'
        self.assertEqual(self.db[entry.id].content, entry.content)

        # autocommit is now off..since the content has not changed in the
        # database.
        entry.begin()
        entry.content = 'TEXT II'
        self.assertNotEqual(self.db[entry.id].content, entry.content)

        # Now the changes are saved (and autocommit is set on again) since the
        # content has changed in the database.
        entry.commit()
        self.assertEqual(self.db[entry.id].content, entry.content)

        entry.begin()
        entry.content = 'TEXT III'
        # Revert the content back to 'TEXT II'
        entry.rollback()
        self.assertEqual(self.db[entry.id].content, 'TEXT II')

    def test_instances(self):
        # Create daily and monthly instances and then search for the same
        # using find_instances
        start_time = time.mktime((2008, 5, 31, 8, 0, 0, 0, 0, 0))
        end_time = time.mktime((2008, 5, 31, 9, 0, 0, 0, 0, 0))

        new_entry = self.create_calendar_entry('appointment', 'daily instance')

        # Appointment Entry created and committed for may31
        new_entry.begin()
        new_entry.set_time(start_time, end_time)
        new_entry.alarm = start_time - 5 * self.minute
        new_entry.commit()

        may30 = time.mktime((2008, 5, 30, 0, 0, 0, 0, 0, 0))
        may31 = time.mktime((2008, 5, 31, 0, 0, 0, 0, 0, 0))
        june30 = time.mktime((2008, 6, 30, 0, 0, 0, 0, 0, 0))

        # Daily instance 2 & 3 should not match any entries
        daily_instances_1 = self.db.daily_instances(may31)
        daily_instances_2 = self.db.daily_instances(may30)
        daily_instances_3 = self.db.daily_instances(may31, events=1)

        entry_id = iter(self.db).next()

        self.assertEqual(len(daily_instances_1), 1)
        self.assertEqual(entry_id, daily_instances_1[0]['id'])
        self.assertNotEqual(len(daily_instances_2), 1)
        self.assertNotEqual(len(daily_instances_3), 1)

        # Monthly instance 2 & 3 should not match any entries
        monthly_instances_1 = self.db.monthly_instances(may31)
        monthly_instances_2 = self.db.monthly_instances(june30)
        monthly_instances_3 = self.db.monthly_instances(may31, events=1,
                                                        todos=1)

        self.assertEqual(len(monthly_instances_1), 1)
        self.assertEqual(entry_id, monthly_instances_1[0]['id'])
        self.assertNotEqual(len(daily_instances_2), 1)
        self.assertNotEqual(len(daily_instances_3), 1)

        # Searching for 'instance' event and 'month' should return '[]'
        instances_found_1 = self.db.find_instances(may30, june30, u'instance')
        instances_found_2 = self.db.find_instances(may30, june30, u'instance',
                                                   events=1)
        instances_found_3 = self.db.find_instances(may30, june30, u'month')

        self.assertEqual(len(instances_found_1), 1)
        self.assertEqual(entry_id, instances_found_1[0]['id'])
        self.assertNotEqual(len(instances_found_2), 1)
        self.assertNotEqual(len(instances_found_3), 1)


def test_main():
    if e32.in_emulator():
        raise test.test_support.TestSkipped("Cannot test on emulator in " +
                                            "textshell mode")
    test.test_support.run_unittest(CalendarTest)

if __name__ == "__main__":
    test_main()
