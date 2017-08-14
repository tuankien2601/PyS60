# Copyright (c) 2008 Nokia Corporation
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

from appuifw import *
from graphics import *
import e32
import time

app.body=c=Canvas()

fonts=[None,
       (None,20),
       'normal',
       ('normal',20),
       (u'fasfsafs',20),
       (u'Nokia Sans S60',10,0),
       (u'Nokia Sans S60',20),
       'dense',
       'title',
       (None,10,FONT_BOLD),
       (None,10,FONT_ITALIC),
       (None,10,FONT_ANTIALIAS),
       (None,10,FONT_NO_ANTIALIAS),
       ('normal',None,FONT_BOLD),
       ('normal',None,FONT_ITALIC),
       ('normal',None,FONT_NO_ANTIALIAS)      
       ]

def font_canvas():
    app.body=c=Canvas()
    c.clear()
    tl=(0,0)
    for font in fonts:
        text_to_draw=u'font:'+str(font)
        (bbox,advance,ichars)=c.measure_text(text_to_draw, font=font)
        print bbox, advance, ichars    
        # position the next piece of text so the top-left corner of its bounding box is at tl.    
        o=(tl[0]-bbox[0],tl[1]-bbox[1])
        # Draw the bounding box of the text as returned by measure_text
        c.rectangle((o[0]+bbox[0],o[1]+bbox[1],o[0]+bbox[2],o[1]+bbox[3]),outline=0x00ff00) 
        c.text(o, text_to_draw, font=font)                
        # Compute the new top-left.
        tl=(0,o[1]+bbox[3])

def font_image():   
    app.body=c=Canvas()
    image = Image.new(c.size) # create an image
    image.clear(0)
    y = 20    
    for font in fonts:
        text_to_draw=u'font:'+str(font)
        # Draw the full text
        image.text((0, y), text_to_draw, fill = 0x0000ff, font = font) # blue text on the image
        y += 25
        c.blit(image) # copy the image's contents to the lower part of the canvas

def measure_text():
    app.body=c=Canvas()
    c.clear()
    tl=(0,0)
    font='normal'
    for maxwidth in (50,100,150,200,250):
        text_to_draw=u'Lorem ipsum dolor sit amet'
        (bbox,advance,maxchars)=c.measure_text(text_to_draw, font=font, maxwidth=maxwidth)
        print bbox, advance, maxchars
        # Position the next piece of text so the top-left corner of its bounding box is at tl.    
        o=(tl[0]-bbox[0],tl[1]-bbox[1])
        # Draw the bounding box of the text as returned by measure_text
        c.rectangle((o[0]+bbox[0],o[1]+bbox[1],o[0]+bbox[2],o[1]+bbox[3]),outline=0x00ff00) 
        # Draw the maximum width specification line.
        c.line((tl[0],tl[1]+10,tl[0]+maxwidth,tl[1]+10), outline=0xff0000)
        # Draw the full text
        c.text(o, text_to_draw, font=font, fill=0x000080)                
        # Draw the text limited to given number of chars.
        c.text(o, text_to_draw[0:maxchars], font=font, fill=0)                
        # Compute the new top-left.
        tl=(0,o[1]+bbox[3]+3)

def font_benchmark():
    app.body=c=Canvas()
    c.clear()
    tl=(0,0)
    font='normal'
    text=u'Lorem ipsum dolor sit amet'
    def bench(name, func):
        start=time.time()
        func()
        end=time.time()
        print "%s: %d s"%(name,end-start)
    def default_font_test(c,t):
        for k in range(1000):
            c.text((0,30),t)        
    bench("default font", lambda:default_font_test(c,text))    

def font_attributes():
    app.body=c=Canvas()
    c.clear()
    y=20
    for font in [(None,None,0),
                 (None,None,FONT_ITALIC),
                 (None,None,FONT_BOLD),
                 (None,None,FONT_ITALIC|FONT_BOLD),
                 (None,None,FONT_ANTIALIAS),
                 (None,None,FONT_SUPERSCRIPT),
                 (None,None,FONT_SUBSCRIPT),
                 (None,None,FONT_NO_ANTIALIAS)]:
        c.text((20,y), u'abcd '+str(font), font=font)
        y+=20


def font_text():
    app.body=t=Text()
    for font in fonts:
        t.font=None
        t.add(u'font:'+str(font))
        t.font=font
        t.add(u'abcd1234')
        
        

lock=e32.Ao_lock()
app.screen='full'
app.exit_key_handler=lock.signal

app.menu=[(u'Fonts on canvas', font_canvas),
          (u'Fonts on image', font_image),
          (u'Measure text', measure_text),
          (u'Font attributes', font_attributes),
          (u'Font benchmark', font_benchmark),
          (u'Fonts on Text', font_text)]
font_canvas()
lock.wait()
