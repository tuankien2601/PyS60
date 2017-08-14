# Copyright (c) 2006 - 2009 Nokia Corporation
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

import logs
import e32
import telephone
import inbox
import unittest
import messaging
import time
from test.test_support import TestSkipped, run_unittest

contacts_store = "c:\\data\\python\\test\\contacts_store.txt"
contacts_info = {}


class LogsTest(unittest.TestCase):
    PHONENUMBER1 = u"+12345"
    PHONENUMBER2 = u"+56789"
    SLEEP = 5

    def test_raw_data(self):
        result = logs.raw_log_data()
        self.assert_(isinstance(result, list) and isinstance(result[0], dict),
                     "The return type should be a list of maps")

    def test_log_data(self):
        global start_t
        global end_t
        start_t = time.mktime(time.gmtime())
        telephone.dial(self.PHONENUMBER1)
        e32.ao_sleep(self.SLEEP)
        telephone.hang_up()
        end_t = time.mktime(time.gmtime())
        result = logs.log_data(type='call', start_log=0, num_of_logs=1,
                               mode='out')
        self.failUnlessEqual(result[0]['number'], self.PHONENUMBER1,
                             'log_data API returns erroneous values')

    def test_log_data_by_time(self):
        result = logs.log_data_by_time('call', start_t, end_t,
                                       mode='out')
        self.assert_(len(result) and result[0]['number'] == self.PHONENUMBER1,
                     "logging data by time is faulty")

    def test_others(self):
        self.failIf(len(logs.faxes()),
                    "There shouldn't be any logs for type faxes")
        self.failIf(len(logs.emails()),
                    "There shouldn't be any logs for type emails")
        self.failIf(len(logs.scheduler_logs()),
                    "There shouldn't be any logs for type scheduler_logs")
        self.failIf(len(logs.data_logs()),
                    "There shouldnt be any data logs")

    def test_log_calls(self):
        telephone.dial(self.PHONENUMBER2)
        e32.ao_sleep(self.SLEEP)
        telephone.hang_up()
        result = logs.calls(mode='out')
        self.assert_((len(result) and
                      result[0]['direction'] == u'Outgoing' and
                      result[0]['description'] == u'Voice call' and
                      result[0]['number'] == self.PHONENUMBER2),
                      "log of outgoing calls not proper")

    def test_log_messages(self):
        s60_version = e32.s60_version_info
        messaging.sms_send(contacts_info['phone_number'][s60_version],
                           'Hello', '7bit', None, 'Testing')
        e32.ao_sleep(self.SLEEP)
        result = logs.sms(start_log=0, num_of_logs=1, mode='in')
        self.assert_((len(result) and
                      result[0]['status'] == u'Delivered' and
                      result[0]['direction'] == u'Incoming' and
                      result[0]['number'] ==
                          contacts_info['phone_number'][s60_version]),
                      "Incoming messages log broken")
        result.extend(logs.sms(start_log=0, num_of_logs=1, mode='out'))
        self.assert_((len(result) and result[1]['status'] == u'Sent' and
                      result[1]['direction'] == u'Outgoing' and
                      result[0]['number'] ==
                          contacts_info['phone_number'][s60_version]),
                      "Outgoing messages log broken")

        # Cleaning up Inbox and Sent items
        ib_msg_ids = inbox.Inbox().sms_messages()
        sent_msg_ids = inbox.Inbox(inbox.ESent).sms_messages()
        if len(ib_msg_ids):
            inbox.Inbox().delete(ib_msg_ids[0])
            inbox.Inbox(inbox.ESent).delete(sent_msg_ids[0])


def test_main():
    global contacts_info
    if not e32.in_emulator():
        try:
            contacts_info = eval(open(contacts_store, 'rt').read())
        except:
            raise TestSkipped('goldfile not present')
        run_unittest(LogsTest)
    else:
        raise TestSkipped('Cannot test on the emulator')


if __name__ == "__main__":
    test_main()
