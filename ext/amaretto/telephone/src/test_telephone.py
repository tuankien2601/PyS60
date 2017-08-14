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
import telephone
import unittest
from test.test_support import TestSkipped, run_unittest


class TelephoneTest(unittest.TestCase):

    def test_call(self):
        PHONENUMBER = u'+9125211'
        self.state = 0
        telephone.call_state(self.validate)
        telephone.dial(PHONENUMBER)
        e32.ao_sleep(4)
        telephone.hang_up()
        e32.ao_sleep(4)

    def validate(self, value):
        self.state += 1
        if len(value):
            if self.state == 1:
                self.failUnlessEqual(value[0], telephone.EStatusDialling,
                                     "State not proper")
            elif self.state == 2:
                self.failUnlessEqual(value[0],
                                     telephone.EStatusDisconnecting,
                                     "State not proper")
            else:
                self.failUnlessEqual(value[0], telephone.EStatusIdle,
                                     "State not proper")


def test_main():
    if not e32.in_emulator():
        run_unittest(TelephoneTest)
    else:
        raise TestSkipped('Cannot test on the emulator')


if __name__ == "__main__":
    test_main()
