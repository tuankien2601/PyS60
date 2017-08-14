# Copyright (c) 2007 - 2009 Nokia Corporation
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
import appuifw
import sensor
from sensor import orientation

orientations = {orientation.TOP:    'top',
                orientation.BOTTOM: 'bottom',
                orientation.LEFT:   'left',
                orientation.RIGHT:  'right',
                orientation.FRONT:  'front',
                orientation.BACK:   'back'}

sensors_supported = sensor.sensors()


def test_sensors_supported():
    print "Sensors supported on this device are: ", sensors_supported


def test_rotation_sensor():

    def rotation_callback(orientation_value):
        print 'New orientation: ', orientations[orientation_value]

    if 'RotSensor' not in sensors_supported:
        print "Rotation sensor not supported by this device"
        return

    rot_sensor_data = sensors_supported['RotSensor']
    rot_sensor = sensor.Sensor(rot_sensor_data['id'],
                               rot_sensor_data['category'])

    print 'Rotate the phone to validate orientation..(Testing for 15 seconds)'
    rot_sensor.set_event_filter(sensor.RotEventFilter())
    rot_sensor.connect(rotation_callback)

    e32.ao_sleep(15)

    rot_sensor.disconnect()
    print 'Rotation sensor test finished'


def test_acceleration_sensor():

    def acceleration_callback(orientation_value):
        print 'New orientation: ', orientations[orientation_value]

    if 'AccSensor' not in sensors_supported:
        print "Acceleration sensor not supported by this device"
        return

    acc_sensor_data = sensors_supported['AccSensor']
    orientation_sensor = sensor.Sensor(acc_sensor_data['id'],
                                       acc_sensor_data['category'])

    print 'The orientation value should change when phone is rotated. ' +\
          'Testing for 15 seconds...'
    orientation_sensor.set_event_filter(sensor.OrientationEventFilter())
    orientation_sensor.connect(acceleration_callback)

    e32.ao_sleep(15)

    orientation_sensor.disconnect()
    print 'Acceleration sensor test finished'


def exit_handler():
    appuifw.app.exit_key_handler = old_exit_handler
    lock.signal()

if __name__ == "__main__":
    # This sensor test should be run only on a 3.1 device
    if not e32.s60_version_info == (3, 1):
        raise RuntimeError("Sensor API not supported on this device")

    lock = e32.Ao_lock()
    appuifw.app.menu=[
                     (u'Query Supported Sensors', test_sensors_supported),
                     (u'Test Rotation Sensor', test_rotation_sensor),
                     (u'Test Acceleration Sensor', test_acceleration_sensor),
                     (u'Exit', lock.signal)]
    old_exit_handler = appuifw.app.exit_key_handler
    appuifw.app.exit_key_handler = exit_handler
    lock.wait()
