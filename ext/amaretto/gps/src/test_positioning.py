# Copyright (c) 2007-2009 Nokia Corporation
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
import positioning
from test.test_support import TestSkipped, run_unittest
import unittest

gps_data = {'modules_available': [
                  {'available': 0, 'id': 270526873, 'name': u'Bluetooth GPS'},
                  {'available': 1, 'id': 270526860, 'name': u'Assisted GPS'},
                  {'available': 1, 'id': 270526858, 'name': u'Integrated GPS'},
                  {'available': 1, 'id': 270559509, 'name': u'Network based'}],
            'default_module': 270526860,
            'module_details': {'available': 1,
                               'status': {'data_quality': 0,
                                          'device_status': 3},
                               'name': u'Integrated GPS',
                               'position_quality': {
                                    'time_to_first_fix': 1000000L,
                                    'cost': 1,
                                    'horizontal_accuracy': 10.0,
                                    'vertical_accuracy': 10.0,
                                    'time_to_next_fix': 1000000L,
                                    'power_consumption': 3},
                                    'capabilities': 127,
                                    'version': u'1.00(0)',
                                    'location': 1,
                                    'technology': 1,
                                    'id': 270526858},
            'position_info': {'satellites': {'horizontal_dop': 100,
                                             'used_satellites': 0,
                                             'vertical_dop': 100,
                                             'time': -62168275800.0,
                                             'satellites': 0,
                                             'time_dop': 100},
                              'position': {'latitude': 100,
                                           'altitude': 100,
                                           'vertical_accuracy': 100,
                                           'longitude': 100,
                                           'horizontal_accuracy': 100},
                              'course': {'speed': 100,
                                         'heading': 100,
                                         'heading_accuracy': 100,
                                         'speed_accuracy': 100}}}


class TestPositioning(unittest.TestCase):

    def test_modules(self):
        result = positioning.modules()
        counter = 0
        for entry in result:
            if entry in result:
                counter += 1
        self.failUnlessEqual(counter, 4, "List of available modules erroneous")

    def test_default_module(self):
        self.failUnlessEqual(positioning.default_module(),
                             gps_data['default_module'],
                             "Default module not set properly")

    def test_module_info(self):
        module_data = positioning. module_info(270526858)
        self.failUnlessEqual(module_data, gps_data['module_details'],
                             "Module data not matching")

    def test_select_module(self):
        positioning.select_module(270526858)
        # Check whether selecting a module changes the default module too.
        self.failUnlessEqual(positioning.default_module(),
                             gps_data['default_module'],
                             "Default module not set properly")

    def test_positioning_info(self):
        positioning.set_requestors([{"type": "service",
                                     "format": "application",
                                     "data": "test_app"}])
        event = positioning.position(course=1, satellites=1, partial=1)
        positioning.stop_position()
        self.failUnlessEqual(len(event), len(gps_data['position_info']),
                             "position API erroneous")
        self.failUnlessEqual(event.keys(), gps_data['position_info'].keys(),
                             "Testing the keys returned by postion API failed")
        compare_keys = (event['satellites'].keys() ==
                       gps_data['position_info']['satellites'].keys() and
                       event['position'].keys() ==
                       gps_data['position_info']['position'].keys() and
                       event['course'].keys() ==
                       gps_data['position_info']['course'].keys())
        self.assert_(compare_keys, "comparision of satellites, course or " +
                                   "position keys failed")

        event = positioning.position(course=0, satellites=1, partial=1)
        positioning.stop_position()
        self.assert_(event['course'] is None and
                     event['satellites'] is not None,
                     'Only course info was set to zero')

        event = positioning.position(course=1, satellites=0, partial=1)
        positioning.stop_position()
        self.assert_(event['satellites'] is None and
                     event['course'] is not None,
                     'only satellites info was set to zero')

        event = positioning.position(course=0, satellites=0, partial=1)
        positioning.stop_position()
        self.assert_(event['course'] is None and
                     event['satellites'] is None,
                     'course and satellites info were not requested for')


def test_main():
    if not e32.in_emulator():
        if e32.s60_version_info == (3, 2):
            # Not run on a 3.2 device because it does not have an internal
            # GPS receiver
            raise TestSkipped('Not run on a 3.2 device')
    else:
        raise TestSkipped('Cannot test on emulator')
    run_unittest(TestPositioning)


if __name__ == "__main__":
    test_main()
