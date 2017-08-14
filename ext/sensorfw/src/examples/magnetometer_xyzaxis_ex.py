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

from sensor import *
import e32


class DemoApp():

    def __init__(self):
        self.magnetometer = \
            MagnetometerXYZAxisData(data_filter=LowPassFilter())
        self.magnetometer.set_callback(data_callback=self.my_callback)
        self.counter = 0

    def my_callback(self):
        # For stream sensor data the callback is hit 35 times per sec(On 5800).
        # The device cannot handle resource hungry operations like print in the
        # callback function for such high frequencies. A workaround is to
        # sample the data as demonstrated below.
        if self.counter % 5 == 0:
            print  "Calib:", self.magnetometer.calib_level
            print "X:%s, Y:%s, Z:%s" % (self.magnetometer.x,
                                      self.magnetometer.y, self.magnetometer.z)
            print "Timestamp:", self.magnetometer.timestamp
        self.counter = self.counter + 1

    def run(self):
        self.magnetometer.start_listening()

if __name__ == '__main__':
    d = DemoApp()
    d.run()
    e32.ao_sleep(5)
    d.magnetometer.stop_listening()
    print "Exiting MagnetometerAxis"
