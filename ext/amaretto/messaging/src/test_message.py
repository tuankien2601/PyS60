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

import e32
import messaging
import inbox
import thread
from test.test_support import TestSkipped, run_unittest
import unittest

contacts_store = "c:\\data\\python\\test\\contacts_store.txt"
contacts_info = {}
s60_version = e32.s60_version_info


class TestMessaging(unittest.TestCase):
    message = u'Hello! from PyS60'
    message_id = 0
    sender_name = 'Msg_Testing'

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        self.inbox_obj = inbox.Inbox(inbox.EInbox)
        self.sent_obj = inbox.Inbox(inbox.ESent)
        self.inbox_obj.bind(self.inbox_callback)

    def inbox_callback(self, ib_id):
        self.message_id = ib_id

    def test_send_sms(self):
        # Test sending of a sms
        messaging.sms_send(contacts_info['phone_number'][s60_version],
                           self.message, '7bit', None, self.sender_name)
        e32.ao_sleep(5)
        ids = self.inbox_obj.sms_messages()
        self.failUnlessEqual(self.inbox_obj.content(ids[0]), self.message)

    def test_sms_in_thread(self):
        # Test sending of a sms in thread.

        def go_thread():
            print "Entered thread"

            def message_callback(state):
                if state == messaging.ESent:
                    print "**Message was sent**"
                if state == messaging.ESendFailed:
                    self.fail("Message sending failed")

            messaging.sms_send(contacts_info['phone_number'][s60_version],
                               self.message, '7bit', message_callback,
                               self.sender_name)
            print "Message send call done"
            e32.ao_sleep(10)
            print "Exiting thread"

        print "Thread started"
        thread.start_new_thread(go_thread, ())
        e32.ao_sleep(15)
        print "Exiting function"

    def tearDown(self):
        ib_msg_ids = self.inbox_obj.sms_messages()
        sent_msg_ids = self.sent_obj.sms_messages()
        if len(ib_msg_ids) and self.message_id == ib_msg_ids[0]:
            self.inbox_obj.delete(ib_msg_ids[0])
            self.sent_obj.delete(sent_msg_ids[0])


def test_main():
    global contacts_info
    if not e32.in_emulator():
        try:
            contacts_info = eval(open(contacts_store, 'rt').read())
        except:
            raise TestSkipped('Contact information not found')
        run_unittest(TestMessaging)
    else:
        raise TestSkipped('Cannot test on the emulator')


if __name__ == "__main__":
    test_main()
