#
# position_finder.py
#
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
#
# Description: This script file is used to find the position using
# "positioning" module under various circumstances which are as follows:
# a) for new set of requestors.
# b) for same set of requestors.
# c) with the help of specified module.

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

# This function is used to stop an ongoing position request
def stop_finding_position():
    positioning.stop_position()

# This function is used to find the postion for new set of requestors
def get_position_with_requestor():
    positioning.set_requestors([{"type":"service",
                                "format":"application",
                                "data":"test_app"}])
    print "Starting position feed again - with requestor."
    print positioning.position(course=1, satellites=1,
                               callback=cb, interval=1000000,
                               partial=1)

# This function is used to find the position for the same set of requestors
def get_position_without_requestor():
    print "Starting position feed again- without requestor."
    print positioning.position(course=1, satellites=1,
                               callback=cb, interval=1000000,
                               partial=1)

# This function is used to find the position with the help of specified module
def set_module():
    modules = positioning.modules()
    positioning.select_module(modules[2]["id"])
    print "Starting position feed again- withspecific module."
    print positioning.position(course=1, satellites=1,
                               callback=cb, interval=1000000,
                               partial=1)
# This function is used to get the last known position
def get_last_position():
    print "start getting last position"
    print positioning.last_position()

positioning.set_requestors([{"type":"service",
                            "format":"application",
                            "data":"test_app"}])

print "Starting position feed."

print positioning.position(course=1, satellites=1,
                           callback=cb, interval=1000000,
                           partial=1)

appuifw.app.menu = [(u"stop request",stop_finding_position),
    (u"position_requestors_set: test1",get_position_with_requestor),
    (u"position_without_requestor: test2",get_position_without_requestor),
    (u"position_select_module: test3",set_module),
    (u"last_position",get_last_position)]

lock=e32.Ao_lock()
appuifw.app.exit_key_handler=clean
lock.wait()
