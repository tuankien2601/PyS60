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

from btsocket import *
import appuifw
import e32


def bt_server(service_type=0):
    service_name = u"Test"

    socket_object = socket(AF_BT, SOCK_STREAM)
    port = bt_rfcomm_get_available_server_channel(socket_object)
    print "Binding service to port - ", port
    socket_object.bind(("", port))
    socket_object.listen(1)

    print "Advertising service as - ", service_name
    if service_type == 0:
        bt_advertise_service(service_name, socket_object, True, OBEX)
    else:
        bt_advertise_service(service_name, socket_object, True, RFCOMM)

    try:
        print "Setting security to ENCRYPT | AUTH | AUTHOR."
        set_security(socket_object, ENCRYPT | AUTH | AUTHOR)
        if service_type == 0:
            temp_file = u"c:\\data\\temp_server.txt"
            print "Waiting for connection from client..."
            bt_obex_receive(socket_object, temp_file)
            print "File received."
        else:
            (sock_addr, peer_addr) = socket_object.accept()
            print "Connection from : ", peer_addr
            temp_text = appuifw.query(u"Client message", "text")
            sock_addr.send(temp_text + '\n')
            print "Message sent"
    finally:
        if service_type == 0:
            bt_advertise_service(service_name, socket_object, False, OBEX)
        else:
            bt_advertise_service(service_name, socket_object, False, RFCOMM)
        socket_object.close()
    print "Finished."


def bt_client(service_type=0):
    if service_type == 0:
        server_addr, services = bt_obex_discover()
    else:
        server_addr, services = bt_discover()
    print "Discovered: %s, %s" % (server_addr, services)
    if len(services) > 0:
        choices = services.keys()
        choices.sort()
        choice = appuifw.popup_menu([unicode(services[x]) + ": " + x
                                   for x in choices], u'Choose port:')
        port = services[choices[choice]]
    else:
        port = services[services.keys()[0]]
    address = (server_addr, port)

    if service_type == 0:
        temp_file = u"c:\\data\\temp_client.txt"
        f = open(temp_file, 'w')
        f.write("hello")
        f.close()

        # send file via OBEX
        print "Sending file %s to host %s port %s" % (temp_file, address[0],
                                                      address[1])
        bt_obex_send_file(address[0], address[1], temp_file)
        print "File sent."
    else:
        temp_sock = socket(AF_BT, SOCK_STREAM)
        temp_sock.connect(address)
        print "Connected..."

        def receive_data():
            line = []
            while 1:
                ch = temp_sock.recv(1)
                if(ch == '\n'):
                    break
                line.append(ch)
            return line

        data = receive_data()
        temp_sock.close()
        print "Data received from the server: ", ''.join(data)


if __name__ == '__main__':
    appuifw.app.exit_key_handler = None
    ap_id = select_access_point()
    print "All access points :", access_points()
    print "Selected Access point ID:", ap_id
    e32.ao_sleep(3)
    mode = appuifw.popup_menu([u'1. Server', u'2. Client'], u'Select mode:')
    if mode is not None:
        service_type = appuifw.popup_menu([u'1. OBEX', u'2. RFCOMM'],
                                          u'Select service type:')
        if mode == 0:
            bt_server(service_type)
        elif mode == 1:
            bt_client(service_type)
