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
import appuifw
import e32
import graphics


class WhileLoopApp:
    '''generic sensor application classes'''

    def __init__(self):
        appuifw.app.exit_key_handler = self.quit
        self.running = True

    def run(self):
        while self.running:
            e32.ao_sleep(1)

    def quit(self):
        self.running = False


class LockSignalApp:

    def __init__(self):
        appuifw.app.exit_key_handler = self.quit
        self.lock = e32.Ao_lock()

    def run(self):
        self.lock.wait()

    def quit(self):
        self.lock.signal()

App = LockSignalApp


class DemoApp(App):
    BG_COLOR = 0x000000 # black
    TEXT_COLOR = 0xffffff # white

    def __init__(self):
        App.__init__(self)
        self.canvas = appuifw.Canvas()
        appuifw.app.body = self.canvas
        self.draw = self.canvas
        myFilter = LowPassFilter()
        self.accelerometer = AccelerometerXYZAxisData(data_filter=myFilter)
        self.accelerometer.set_callback(data_callback=self.callback)

        self.rotation = RotationData()
        self.magenticnorth = MagneticNorthData()

        self.ALS = AmbientLightData()
        self.proxi = ProximityMonitor()
        self.orientation = OrientationData()

        self.doubletap = AccelerometerDoubleTappingData()
        self.doubletapCounter = 0
        self.doubletap.set_callback(data_callback=self.dtCb)
        self.doubletapAxis = self.doubletap.get_axis_active()

    def dtCb(self):
        self.doubletapCounter += 1

    def callback(self):

        self.draw.clear(self.BG_COLOR)
        self.draw.text((10, 10), u'accelerometer', fill=self.TEXT_COLOR)
        self.draw.text((120, 10), u'Rotation', fill=self.TEXT_COLOR)

        x, y, z = self.accelerometer.x, self.accelerometer.y, self.accelerometer.z
        self.draw.text((10, 30), u'x : %d' % x, fill=self.TEXT_COLOR)
        self.draw.text((10, 50), u'y : %d' % y, fill=self.TEXT_COLOR)
        self.draw.text((10, 70), u'z : %d' % z, fill=self.TEXT_COLOR)

        x, y, z = self.rotation.x, self.rotation.y, self.rotation.z
        self.draw.text((120, 30), u'x : %d' % x, fill=self.TEXT_COLOR)
        self.draw.text((120, 50), u'y : %d' % y, fill=self.TEXT_COLOR)
        self.draw.text((120, 70), u'z : %d' % z, fill=self.TEXT_COLOR)
        
        self.draw.text((10, 130),
                       u'ALS : %s' % get_logicalname(SensrvAmbientLightData, self.ALS.ambient_light),
                       fill=self.TEXT_COLOR)

        self.draw.text((10, 150),
                       u'proxi : %s' % get_logicalname(ProximityState, self.proxi.proximity_state),
                       fill=self.TEXT_COLOR)

        self.draw.text((10, 170),
                       u'orientation : %s' % get_logicalname(SensrvDeviceOrientation,
                                             self.orientation.device_orientation),
                       fill=self.TEXT_COLOR)

        self.draw.text((10, 190),
                       u'double taps : %d' % self.doubletapCounter,
                       fill=self.TEXT_COLOR)

        self.draw.text((10, 210),
                       u'enabled doubletap axis: x=%d y=%d z=%d' % self.doubletapAxis,
                       fill=self.TEXT_COLOR)

    def run(self):
        self.accelerometer.start_listening()
        self.rotation.start_listening()
        self.magenticnorth.start_listening()
        self.ALS.start_listening()
        self.proxi.start_listening()
        self.orientation.start_listening()
        self.doubletap.start_listening()
        App.run(self)

    def quit(self):
        self.accelerometer.stop_listening()
        self.rotation.stop_listening()
        self.magenticnorth.stop_listening()
        self.ALS.stop_listening()
        self.proxi.stop_listening()
        self.orientation.stop_listening()
        self.doubletap.stop_listening()
        print "+++++++++++++++++"
        App.quit(self)


if __name__ == '__main__':
    d = DemoApp()
    d.run()
