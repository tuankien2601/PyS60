# Copyright (c) 2005-2009 Nokia Corporation
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
import unittest
from test.test_support import TestSkipped, run_unittest

if e32.in_emulator():
    raise TestSkipped('Cannot test on the emulator')

import os
import shutil
import appuifw
import camera
 
# Set this to `True` when writing to a goldfile
create_goldfile = False

camera_goldfile = "c:\\data\\python\\test\\camera_goldfile.txt"
camera_test_data = {}
save_path = u'c:\\data\\python\\test\\'


class CameraTest(unittest.TestCase):

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)

    def _test_util(self, test_key, test_data):
        global camera_test_data
        if create_goldfile:
            camera_test_data[test_key] = test_data
        else:
            self.assertEqual(camera_test_data[s60_version][test_key],
                                                                     test_data)

    def delete_files_if_exist(self, filenames, path):
        for filename in filenames:
            if os.path.exists(path + filename):
                os.remove(path + filename)

    def move_test_data(self, filenames):
        testcasedata_dir = save_path + 'testcasedata\\'
        if not os.path.exists(testcasedata_dir):
            os.mkdir(testcasedata_dir)
        self.delete_files_if_exist(filenames, testcasedata_dir)
        for filename in filenames:
            # Move the files(snaps or video) to the testcasedata_dir folder
            shutil.move(save_path + filename, testcasedata_dir)

    def test_cameras_available(self):
        self._test_util('cameras_available', camera.cameras_available())

    def test_image_modes(self):
        self._test_util('image_modes', camera.image_modes())

    def test_image_sizes(self):
        self._test_util('image_sizes', camera.image_sizes())

    def test_flash_modes(self):
        self._test_util('flash_modes', camera.flash_modes())

    def test_max_zoom(self):
        self._test_util('max_zoom', camera.max_zoom())

    def test_exposure_modes(self):
        self._test_util('exposure_modes', camera.exposure_modes())

    def test_white_balance_modes(self):
        self._test_util('white_balance_modes', camera.white_balance_modes())

    def test_take_photo(self):
        snaps = [u'low.jpg', u'middle.jpg', u'high.jpg']
        self.delete_files_if_exist(snaps, save_path)

        try:
            low_res_picture = camera.take_photo(mode='RGB12')
            low_res_picture.save(save_path + snaps[0])
            middle_res_picture = camera.take_photo(mode='RGB16')
            middle_res_picture.save(save_path + snaps[1])
            high_res_picture = camera.take_photo(mode='RGB')
            high_res_picture.save(save_path + snaps[2])
        except RuntimeError, e:
            print >> sys.stderr, "Error taking a snap :", e
            raise
        else:
            for snap in snaps:
                self.failUnless(os.path.exists(save_path + snap))
        finally:
            self.move_test_data(snaps)

    def test_start_record(self):

        def video_cb(err, code):
            self.failIf(err, "Error while recording")
            if self.counter:
                self.failUnlessEqual(code, camera.EPrepareComplete,
                                     "State is not proper")
            else:
                self.failUnlessEqual(code, camera.EOpenComplete,
                                     "State is not proper")
            self.counter += 1

        def finder_cb(im):
            appuifw.app.body.blit(im)

        self.counter = 0
        video_filename = 'test_video.3gp'
        appuifw.app.body = appuifw.Canvas()
        self.delete_files_if_exist([video_filename], save_path)

        try:
            camera.start_finder(finder_cb)
            camera.start_record(save_path + video_filename, video_cb)
            e32.ao_sleep(5)
            camera.stop_record()
            camera.stop_finder()
        except RuntimeError, e:
            print >> sys.stderr, "Error recording a video :", e
            raise
        else:
            self.failUnless(os.path.exists(save_path + video_filename))
        finally:
            self.move_test_data([video_filename])


def test_main():
    global camera_test_data
    global s60_version
    s60_version = e32.s60_version_info
    try:
        camera_test_data = eval(open(camera_goldfile, 'rt').read())
    except:
        raise TestSkipped('goldfile not present')
    run_unittest(CameraTest)


if __name__ == "__main__":
    test_main()
