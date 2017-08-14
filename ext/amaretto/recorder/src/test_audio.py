# Copyright (c) 2005 - 2009 Nokia Corporation
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
import audio
import unittest
from test.test_support import TestSkipped, run_unittest


class AudioTest(unittest.TestCase):

    def __init__(self, methodName='runTest'):
        unittest.TestCase.__init__(self, methodName=methodName)
        self.sound_object = \
            audio.Sound.open(u'c:\\Data\\python\\test\\Dawning.aac')

    def test_text_to_speech(self):
        try:
            audio.say(u"Hello", prefix=audio.TTS_PREFIX)
        except:
            self.fail("Text to Speech conversion API broken")

    def test_playback(self):

        def validate(prev_state, curr_state, err_code):
            self.counter += 1
            self.failIf(err_code, "Error playing file")
            if self.counter == 1:
                self.assert_((prev_state == audio.EOpen and
                              curr_state == audio.EPlaying),
                              "State transition improper")
            else:
                self.assert_((prev_state == audio.EPlaying and
                              curr_state == audio.EOpen),
                              "State transition improper")

        self.counter = 0
        self.failUnlessEqual(self.sound_object.state(), audio.EOpen,
                             "State information is wrong")
        self.sound_object.set_volume(2)
        self.sound_object.play(times=1, interval=0, callback=validate)
        self.failUnlessEqual(self.sound_object.state(), audio.EPlaying,
                             "State information is wrong")
        e32.ao_sleep(5)

    def test_volume_functionality(self):
        self.sound_object.play()
        max_volume = self.sound_object.max_volume()
        set_volume = max_volume - 2
        self.sound_object.set_volume(set_volume)
        self.failUnlessEqual(self.sound_object.current_volume(), set_volume,
                             "Volume not set properly")
        self.sound_object.set_volume(-1)
        self.failIf(self.sound_object.current_volume(),
                    "Volume should have been set to zero")
        self.sound_object.set_volume(max_volume + 5)
        self.failUnlessEqual(self.sound_object.current_volume(), max_volume,
                             "Volume should have been set to max volume")

    def test_playback_time(self):
        time_of_playback = self.sound_object.duration()
        # Subtract 3 seconds(expressed in micro seconds) from the total
        # duration of the track and start playback from this position.
        start_position = time_of_playback - 3000000L
        self.sound_object.set_position(start_position)
        self.sound_object.play()
        # The playback starts a few milliseconds earlier than the set time on
        # Nokia 5800. Adjusting for this difference while comparing
        self.assert_((self.sound_object.current_position() >=
                      start_position - 3500L),
                     "Playback should have started at set position")

    def test_record(self):
        sound_record = audio.Sound.open(u'c:\\Data\\python\\recorded.wav')
        sound_record.record()
        e32.ao_sleep(3)
        sound_record.stop()
        self.assert_((self.sound_object.duration() >= 3),
                     "Should have recorded for atleast 3 seconds")
        sound_record.play()
        e32.ao_sleep(3)
        sound_record.stop()

    def tearDown(self):
        self.sound_object.stop()

    def __del__(self):
        self.sound_object.close()


def test_main():
    if not e32.in_emulator():
        run_unittest(AudioTest)
    else:
        raise TestSkipped('Cannot test on the emulator')

if __name__ == "__main__":
    test_main()
