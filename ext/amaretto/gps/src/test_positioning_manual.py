# Copyright (c) 2007 - 2009 Nokia Corporation
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

import appuifw
import e32
import positioning


def cb(event):
    print "---"
    print event
    print "---"


def clean():
    positioning.stop_position()
    print "Done."
    lock.signal()


positioning.set_requestors([{"type": "service",
                             "format": "application",
                             "data": "test_app"}])
print "Starting position feed."
print positioning.position(course=1, satellites=1, callback=cb,
                           interval=500000, partial=1)

lock = e32.Ao_lock()
appuifw.app.exit_key_handler = clean
lock.wait()
print "Testing last_position API..."
print "Values displayed only if position data is cached"
print positioning.last_position()
print "Done."
