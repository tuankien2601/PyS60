# Copyright (c) 2009 Nokia Corporation
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

import telephone
import e32
import appuifw


phone_number = u'+919008025211'
# During the lifetime of a call, state transitions occur amongst the ones
# listed here
state_info = ['Status Unknown',
              'Idle',
              'Dialling',
              'Ringing',
              'Answering',
              'Connecting',
              'Connected',
              'Reconnect Pending',
              'Diconnecting',
              'On Hold',
              'Transferring call',
              'Transfer Alerting']


def validate(call_info):
    # Phone number is displayed only if it is an incoming call
    print "Phone number: ", call_info[1]
    print "Call status: ", state_info[call_info[0]]


def test_outgoing_call():
    sleep_during_call = 8
    sleep_after_call = 4

    print "Making a phone call.."
    telephone.dial(phone_number)
    e32.ao_sleep(sleep_during_call)
    print "Hanging up the call.."
    telephone.hang_up()
    e32.ao_sleep(sleep_after_call)
    print "Test completed successfully."


def test_incoming_call():
    print "Test for incoming calls started. Validate the call state " +\
          "getting printed to the actual call state the phone is in."
    telephone.incoming_call()
    telephone.answer()


def exit_handler():
    appuifw.app.exit_key_handler = old_exit_handler
    lock.signal()


if __name__ == "__main__":
    lock = e32.Ao_lock()

    # Bind the callback
    telephone.call_state(validate)

    appuifw.app.menu=[
                     (u'Test Incoming Call', test_incoming_call),
                     (u'Test Outgoing Call', test_outgoing_call),
                     (u'Exit', lock.signal)]
    old_exit_handler = appuifw.app.exit_key_handler
    appuifw.app.exit_key_handler = exit_handler
    lock.wait()
