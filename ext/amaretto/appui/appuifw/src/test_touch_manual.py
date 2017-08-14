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
import appuifw
import e32
from graphics import *
from key_codes import *


class WhileLoopApp:

    def __init__(self):
        appuifw.app.exit_key_handler = self.quit
        self.running = True
        self.take_screenshot = False

    def run(self):
        while self.running:
            e32.ao_sleep(1)
        self.quit()

    def quit(self):
        appuifw.app.exit_key_handler = None
        self.running = False


class TestFramework(WhileLoopApp):
    BG_COLOR = 0xffffff # white
    BORDER_COLOR = 0x000000 # black

    def __init__(self):
        WhileLoopApp.__init__(self)
        self.canvas = appuifw.Canvas(event_callback=self.event_callback,
                                     redraw_callback=self.redraw_callback)
        self.old_body = appuifw.app.body
        appuifw.app.body = self.canvas
        appuifw.app.screen = 'full'
        appuifw.app.focus = self.focus_monitor
        self.img = Image.new(self.canvas.size)
        self.y_max = self.canvas.size[1]
        self.box_width = self.canvas.size[0] / 4
        self.box_height = self.canvas.size[1] - 40
        self.clear_button = ((0, self.box_height),
                             (self.box_width, self.y_max))
        self.check1_button = ((self.box_width, self.box_height),
                             (self.box_width*2, self.y_max))
        self.check2_button = ((self.box_width*2, self.box_height),
                       (self.box_width*3, self.y_max))
        self.quit_button = ((self.box_width*3 - 25, self.box_height),
                            (self.box_width*4, self.y_max))
        self.canvas.bind(EKeyMenu, self.keyevent_callback, \
                        ((0, 100), (0, 100)))
        self.canvas.bind(EButton1Up, self.check2_button_callback,
                         self.check2_button)
        self.canvas.bind(EButton1Up, self.check1_callback, self.check1_button)
        self.canvas.bind(EButton1Up, self.reset_canvas, self.clear_button)
        self.reset_canvas()
        self.counter = 1

    def keyevent_callback(self):
        self.canvas.text((25, 280), u'Coordinates are ignored..')

    def check2_button_callback(self, pos):
        """ Check for partially overlapped binding"""
        self.canvas.text((25, 240), u'You are in Check2 callback now..')
        self.test_bind_again()
        self.test_bind_none()

    def focus_monitor(self, value):
        if value:
            self.canvas.blit(self.img)

    def clear_call(self, pos):
        """ Just to check if callback is hit properly or not """
        self.canvas.text((25, 200), u'You are in clear callback now..')
        self.canvas.bind(EButton1Up, self.reset_canvas, self.check1_button)

    def check1_callback(self, pos):
        self.canvas.text((20, 160), u'Check1 callback.You get this only once')
        self.canvas.text((20, 180), u'Press Check1 to hit clear callback now')
        self.canvas.bind(EButton1Up, self.clear_call, self.check1_button)

    def set_exit(self, pos):
        appuifw.app.body = self.old_body
        self.img = None
        self.running = False

    def reset_canvas(self, pos=(0, 0)):
        self.img.clear(self.BG_COLOR)
        buttons = [self.clear_button, self.check1_button, self.check2_button,
                   self.quit_button]
        options = [u'Clear', u'Check1', u'Check2', u'Quit']
        for button, text in zip(buttons, options):
            self.img.rectangle(button, outline=self.BORDER_COLOR, width=5)
            self.img.text((button[0][0]+25, button[0][1]+25), text,
                          fill=self.BORDER_COLOR)

        self.erase_mode = False
        self.img.rectangle(((0, 0), self.canvas.size),
                           outline=self.BORDER_COLOR, width=5)
        self.canvas.blit(self.img)

    def test_bind_none(self):
        """ Bind none to a callback and check if it works"""
        self.canvas.bind(EButton1Up, None, self.check2_button)

    def test_bind_again(self):
        """ Bind a separate callback to the same coordinates"""
        self.canvas.bind(EButton1Up, self.set_new_exit, self.quit_button)
        self.canvas.bind(EButton1Up, self.set_exit, self.quit_button)

    def set_new_exit(self):
        """ Dummy bind.This should not get called ideally"""
        self.canvas.text((25, 100), u'This is a dummy binding')

    def redraw_callback(self, rect):
        return

    def event_callback(self, event):
        """Check for keys present in the event_callback and print the same
           on the canvas"""
        if event['type'] in [EButton1Up, EButton1Down, EDrag]:
            self.canvas.text((25, 30), u'Type value is correct')
        if 'modifier' in event:
            self.canvas.text((25, 50), u'Modifier is present')
        if 'pos' in event:
            self.canvas.text((25, 70), u'Position is present')
        if 'scancode' in event or 'keycode' in event:
            self.canvas.text((25, 90), u'Scancode, Keycode present in event')
            if self.counter == 1:
                self.canvas.bind(EButton1Up, None)
            else:
                self.canvas.bind(EButton1Up, self.set_exit, self.quit_button)
            self.counter = self.counter + 1

    def run(self):
        WhileLoopApp.run(self)

    def quit(self):
        WhileLoopApp.quit(self)


if __name__ == '__main__':
    test = TestFramework()
    test.run()
