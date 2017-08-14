# Copyright (c) 2005-2009 Nokia Corporation
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

import e32
import sysinfo
import unittest
from test.test_support import TestSkipped, run_unittest

# Set this to `True` for creating the gold file for this test
create_goldfile = False

sysinfo_goldfile = "c:\\data\\python\\test\\sysinfo_goldfile.txt"
sysinfo_test_data = {}


class SysinfoTest(unittest.TestCase):

    def _test_util(self, test_key, test_data): 
        global sysinfo_test_data
        if create_goldfile:
            sysinfo_test_data[test_key] = test_data
        else:
            self.assertEqual(sysinfo_test_data[test_key], test_data)

    def test_os_version(self):
        self._test_util("os_version", sysinfo.os_version())

    def test_sw_version(self):
        self._test_util("sw_version", sysinfo.sw_version())

    def test_imei(self):
        self._test_util("imei", sysinfo.imei())

    def test_battery(self):
        sysinfo.battery()

    # The APIs sysinfo.signal() & sysinfo.signal_bars() have the same
    # functionality, ie both just call _sysinfo.signal_bars().
    # Testing signal_bars, as this is the documented API.
    def test_signal_bars(self):
        sysinfo.signal_bars()

    def test_singal_dbm(self):
        sysinfo.signal_dbm()

    def test_total_ram(self):
        self._test_util("total_ram", sysinfo.total_ram())

    def test_total_rom(self):
        self._test_util("total_rom", sysinfo.total_rom())

    # sysinfo.max_ramdrive_size can raise KErrNotSupported error. So, check for
    # this result. If this behaviour changes, then the test will fail.
    def test_max_ramdrive_size(self):
        try:
            self._test_util("max_ramdrive_size", sysinfo.max_ramdrive_size())
        except Exception, err:
            self.failUnlessEqual('[Errno -5] KErrNotSupported', str(err))

    def test_display_twips(self):
        self._test_util("display_twips", sysinfo.display_twips())

    def test_display_pixels(self):
        self._test_util("display_pixels", sysinfo.display_pixels())

    def test_free_ram(self):
        sysinfo.free_ram()

    def test_free_drivespace(self):
        sysinfo.free_drivespace()

    def test_ring_type(self):
        self._test_util("ring_type", sysinfo.ring_type())

    def test_active_profile(self):
        self._test_util("active_profile", sysinfo.active_profile())

def test_main():
    global sysinfo_test_data
    if not create_goldfile:
        try:
            sysinfo_test_data = eval(open(sysinfo_goldfile, 'rt').read())\
                                                         [e32.s60_version_info]
        except Exception, err:
            print err
            raise TestSkipped('Error reading goldfile')
    run_unittest(SysinfoTest)
    if create_goldfile:
        open(sysinfo_goldfile, 'wt').write(repr(sysinfo_test_data))

if __name__ == "__main__":
    test_main()
