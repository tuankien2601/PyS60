#
# postioning_test.py
#
# Simple test script for positioning extension.
#
# Copyright (c) 2007 Nokia Corporation
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

import positioning

# SYMBIAN_UID = 0xE1000121

# information about available positioning modules
print "***available modules***"
print positioning.modules()
print ""

# id of the default positioning module
print "***default module***"
print positioning.default_module()
print ""

# detailed information about the default positioning module
print "***detailed module info***"
print positioning.module_info(positioning.default_module())
print ""

# set requestors.
# at least one requestor must be set before requesting the position.
# the last requestor must always be service requestor 
# (whether or not there are other requestors). 
positioning.set_requestors([{"type":"service",
                             "format":"application",
                             "data":"test_app"}])

# get the position. 
# note that the first position()-call may take a long time
# (because of gps technology).
print "***position info***"                         
print positioning.position()
print ""

# re-get the position.
# this call should be much quicker.
# ask also course and satellite information.
print "***course and satellites***" 
print positioning.position(course=1,satellites=1)
print ""

