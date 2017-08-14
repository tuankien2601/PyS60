# Copyright (c) 2005-2008 Nokia Corporation
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


from graphics import *
import sys
import os
import unittest
import test.test_support
import e32
import shutil

save_path = u'c:\\data\\python\\test\\'


class BasicGraphicsTest(unittest.TestCase):

    def delete_file_if_exists(self, filename):
        try:
            os.remove(save_path + filename)
        except OSError:
            pass

    def move_test_data(self, filename):
        testcasedata_dir = save_path + 'testcasedata\\'
        if not os.path.exists(testcasedata_dir):
            os.mkdir(testcasedata_dir)
        # Save a copy for manual verification
        shutil.copy(save_path + filename, testcasedata_dir)

    def test_take_screenshot(self):
        screenshot_image = u'screenshot.jpg'
        self.delete_file_if_exists(screenshot_image)
        try:
            screen = screenshot()
        except RuntimeError, e:
            print >>sys.stderr, "Error taking screenshot :", e
            raise
        else:
            screen.save(save_path + screenshot_image)
            self.failUnless(os.path.exists(save_path + screenshot_image))
        finally:
            self.move_test_data(screenshot_image)
            self.delete_file_if_exists(screenshot_image)

    def test_draw_shapes(self):
        original_image = u'abstract_art.jpg'
        resized_image = u'abstract_art_resized.jpg'

        def delete_test_files():
            self.delete_file_if_exists(original_image)
            self.delete_file_if_exists(resized_image)

        delete_test_files()
        abstract_art = Image.new((500, 500))
        abstract_art.rectangle((5, 5, 200, 200), fill=(255, 0, 0), outline=255,
                               width=4)
        abstract_art.line((2, 2, 200, 200), width=4, outline=255)
        abstract_art.polygon([(100, 400), (200, 500), (300, 400), (200, 250),
                            (100, 200)], fill=0, outline=25, width=4)
        abstract_art.ellipse((20, 20, 500, 100), fill=0xffff00, width=2)
        abstract_art.line((100, 400, 50, 50), width=4, outline=0x004000)
        try:
            abstract_art.text((325, 450), u'Python For S60!', font='normal')
        except ValueError, e:
            print >>sys.stderr, e
        abstract_art.text((325, 400), u'Python For S60!', font=(None, 20,
                          FONT_BOLD|FONT_ITALIC))

        abstract_art.save(save_path + original_image)
        self.failUnless(os.path.exists(save_path + original_image))

        image_details = Image.inspect(save_path + original_image)
        self.failUnlessEqual(image_details['size'], (500, 500))

        new_art = abstract_art.resize((250, 250), keepaspect=1)
        new_art.save(save_path + resized_image)
        self.failUnless(os.path.exists(save_path + resized_image))

        image_details = Image.inspect(save_path + resized_image)
        self.failUnlessEqual(image_details['size'], (250, 250))

        self.move_test_data(original_image)
        self.move_test_data(resized_image)
        delete_test_files()


def test_main():
    if e32.in_emulator():
        raise test.test_support.TestSkipped('Cannot test on emulator easily')
    test.test_support.run_unittest(BasicGraphicsTest)

if __name__ == "__main__":
    test_main()
