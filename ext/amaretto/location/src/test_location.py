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

import location
import unittest
import test.test_support
import e32


class LocationTest(unittest.TestCase):

    def test_gsm_location(self):
        # gms_location returns : [Mobile Country Code, Mobile Network Code,
        # Location Area Code, CellID]
        mcc, mnc, lac, cell_id = location.gsm_location()
        # Since cell_id changes very frequently it is omitted
        self.assertEqual([mcc, mnc, lac], [404, 45, 1627])


def test_main():
    if e32.in_emulator():
        raise test.test_support.TestSkipped("Cannot test on emulator")
    test.test_support.run_unittest(LocationTest)

if __name__ == "__main__":
    test_main()
