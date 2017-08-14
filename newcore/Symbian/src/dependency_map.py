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


def get_dependency(path):
    cmd1 = 'python c:\\APPS\\actpython\\Scripts\\'
    cmd2 = 'sfood-imports -u %s > temp.txt' % path
    dep_cmd = cmd1 + cmd2
    os.system(dep_cmd)
    key = path.split('\\')[-1]

    new_path = path.replace('\\', '\\\\').rstrip(key)
    if os.path.exists('temp.txt'):
        f = open("temp.txt", 'r')
        module_list = []
        for line in f:
            module_name = line.strip()
            if module_name not in module_list:
                module_list.append(module_name)

        module_deps[key] = module_list
        open("module_dependency.cfg", 'w').write(str(module_deps))
        f.close()

if __name__ == "__main__":
    py_module_path = []
    module_type = []
    module_deps = {}
    config_file = open('modules.cfg', 'r')
    for line in config_file:
        line = line.strip()
        if line == 'PY_MODULES':
            module_type = line
            continue
        if line and line[0] != '#':
            if module_type == 'PY_MODULES':
                py_module_path.append(line.split(':')[0] + line.split(':')[1])

    for path in py_module_path:
        get_dependency(path)

    if os.path.exists('module_dependency.cfg'):
        fp = open("module_dependency.cfg", 'r')
        buf = fp.read()
        buf = buf.replace("], ", "], \n")
        fp.close()
        open("module_dependency.cfg", 'w').write(buf)
    os.remove('temp.txt')
