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

import socket
import appuifw
import e32


def quit():
    app_lock.signal()


def retreive_ap():
    access_points = socket.access_points()
    for ap in access_points:
        if not 'iapid' in ap and not 'name' in ap:
            appuifw.note(u"Incorrect return value. Test failed")
    print access_points
    option = appuifw.query(u"Run negative test as well?", 'query')
    if option is not None:
        try:
            socket.access_points("Invalid argument")
        except:
            appuifw.note(u"Exception raised for invalid argument. Test passed")
            raise


def set_ap():
    val = appuifw.query(u"Access point name:", 'text')
    if val is not None:
        try:
            socket.set_default_access_point(unicode(val))
        except:
            appuifw.note(u"If you passed an invalid access point then the" +
                          " Test Passed else it Failed.")
            raise
        else:
            appuifw.note(u"Default access point set. Test passed. " +
                         "Run ConnectToTheInternet test to verify.")


def clear_ap():
    socket.set_default_access_point(None)
    appuifw.note(u"Default access point cleared. Test passed. " +
                         "Run ConnectToTheInternet test to verify.")


def connect_net():
    print '+++++++++++++++++++++'
    print socket.getaddrinfo("www.google.com", 'http')
    print '+++++++++++++++++++++'


appuifw.app.title = u"test_socket_ap"
appuifw.app.screen = "normal"
appuifw.app.menu = [(u"RetrieveAccessPoints", retreive_ap),
                    (u"SetDefaultAccessPoint", set_ap),
                    (u"ClearDefaultAccessPoint", clear_ap),
                    (u'ConnectToTheInternet', connect_net)]

appuifw.app.exit_key_handler = quit

app_lock = e32.Ao_lock()
app_lock.wait()
