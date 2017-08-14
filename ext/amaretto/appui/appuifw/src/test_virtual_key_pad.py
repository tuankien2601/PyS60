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

import appuifw
import e32
from key_codes import *

key_type = {0: u'LeftArrowKey', 1: u'RightArrowKey', 2: u'UpArrowKey',
            3: u'DownArrowKey', 4: u'SelectKey'}


class TestVirtualKeyPad():

    def __init__(self):
        self.canvas = None
        appuifw.app.body = self.canvas = \
                            appuifw.Canvas(redraw_callback=self.refresh,
                                           resize_callback=self.refresh)
        appuifw.app.exit_key_handler = self.quit
        self.canvas.clear(0x1234567)
        self.running = True
        self.count = 0
        self.prev_key = -1
        self.canvas.bind(EKeyLeftArrow, lambda: self.event_called(0))
        self.canvas.bind(EKeyRightArrow, lambda: self.event_called(1))
        self.canvas.bind(EKeyUpArrow, lambda: self.event_called(2))
        self.canvas.bind(EKeyDownArrow, lambda: self.event_called(3))
        self.canvas.bind(EKeySelect, lambda: self.event_called(4))
        appuifw.app.focus = self.refresh
        appuifw.app.menu = [(u"Screen Area",
                                ((u"Large",
                                    lambda: self.screen_area("large")),
                                (u"Full",
                                    lambda: self.screen_area("full")),
                                (u"Normal",
                                    lambda: self.screen_area("normal")))),
                            (u"Orientation",
                                ((u'Landscape',
                                    lambda: self.orientation("landscape")),
                                (u'Portrait',
                                    lambda: self.orientation("portrait")),
                                 (u'Automatic',
                                    lambda: self.orientation("automatic")))),
                            (u"UI Controls",
                                ((u"Text",
                                    lambda: self.text_control()),
                                 (u"ListBox",
                                    lambda: self.listbox_control()),
                                 (u"Canvas",
                                    lambda: self.canvas_control()))),
                            (u"Direction pad",
                                ((u"On",
                                    lambda: self.directional_pad(True)),
                                 (u"Off",
                                    lambda:
                                        self.directional_pad(False))))]

    def refresh(self, rect):
        if self.canvas:
            self.canvas.clear(0x1234567)

    def run(self):
        while self.running:
            e32.ao_sleep(0.01)

    def quit(self):
        appuifw.app.exit_key_handler = None
        self.canvas = None
        self.running = False

    def orientation(self, orientation):
        appuifw.app.orientation = orientation
        self.refresh((0, 0, 0, 0))

    def event_called(self, key_id):
        self.refresh((0, 0, 0, 0))
        if self.prev_key != key_id:
            self.count = 0
        self.canvas.text((20, 40), unicode(key_type[key_id]) + u' is pressed')
        self.canvas.text((20, 20), u"Count :" + unicode(self.count))
        self.count += 1
        self.prev_key = key_id

    def screen_area(self, area):
        appuifw.app.screen = area
        self.refresh((0, 0, 0, 0))

    def text_control(self):
        appuifw.app.body = text_obj = appuifw.Text()
        text_obj.add(u"The Virtual Key Pad should not be visible in this mode")

    def listbox_control(self):
        entries = [u'Item1', u'Item2', u'Item3']
        appuifw.app.body = self.list_box = \
                                    appuifw.Listbox(entries)
        appuifw.note(u'Listbox view should not have the Virtual KeyPad')

    def canvas_control(self):
        appuifw.app.body = self.canvas
        if not appuifw.app.directional_pad:
            appuifw.note(u'Canvas view restored without Virtual KeyPad')
        else:
            appuifw.note(u'Canvas view restored with the Virtual KeyPad')
        self.refresh((0, 0, 0, 0))

    def directional_pad(self, value):
        appuifw.app.directional_pad = value
        appuifw.note(
            u'Switching body to canvas to verify directional_pad operation')
        appuifw.app.body = self.canvas
        self.refresh((0, 0, 0, 0))

if __name__ == '__main__':
    obj = TestVirtualKeyPad()
    obj.run()
