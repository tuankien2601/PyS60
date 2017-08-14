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

import os
import time
import e32
import appuifw
from key_codes import *
from appuifw import FFormEditModeOnly, FFormViewModeOnly, FFormAutoLabelEdit,\
                    FFormAutoFormEdit, FFormDoubleSpaced


def app_quit():
    app_lock.signal()


def list_box_callback():
    appuifw.app.body = old_body
    print "--Callback called--"
    print "Item index : ", list_box.current()


def generate_listbox_entries(with_icons=False, list_type='single'):
    entries = []
    icon = None
    icon_file = u"z:\\resource\\apps\\avkon2.mbm"
    for icon_id in range(0, 200, 2):
        row1 = u"Icon: " + unicode(str(icon_id))
        row2 = u"Mask: " + unicode(str(icon_id + 1))
        if with_icons:
            icon = appuifw.Icon(icon_file, icon_id, icon_id + 1)
        if list_type == 'double':
            if icon is not None:
                entries.append((row1, row2, icon))
            else:
                entries.append((row1, row2))
        else:
            if icon is not None:
                entries.append((row1, icon))
            else:
                entries.append(row1)
    return entries


def single_list_box(show_icons=False):
    global list_box
    if show_icons:
        entries = generate_listbox_entries(with_icons=True)
    else:
        entries = generate_listbox_entries()
    list_box = appuifw.Listbox(entries, list_box_callback)
    appuifw.app.body = list_box


def double_list_box(show_icons=False):
    global list_box
    if show_icons:
        entries = generate_listbox_entries(with_icons=True, list_type='double')
    else:
        entries = generate_listbox_entries(list_type='double')
    list_box = appuifw.Listbox(entries, list_box_callback)
    appuifw.app.body = list_box


def test_form_type():
    fields = [(u"Text", "text", u"Check that this line is wrapped and " +
                "that it spans more than a line. "),
              (u"Integer", 'number', 12),
              (u"Date - 2009-05-18 10:10:00", 'date', 1242641400.0),
              (u"Time - 10:00", 'time', float(10*60*60)),
              (u"Combo", 'combo', ([u"1", u"2", u"3"], 0)),
              (u"Float", 'float', 0.12345)]
    f = appuifw.Form(fields, appuifw.FFormEditModeOnly)
    f.execute()


def test_form_insert_pop():

    def menu_callback1():
        appuifw.note(u"Menu callback1 hit!")

    def menu_callback2():
        appuifw.note(u"Menu callback2 hit!")

    f = appuifw.Form(get_simple_form(), FFormEditModeOnly)
    f.menu = [(u'Menu Item 1', menu_callback1),
              (u'Menu Item 2', menu_callback2)]
    f.execute()
    # Insert an item into the form
    item_value = appuifw.multi_query(u'Item index', u'Field descriptor')
    if item_value is not None:
        f.insert(int(item_value[0]),
                 (u'Text', 'text', unicode(item_value[1])))
        appuifw.note(u"Item inserted!")
    f.execute()
    # Remove a field from the form
    item = f.pop()
    appuifw.note(u"Item : " + unicode(item) + u"popped!")
    f.execute()


def get_simple_form():
    result = []
    result.append(
      (u"Text", 'text', u'This looks ok even though there are multiple lines'))
    result.append((u"Number", 'number', 99))
    result.append((u"Combo", "combo", ([u"1", u"2", u"3"], 0)))
    return result


def test_form_flags():
    flags = [FFormEditModeOnly, FFormViewModeOnly, FFormAutoLabelEdit,
             FFormAutoFormEdit, FFormDoubleSpaced]
    options = [u'EditModeOnly', u'ViewModeOnly', u'AutoLabelEdit',
               u'AutoFormEdit', u'DoubleSpaced']
    item_index = 0
    while item_index is not None:
        item_index = appuifw.selection_list(options)
        if item_index is not None:
            f = appuifw.Form(get_simple_form(), flags[item_index])
            f.execute()


def test_text_widget():
    text_styles = {'bold': appuifw.STYLE_BOLD, 'italic': appuifw.STYLE_ITALIC,
                   'underline': appuifw.STYLE_UNDERLINE,
                   'strikethrough': appuifw.STYLE_STRIKETHROUGH}
    text_highlight = {'standard': appuifw.HIGHLIGHT_STANDARD,
                      'rounded': appuifw.HIGHLIGHT_ROUNDED,
                      'shadow': appuifw.HIGHLIGHT_SHADOW}

    def text_callback():
        appuifw.note(u"Text callback hit!")

    old_body1 = appuifw.app.body
    appuifw.app.body = t = appuifw.Text()
    t.font = appuifw.available_fonts()[0]
    t.color = 0x004000 # Dark green

    appuifw.note(u"Testing different styles")
    for style in text_styles:
        for highlight in text_highlight:
            appuifw.note(u"Setting style: " + unicode(style) + " Highlight: " +
                         unicode(highlight))
            t.style = (text_styles[style] | text_highlight[highlight])
            t.set(u"This is a dummy text written using the set command. ")
            e32.ao_sleep(2)

    e32.ao_sleep(2)
    appuifw.note(u"Adding some more text...")
    t.add(u"Appending more text using add command")
    e32.ao_sleep(2)
    appuifw.note(u"Total length: " + unicode(t.len()))
    appuifw.note(u"Deleting 10 chars from pos 5")
    t.delete(5, 10)
    e32.ao_sleep(2)
    appuifw.note(u"Current cursor position : " + unicode(t.get_pos()))
    appuifw.note(u"Setting the cursor pos to 0")
    t.set_pos(0)
    e32.ao_sleep(2)
    appuifw.note(u"Clearing text contents")
    t.clear()
    e32.ao_sleep(2)
    t.bind(EKeyEnter, text_callback)
    appuifw.note(u"Enter key is bound to a function. Press it now!")
    t.set(u"Sleeping for 5 secs")
    e32.ao_sleep(5)
    appuifw.note(u"Testing of Text widget is complete.")
    appuifw.app.body = old_body1


def test_list_query():
    query_types = ['text', 'code', 'number', 'query', 'float', 'date', 'time']
    for query in query_types:
        appuifw.note(u'Testing query type: ' + unicode(query))
        value = appuifw.query(u'Test query', query)
        if value is not None:
            if query == 'date':
                value = time.strftime("%d/%m/%y", time.localtime(value))
            elif query == 'time':
                value = time.strftime("%H:%M", time.gmtime(value))
            appuifw.note(u'Value : ' + unicode(value))
        else:
            appuifw.note(u'Query cancelled')

    single_item_list = [unicode(x) for x in range(10)]
    double_item_list = [(unicode(x), unicode(x + 1)) for x in range(10)]
    appuifw.note(u'Testing single item list')
    value = appuifw.popup_menu(single_item_list, u'Single item list')
    appuifw.note(u'Selected item : ' + unicode(value))
    appuifw.note(u'Testing double item list')
    value = appuifw.popup_menu(double_item_list, u'Double item list')
    appuifw.note(u'Selected item : ' + unicode(value))

    appuifw.note(u'Testing selection list with search field')
    value = appuifw.selection_list(single_item_list, search_field=1)
    appuifw.note(u'Selected item : ' + unicode(value))

    for selection_style in ['checkbox', 'checkmark']:
        appuifw.note(u'Testing multi-selection list, style - ' +
                     unicode(selection_style))
        value = appuifw.multi_selection_list(single_item_list,
                                             style=selection_style)
        appuifw.note(u'Selected items : ' + unicode(value))


def test_content_handler():

    def content_handler_callback():
        appuifw.note(u"File opened. CH callback hit!")

    ch = appuifw.Content_handler(content_handler_callback)
    fp = open("c:\\data\\python\\temp.txt", "w")
    fp.write("FOO BAR")
    fp.close()
    appuifw.note(u"Opening temp.txt in embedded mode")
    ch.open(u"c:\\data\\python\\temp.txt")
    os.remove("c:\\data\\python\\temp.txt")
    appuifw.note(u"Opening snake.py in standalone mode")
    ch.open_standalone(u"c:\\data\\python\\snake.py")


def test_info_popup():
    global i
    alignment = {'LeftVTop': appuifw.EHLeftVTop,
                 'LeftVCenter': appuifw.EHLeftVCenter,
                 'LeftVBottom': appuifw.EHLeftVBottom,
                 'CenterVTop': appuifw.EHCenterVTop,
                 'CenterVCenter': appuifw.EHCenterVCenter,
                 'CenterVBottom': appuifw.EHCenterVBottom,
                 'RightVTop': appuifw.EHRightVTop,
                 'RightVCenter': appuifw.EHRightVCenter,
                 'RightVBottom': appuifw.EHRightVBottom}
    i = appuifw.InfoPopup()
    for x, item in enumerate(alignment):
        i.show(u"Tip position: " + item, (50, 50), 5000, 0, alignment[item])
        e32.ao_sleep(6)


appuifw.app.menu = [
    (u'Listbox Test',
        ((u'SingleStyleListBox', single_list_box),
        (u'DoubleStyleListBox', double_list_box),
        (u'SingleStyleGraphicListBox', lambda:single_list_box(True)),
        (u'DoubleStyleGraphicListBox', lambda:double_list_box(True)))),
    (u'Form Test',
        ((u'TestFormType', test_form_type),
        (u'TestFormFlags', test_form_flags),
        (u'TestFormInsertPop', test_form_insert_pop))),
    (u'Text Widget Test', test_text_widget),
    (u'Query & List', test_list_query),
    (u'Content Handler Test', test_content_handler),
    (u'Info Popup Test', test_info_popup),
    (u'Exit', app_quit)]


app_lock = e32.Ao_lock()
old_body = appuifw.app.body
appuifw.app.exit_key_handler = app_quit
app_lock.wait()
