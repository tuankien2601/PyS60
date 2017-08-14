#
# reitti.py
#
# A simple user interface to the SMS based "reitti" service
# for ordering information on public transportation between
# given addresses in Finnish capital region.
#
# Copyright (c) 2005 Nokia Corporation
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

import appuifw
from messaging import *

old_title = appuifw.app.set_title(u"Reitti interface")

dep = appuifw.multi_query(u"Departure address",u"h/e/v (Helsinki/Espoo/Vantaa)")
if dep != None:
    des = appuifw.multi_query(u"Destination address",u"h/e/v (Helsinki/Espoo/Vantaa)")
    if des != None:
        if appuifw.query(u"Send request as SMS to 16400","query") == True:
            sms_send("16400", u"reitti "+dep[0]+" "+dep[1]+", "+des[0]+" "+des[1])
            appuifw.note(u"Reitti request sent - reply comes per SMS", "info")
        
appuifw.app.set_title(old_title)
