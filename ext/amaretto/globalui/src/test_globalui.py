# Copyright (c) 2008 Nokia Corporation
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
import appuifw
import globalui

def test_global_notes():
    globalui.global_note(u'plain global note')
    for note_type in ('error', 'info', 'warn', 'confirm', 'charging',
                      'text', 'wait', 'not_charging', 'battery_full',
                      'battery_low', 'recharge_battery'):
        globalui.global_note(u'Note type: ' + note_type, note_type)
        time.sleep(1)

def test_permanent_global_note():
    globalui.global_note(u'Permanent note', 'perm')

def test_global_queries():
    result = globalui.global_query(u"global_query",3);
    print "result from global_query:", result
    result = globalui.global_msg_query(u"global_msg_query",u"MyHeader",3);
    print "result from global_msg_query:", result
    result = globalui.global_popup_menu([u"item1", u"item2"],u"MyMenu",3)
    print "result from global_popup_menu:", result


quitlock = e32.Ao_lock()
appuifw.app.menu=[(u'All global notes', test_global_notes),
                  (u'Global queries', test_global_queries),
                  (u'Test permanent global note', test_permanent_global_note),
                  (u'Exit', quitlock.signal)]

print "Select a test case from the Options menu."
quitlock.wait()
