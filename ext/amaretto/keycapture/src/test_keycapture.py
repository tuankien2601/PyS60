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

import keycapture
import unittest
import test.test_support
import key_codes
import e32


class KeycaptureTest(unittest.TestCase):

    def test_key_capture(self):

        def key_callback(self, key):
            print "Callback hit! Key pressed :", key
            self.fail("Someone/Something pressed a key! :-O ")

        capturer = keycapture.KeyCapturer(key_callback)
        capturer.forwarding = 0
        capturer.keys = (key_codes.EKeyUpArrow, key_codes.EKeyDownArrow)
        capturer.start()
        e32.ao_sleep(1)
        capturer.stop()
        self.assertEqual(0, capturer.last_key())

        capturer.keys = keycapture.all_keys
        capturer.forwarding = 1
        capturer.start()
        e32.ao_sleep(1)
        self.assertEqual(0, capturer.last_key())
        e32.ao_sleep(1)
        capturer.stop()
        self.assertEqual(0, capturer.last_key())


def test_main():
    if e32.in_emulator():
        raise test.test_support.TestSkipped("Cannot test on emulator in " +
                                            "textshell mode")    
    test.test_support.run_unittest(KeycaptureTest)


if __name__ == "__main__":
    test_main()
