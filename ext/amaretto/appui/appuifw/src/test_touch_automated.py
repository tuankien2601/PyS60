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

import sys
sys.path.append('c:\\data\\python')
import test.test_support
import unittest
import appuifw
from graphics import *
from key_codes import *


class TestFramework(unittest.TestCase):

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        self.canvas = appuifw.Canvas(event_callback=self.event_callback,
                                     redraw_callback=self.redraw_callback)
        self.old_body = appuifw.app.body
        appuifw.app.body = self.canvas
        appuifw.app.focus = self.focus_monitor
        self.img = Image.new(self.canvas.size)
        self.scenario_one = ((20, 20), (75, 80), [25, 50])
        self.scenario_two = ([10, 20], [30, 50])
        self.scenario_three = [(20, 20), (40, 40)]
        self.scenario_four = ((100, 100))

    def focus_monitor(self, value):
        if value:
            self.canvas.blit(self.img)

    def set_exit(self, pos):
        appuifw.app.body = self.old_body
        self.canvas = None
        self.img = None
        self.running = False

    def redraw_callback(self, rect):
        pass

    def event_callback(self, event):
        pass

    def test_bind_tuple(self):
        """Passing a tuple of three tuples should raise exception"""
        try:
            self.canvas.bind(EButton1Up, self.event_callback, \
                             ((0, 100), (0, 100), (10, 20)))
        except Exception, err:
            print "Exception: ", err
        else:
            self.fail("Passing three tuples is working")

    def test_binding_check1(self):
        """Passing a tuple having tuples and lists should raise exception"""
        try:
            self.canvas.bind(EButton1Up, self.set_exit, self.scenario_one)
        except Exception, err:
            print "Exception: ", err

    def test_binding_check2(self):
        """Passing a tuple of lists should raise exception"""
        try:
            self.canvas.bind(EButton1Up, self.set_exit, self.scenario_two)
        except Exception, err:
            print "Exception: ", err

    def test_bind_check3(self):
        """Passing co-ordinates as a list should raise exception"""
        try:
            self.canvas.bind(EButton1Up, self.set_exit, self.scenario_three)
        except Exception, err:
            print "Exception: ", err

    def test_bind_check4(self):
        """Passing tuple of one tuple should raise exception"""
        try:
            self.canvas.bind(EButton1Up, self.set_exit, self.scenario_four)
        except Exception, err:
            print "Exception: ", err

    def test_bind_keyevent(self):
        """Passing co-ordinates for key events should ignore coordinates"""
        canvas2 = appuifw.Canvas(redraw_callback=self.redraw_callback)
        try:
            canvas2.bind(EKeyUpArrow, self.event_callback, \
                         ((0, 100), (0, 100)))
        except:
            self.fail("Check passing coordinates to key events")

    def test_begin_end_redraw(self):
        # Multiple begin-end redraw calls should raise exception
        try:
            self.canvas.begin_redraw((1, 1, 14, 14))
            self.canvas.begin_redraw((1, 1, 14, 14))
        except RuntimeError, err:
            pass
        except:
            raise
        else:
            raise
        finally:
            self.canvas.end_redraw()
        try:
            self.canvas.end_redraw()
        except RuntimeError, err:
            pass
        except:
            raise
        else:
            raise

        # Begin redraw takes an optional rect argument
        try:
            self.canvas.begin_redraw((1, 1, 14, 14))
            self.canvas.line((5, 5, 10, 10), outline=0xffffff, width=4,
                           fill=0xffffff)
            self.canvas.end_redraw()
            self.canvas.begin_redraw()
            self.canvas.line((5, 5, 10, 10), outline=0xffffff, width=4,
                           fill=0xffffff)
            self.canvas.end_redraw()
        except:
            raise

        # Pass invalid argument to begin redraw
        try:
            self.canvas.begin_redraw("1")
        except TypeError, err:
            pass
        except:
            raise

        # Test couple of graphics draw methods within the begin-end redraw
        try:
            self.canvas.begin_redraw()
            self.canvas.line((5, 5, 10, 10))
            self.canvas.polygon((1, 1, 5, 1, 3, 3))
            self.canvas.rectangle((0, 0, 0, 5, 5, 5, 5, 0))
            self.canvas.ellipse((20, 20, 40, 40), 15, 60)
            self.canvas.end_redraw()
        except:
            raise

    def test_event_callback(self):
        """ Passing a non callable parameter should raise an exception"""
        try:
            event_noncallable = "Just an event"
            canvas3 = appuifw.Canvas(event_callback=event_noncallable,
                                     redraw_callback=self.redraw_callback)
        except Exception, err:
            print "Exception: ", err
        else:
            self.fail("The binding accepts non callable parameter")

    def test_touch_enabled(self):
        """Test if touch is enabled on the device"""
        try:
            if appuifw.touch_enabled():
                print "Yes. It is!"
            else:
                print "No!"
        except:
            self.fail("touch_enabled() not working")


def test_main():
    test.test_support.run_unittest(TestFramework)

if __name__ == "__main__":
    test_main()
