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

from test.test_support import TestSkipped, run_unittest
import unittest
import e32
from sensor import *


class SensorModuleTest(unittest.TestCase):

    def test_get_property(self):
        sensor_channel = AccelerometerXYZAxisData()
        print sensor_channel.get_property()


def test_main():
    # To ensure the test is run only on devices which support sensor
    # framework the following block of code is added.
    try:
        import sensorfw
    except:
        raise TestSkipped('Sensor Framework not supported on this device.')
    run_unittest(SensorModuleTest)

if __name__ == "__main__":
    test_main()
