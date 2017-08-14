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
        self.magnetic_north = MagneticNorthData()
        self.magnetic_north.set_callback(data_callback=self.my_callback)

    def my_callback(self):
        azimuth = str(self.magnetic_north.azimuth)
        print "Azimuth:", azimuth
        print "Timestamp:", self.magnetic_north.timestamp

    def run(self):
        self.magnetic_north.start_listening()

if __name__ == '__main__':
    d = DemoApp()
    d.run()
    e32.ao_sleep(1)
    d.magnetic_north.stop_listening()
    print "Exiting MagneticNorth"
