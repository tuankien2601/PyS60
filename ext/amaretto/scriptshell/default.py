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

import sys
import os
import appuifw
import series60_console
import e32


class SelfClearingNamespace:

    def __init__(self):
        self.namespace={'__builtins__': __builtins__,
                        '__name__': '__main__'}

    def __del__(self):
        # Here's a subtle case. The default namespace is deleted at interpreter
        # exit, but these namespaces aren't since all but the most trivial
        # scripts create reference cycles between the functions defined in the
        # script and the namespace of the script. To avoid this we explicitly
        # clear the old namespace to break these reference cycles.
        self.namespace.clear()

script_namespace = SelfClearingNamespace()


def query_and_exec():

    def is_py(x):
        ext=os.path.splitext(x)[1].lower()
        return ext == '.py' or ext == '.pyc' or ext == '.pyo'

    script_list = []
    script_names = []
    final_script_list = []
    final_script_names = []
    for nickname, path in script_dirs:
        if os.path.exists(path):
            script_list = map(lambda x: (nickname + x, path + '\\' + x),
                               map(lambda x: unicode(x, 'utf-8'),
                                   filter(is_py, os.listdir(path))))
            script_names = map(lambda x: unicode(x[0]), script_list)
            script_names.reverse()
            script_list.reverse()
            final_script_list += script_list
            final_script_names += script_names

    index = appuifw.selection_list(final_script_names)
    # We make a fresh, clean namespace every time we run a new script, but
    # use the old namespace for the interactive console and btconsole sessions.
    # This allows you to examine the script environment in a console
    # session after running a script.
    global script_namespace

    script_namespace = SelfClearingNamespace()
    if index >= 0:
        execfile(final_script_list[index][1].encode('utf-8'),
                 script_namespace.namespace)


def exec_interactive():
    import interactive_console
    interactive_console.Py_console(my_console).interactive_loop(
                                                    script_namespace.namespace)


def exec_btconsole():
    import btconsole
    btconsole.main(script_namespace.namespace)


def set_defaults():
    appuifw.app.body = my_console.text
    appuifw.app.title = u'Python'
    sys.stderr = sys.stdout = my_console
    appuifw.app.screen = 'normal'
    appuifw.app.orientation = 'automatic'
    if appuifw.touch_enabled():
        appuifw.app.directional_pad = True
    else:
        appuifw.app.directional_pad = False
    init_options_menu()


def show_copyright():
    print str(copyright) + u"\nSee www.python.org for more information."


def menu_action(f):
    appuifw.app.menu = []
    saved_exit_key_handler = appuifw.app.exit_key_handler

    try:
        try:
            f()
        finally:
            appuifw.app.exit_key_handler = saved_exit_key_handler
            set_defaults()
    except:
        import traceback
        traceback.print_exc()


def init_options_menu():
    appuifw.app.menu = [(u"Run script",
                         lambda: menu_action(query_and_exec)),
                        (u"Interactive console",
                         lambda: menu_action(exec_interactive)),
                        (u"Bluetooth console",
                         lambda: menu_action(exec_btconsole)),
                        (u"About",
                         lambda: menu_action(show_copyright))]

# In case of emu, the path to non-base modules needs to be added
if e32.in_emulator():
    sys.path.append('c:\\resource\\python25\\python25_repo.zip')

script_dirs = [(u'c:', 'c:\\data\\python'), (u'e:', 'e:\\python'),
               (u'e:', 'e:\\data\\python')]
for path in ('c:\\data\\python\\lib', 'e:\\python\\lib'):
    if os.path.exists(path):
        sys.path.append(path)

my_console = series60_console.Console()
set_defaults()
lock = e32.Ao_lock()
appuifw.app.exit_key_handler = lock.signal
print 'Python for S60 Version ' + e32.pys60_version
print 'Capabilities Present:', e32.get_capabilities()
print '\nSelect:'
print '"Options -> Run script" to run a script'
print '"Options -> About" to view copyright\n'
lock.wait()
appuifw.app.set_exit()
