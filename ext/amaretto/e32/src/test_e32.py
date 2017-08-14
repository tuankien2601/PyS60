# Copyright (c) 2008-2009 Nokia Corporation
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

import os
import unittest
import thread
import time
import test.test_support
import e32

sleep_time = 2
file_to_copy = u'c:\\data\\python\\test\\e32_file.log'
file_copied_to = u'c:\\data\\python\\test\\e32_test_file.log'
version_file = u'c:\\data\\python\\test\\e32_goldfile.txt'
platform_file = u'c:\\data\\python\\test\\platform.txt'
text = u'e32module'
initial_val = 1
changed_val = 2
seconds = 10
lock = e32.Ao_lock()
version_info = {}
platform_emu = False
tuple_of_capas = ()

# The server script will write to this temporary file when it is executed.
server_tempfile = "c:\\data\\python\\test\\start_server_tempfile.txt"
server_test_code = '''
f = open(r"%s", "w")
f.write("start_server works!")
f.close()
''' % server_tempfile


def get_version_info():
    global version_info
    version_info = eval(open(version_file, 'rt').read())


def get_platform():
    global platform_emu
    fp = open(platform_file, 'rt')
    plat_str = fp.read()
    if plat_str == "emu":
        platform_emu = True
    fp.close()


class E32ModTest(unittest.TestCase):

    def setUp(self):
        self.test_data = initial_val

    def test_ao_yield(self):
        e32.ao_yield()

    def test_drive_list(self):
        drive_list = e32.drive_list()
        self.failUnless(u'C:' in drive_list)

    def test_file_copy(self):
        # Create two temp files to copy and read.
        fp = open(file_to_copy, 'w')
        fp.write(text)
        fp.close()
        e32.file_copy(file_copied_to, file_to_copy)
        fp = open(file_copied_to, 'r')
        content = fp.read()
        self.failUnlessEqual(content, text)
        fp.close()
        os.remove(file_copied_to)
        os.remove(file_to_copy)

    def test_ao_sleep(self):
        before_sleep = time.time()
        e32.ao_sleep(sleep_time)
        after_sleep = time.time()
        diff_time = after_sleep - before_sleep
        self.failUnless(diff_time >= sleep_time)

    def modify_test_data(self, val):
        self.test_data = val

    def test_Ao_lock(self):
        """Test for Ao_callgate and Ao_lock"""

        def modify_data_and_signal_lock():
            # A helper function that is called asynchronously
            # via the active object mechanism.
            self.modify_test_data(changed_val)
            lock.signal()

        func = e32.ao_callgate(modify_data_and_signal_lock)
        func()
        lock.wait()
        self.failUnlessEqual(self.test_data, changed_val)

    def call_modify_test_data(self):
        self.modify_test_data(changed_val)

    def test_Ao_timer(self):
        timer = e32.Ao_timer()
        timer.after(sleep_time, self.call_modify_test_data)
        # sleep for an extra second to avoid a race condition
        e32.ao_sleep(sleep_time + 1)
        self.failUnlessEqual(self.test_data, changed_val)

    def test_timer_cancel(self):
        timer = e32.Ao_timer()
        timer.after(sleep_time, self.call_modify_test_data)
        timer.cancel()
        e32.ao_sleep(sleep_time + 1)
        self.failIfEqual(self.test_data, changed_val)

    def test_in_emulator(self):
        get_platform()
        self.failUnlessEqual(e32.in_emulator(), platform_emu)

    def test_set_home_time(self):
        time_before = time.time()
        time_after = time.time() + seconds
        try:
            e32.set_home_time(time_after)
        except Exception, err:
            # It needs higher capability than selfsigned.
            self.failUnlessEqual("[Errno -46] KErrPermissionDenied", str(err))
        else:
            diff_time = time.time() - time_before
            # The reason we do this is because the time is never increased
            # to exact 10 seconds and the difference does not yield 10
            self.failUnless(diff_time > 9)
            # This is done to restore the previous time
            prev_time = time.time() - seconds
            e32.set_home_time(prev_time)

    def test_pys60_version(self):
        get_version_info()
        version = e32.pys60_version
        self.failUnlessEqual(version_info['pys60_version'], version)

    def test_s60_version_info(self):
        self.failUnlessEqual(e32.s60_version_info, version_info['s60_version_info'])

    def test_is_ui_thread(self):

        def modify_if_in_ui_thread():
            if e32.is_ui_thread():
                self.modify_test_data(changed_val)
            lock.signal()

        self.failUnless(e32.is_ui_thread())
        thread.start_new_thread(modify_if_in_ui_thread, ())
        lock.wait()
        self.failIfEqual(self.test_data, changed_val)

    def test_reset_inactivity(self):
        """Check for inactivity and reset_inactivity"""
        e32.ao_sleep(sleep_time)
        sec_inactive = e32.inactivity()
        e32.reset_inactivity()
        sec_after_active = e32.inactivity()
        self.failUnless(sec_inactive > sec_after_active)

    def test_start_server(self):
        if e32.in_emulator():
            print "Cannot test e32.start_server on emulator"
            return
        server_script = u"c:\\data\\python\\test\\e32_start_server.py"

        # Create a test script which writes to a file
        f = open(server_script, "w")
        f.write(server_test_code)
        f.close()
        # Start the server and wait for a maximum of 20 seconds for the server
        # to write to the tempfile
        e32.start_server(server_script)
        for i in range(10):
            e32.ao_sleep(2)
            if os.path.exists(server_tempfile):
                break
        tempfile_text = open(server_tempfile, "r").read()
        self.failUnless(tempfile_text == 'start_server works!')
        os.remove(server_script)
        os.remove(server_tempfile)

    def test_start_exe(self):
        start_exe_tempfile = "c:\\data\\python\\test\\start_exe_tempfile.txt"
        # Start the exe with the command line argument 'start_exe_works' and
        # later verify if the exe writes it to a predefined temporary file
        e32.start_exe("start_exe_testapp.exe", "start_exe_works!", 1)
        tempfile_text = open(start_exe_tempfile, "r").read()
        self.failUnless(tempfile_text == 'start_exe_works!')
        os.remove(start_exe_tempfile)

    def test_get_capabilities(self):
        global tuple_of_capas
        tuple_of_capas = e32.get_capabilities()
        self.failIfEqual(len(tuple_of_capas), 0)

    def test_has_capabilities(self):
        if len(tuple_of_capas):
            count = 0
            for exec_statement in ["e32.has_capabilities([''])",
                "e32.has_capabilities(['Some_capability'])"]:
                try:
                    exec(exec_statement)
                except ValueError:
                    count = count + 1
            if count != 2:
                self.fail("Exceptions not raised properly for invalid cases")
        both_present = e32.has_capabilities([tuple_of_capas[0],
                                       tuple_of_capas[1]])
        all_present = e32.has_capabilities(list(tuple_of_capas))
        one_present = e32.has_capabilities([tuple_of_capas[0],
                                       'TCB'])
        all_absent = e32.has_capabilities(['TCB', 'TrustedUI', 'DRM'])
        result = (both_present and all_present) and not \
                 (one_present or  all_absent) and e32.has_capabilities([])
        self.failUnlessEqual(result, True, "Values returned are erroneous")


def test_main():
    test.test_support.run_unittest(E32ModTest)

if __name__ == "__main__":
    test_main()
