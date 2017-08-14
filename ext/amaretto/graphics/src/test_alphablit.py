# Copyright (c) 2005 Nokia Corporation
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
import graphics
from appuifw import *

app.body=c=Canvas()

alphaimg=graphics.Image.new((256,256),'L')
alphaimg.clear(0)
for y in xrange(256):
    alphaimg.line((0,y,255,y),outline=(y,y,y))

textimg1=graphics.Image.new((256,256))
textimg2=graphics.Image.new((256,256))

def dodraw(target, text, color):
    target.clear(0)
    for y in xrange(0,256,30):
        target.text((20,y),text,fill=color,font='dense')

dodraw(textimg1, u'Lorem ipsum',0xff0000)
dodraw(textimg2, u'dolor sit amet',0x00ff00)

canvasimg=graphics.Image.new(c.size)

for x in range(100):
    canvasimg.clear(0)
    canvasimg.blit(textimg1)
    canvasimg.blit(textimg2,target=(x,x),mask=alphaimg)
    c.blit(canvasimg)
    e32.ao_sleep(0.01)

e32.ao_sleep(1)
