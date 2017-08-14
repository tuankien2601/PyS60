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

excluded_modules = []
module_paths = []
pyd_path_name = []
    

def get_mod_list(arg, dirname, names):
    for name in names:
        # This check is done for not including the test_ files for modules
        if not name.startswith('test_') and name.endswith('.py'):
            path = os.path.join(dirname, name)
            path = path.replace(lib_path + '\\', '')
            # This check for 'test' is done to remove those files which fall
            # under test or crashers
            if not 'test' in path:
                module_paths.append(path)


def gen_excluded_list():
    global excluded_modules
    py_module = []
    module_type = []
    pyd_modules = []
    builtin_modules = []
    modules_cfg_path = \
            os.path.abspath('..\\newcore\\Symbian\\src\\modules.cfg')
    config_file = open(modules_cfg_path, 'r')
    for line in config_file:
        line = line.strip()
        if line in ['PYD', 'PY_MODULES', 'BUILTIN', 'EXT-MODULES',
                    'EXT-PYD', 'EXT-MOD-CFGS']:
            module_type = line
            continue
        if line and line[0] != '#':
            if module_type == 'PYD':
                pyd_modules.append(line.split(',')[0])
            elif module_type == 'BUILTIN':
                builtin_modules.append(line.split('=')[0])
            elif module_type == 'PY_MODULES':
                py_module.append(line.split(':')[1])
    excluded_modules = py_module + builtin_modules + pyd_modules


def get_pyd_list(arg, dirname, names):
    for name in names:
        if name.endswith('.c'):
            path = os.path.join(dirname, name)
            buf = open(path, 'r').read()
            if "Py_InitModule" in buf:
                pyd_path_name.append(path)


def add_pyd():
    pyd_name = []
    for pyd in pyd_path_name:
        fp = open(pyd, 'r')
        file_name = os.path.basename(pyd)
        for line in fp:
            if "Py_InitModule" in line:
                try:
                    pyd_name.append(line.split('"')[1])
                except:
                    # Some files do not have their pyd names in Py_InitModule,
                    # a split on these files would not yield the names so their
                    # names are hard coded
                    if file_name == 'unicodedata.c':
                        pyd_name.append('unicodedata')
                    elif file_name == 'posixmodule.c':
                        pyd_name.append('posix')
                    elif file_name == 'pyexpat.c':
                        pyd_name.append('pyexpat')
                    elif file_name == 'socketmodule.c':
                        pyd_name.append('_socket')
                    elif file_name == '_bsddb.c':
                        pyd_name.append('_bsddb')
                    continue
        fp.close()                             
    module_paths.extend(pyd_name)


def gen_mod_dep_cfg_template():
    # These are the module dependency map for modules that are not
    # found by the module_config_parser and hence they need an explicit mention
    additional_modules = {'__main__': {'type': 'base', 'deps': []},
                          '__builtin__': {'type': 'base', 'deps': []},
                          'sys': {'type': 'base', 'deps': []},
                          'nt': {'type': 'excluded', 'deps': []},
                          'win32evtlogutil':{'type': 'excluded', 'deps': []},
                          'win32evtlog': {'type': 'excluded', 'deps': []}}

    for module in excluded_modules:
        if module in module_paths:
            module_paths.remove(module)

    # The exclude py_files() are parsed here
    dep_map = {}
    mod_dep_cfg_template_path = \
    os.path.abspath(
                '..\\newcore\\Symbian\\src\\module_dependency.cfg.template')
    dep_map = eval(open(mod_dep_cfg_template_path, 'r').read())
    dep_map.update(additional_modules)
    mod_map = {}
    for mod in module_paths:
        mod_map['type'] = 'excluded'
        mod_map['deps'] = []
        mod_name = os.path.splitext(mod)[0].replace('\\', '.')
        dep_map[mod_name] = mod_map
    open(mod_dep_cfg_template_path, 'w').write(str(dep_map))

    fp = open(mod_dep_cfg_template_path, 'r')
    buf = fp.read()
    buf = buf.replace("}, '", "}, \n'")
    fp.close()
    open(mod_dep_cfg_template_path, 'w').write(buf)

if __name__ == "__main__":
    # Walk through the Lib path for inbuilt modules
    lib_path = os.path.abspath("..\\newcore\\Lib")
    os.path.walk(lib_path, get_mod_list, None)
    gen_excluded_list()
    os.path.walk(os.path.abspath("..\\newcore\\Modules"), get_pyd_list, None)
    add_pyd()
    gen_mod_dep_cfg_template()
