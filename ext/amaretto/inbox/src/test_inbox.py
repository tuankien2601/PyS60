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
from test.test_support import TestSkipped, run_unittest
import unittest
import inbox
import messaging
import time

contacts_store = "c:\\data\\python\\test\\contacts_store.txt"
contacts_info = {}
s60_version = e32.s60_version_info


class InboxModuleTest(unittest.TestCase):
    message = u"Hi from PyS60"
    message_id = 0
    sender = 'Msg_Testing'
    current_time = time.time()
    temp_id = 0

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        self.type_inbox = inbox.Inbox(inbox.EInbox)
        self.type_inbox.bind(self.inbox_callback)
        self.type_sent = inbox.Inbox(inbox.ESent)
        self.type_sent.bind(self.sent_callback)
        self.type_outbox = inbox.Inbox(inbox.EOutbox)
        self.type_outbox.bind(self.outbox_callback)

    def outbox_callback(self, ob_id):
        # Store the message id which will be used to verify whether Outbox
        # callback is hit
        self.temp_id = ob_id

    def setUp(self):
        messaging.sms_send(contacts_info['phone_number'][s60_version],
                           self.message, '7bit', None, self.sender)
        e32.ao_sleep(10)

    def test_outbox_callback(self):
        self.assert_(self.temp_id, "Outbox callback not hit")

    def sent_callback(self, sent_msg_id):
        self.failUnlessEqual(self.message_id, sent_msg_id)

    def inbox_callback(self, inbox_msg_id):
        self.message_id = inbox_msg_id

    def test_inbox_folder(self):
        # After sending the message to the same number check the inbox for
        # receipt of the message and validate contents.
        msg_ids = self.type_inbox.sms_messages()
        self.failUnlessEqual(self.type_inbox.content(msg_ids[0]),
                             self.message,
                             "The message contents are not proper")
        # Subtract ten seconds buffer time from the time of receipt of
        # message in inbox to validate.
        if self.type_inbox.time(msg_ids[0]) < self.current_time:
            self.fail("Time of received message in inbox not appropriate")
        self.failUnlessEqual(self.type_inbox.unread(msg_ids[0]), 1,
                             "The message should have been marked unread")
        self.failUnlessEqual(contacts_info['phone_number'][s60_version],
                             self.type_inbox.address(msg_ids[0]),
                             "The message should have been unread")
        # Mark the message as read
        self.type_inbox.set_unread(msg_ids[0], 0)
        self.failIfEqual(self.type_inbox.unread(msg_ids[0]), 1,
                         "The message should have been marked read")
        # Mark the message as unread again
        self.type_inbox.set_unread(msg_ids[0], 1)
        self.failUnlessEqual(self.type_inbox.unread(msg_ids[0]), 1,
                         "The message should have been marked unread")

    def test_sent_folder(self):
        # Test the Sent Items folder
        msg_ids = self.type_sent.sms_messages()
        self.failUnlessEqual(self.type_sent.content(msg_ids[0]),
                             self.message,
                             "The message content is not proper")

    def test_message_delete(self):
        # Check if the messages are deleted successfully.
        self.type_inbox.delete(self.message_id)
        ib_msg_ids = self.type_inbox.sms_messages()
        if len(ib_msg_ids) and self.message_id == ib_msg_ids[0]:
            self.fail("Something wrong with the delete API")
        sent_msg_ids = self.type_sent.sms_messages()
        self.type_sent.delete(sent_msg_ids[0])

    def tearDown(self):
        ib_msg_ids = self.type_inbox.sms_messages()
        sent_msg_ids = self.type_sent.sms_messages()
        if len(ib_msg_ids) and self.message_id == ib_msg_ids[0]:
            self.type_inbox.delete(ib_msg_ids[0])
            self.type_sent.delete(sent_msg_ids[0])


class OutboxFolderTest(unittest.TestCase):
    """ Tests operations on Outbox folder."""
    message = u"Testing for outbox"
    sender = "Msg Testing"
    message_id = 0

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        self.type_outbox = inbox.Inbox(inbox.EOutbox)

    def setUp(self):
        messaging.sms_send('123', self.message,
                           '7bit', None, self.sender)
        e32.ao_sleep(5)

    def test_outbox_folder(self):
        msg_ids = self.type_outbox.sms_messages()
        self.failUnlessEqual(self.type_outbox.content(msg_ids[0]),
                             self.message,
                             "Content of the message erroneous")

    def test_message_delete(self):
        # Check if the messages are deleted successfully.
        ob_msg_ids = self.type_outbox.sms_messages()
        self.message_id = ob_msg_ids[0]
        self.type_outbox.delete(ob_msg_ids[0])
        ob_msg_ids = self.type_outbox.sms_messages()
        if len(ob_msg_ids) and self.message_id == ob_msg_ids[0]:
            self.fail("Something wrong with the delete API")

    def tearDown(self):
        ob_msg_ids = self.type_outbox.sms_messages()
        if len(ob_msg_ids) and not self.message_id:
            self.type_outbox.delete(ob_msg_ids[0])


def test_main():
    global contacts_info
    if not e32.in_emulator():
        try:
            contacts_info = eval(open(contacts_store, 'rt').read())
        except:
            raise TestSkipped('Contact information not found')
        # Not running OutboxFolderTest as it pops a error dialog on message
        # not being sent and thus requires a key press for its exit.
        run_unittest(InboxModuleTest)
    else:
        raise TestSkipped('Cannot test on emulator')


if __name__ == "__main__":
    test_main()
