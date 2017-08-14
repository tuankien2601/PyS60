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

import unittest
import btsocket
import test.test_support
import e32


class BTSocketTest(unittest.TestCase):

    def test_bt_server(self):
        server_socket = btsocket.socket(btsocket.AF_BT, btsocket.SOCK_STREAM)
        port = btsocket.bt_rfcomm_get_available_server_channel(server_socket)
        server_socket.bind(("", port))
        server_socket.listen(1)
        btsocket.bt_advertise_service(u"port1", server_socket, True,
                                      btsocket.RFCOMM)
        btsocket.bt_advertise_service(u"port1", server_socket, False,
                                      btsocket.RFCOMM)
        btsocket.bt_advertise_service(u"port2", server_socket, True,
                                      btsocket.OBEX)
        btsocket.bt_advertise_service(u"port2", server_socket, False,
                                      btsocket.OBEX)
        btsocket.set_security(server_socket, (btsocket.AUTH |
                              btsocket.ENCRYPT | btsocket.AUTHOR))
        server_socket.close()

    def test_access_point(self):
        access_points = btsocket.access_points()
        if access_points:
            apo = btsocket.access_point(access_points[0]['iapid'])
            btsocket.set_default_access_point(apo)
            btsocket.set_default_access_point(None)


def test_main():
    if e32.in_emulator():
        raise test.test_support.TestSkipped("Cannot test on emulator")
    test.test_support.run_unittest(BTSocketTest)

if __name__ == "__main__":
    test_main()
