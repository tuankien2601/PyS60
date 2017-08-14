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
import unittest
import test.test_support


codecs_list = ['_codecs_jp', '_codecs_tw', '_codecs_cn', '_codecs_iso2022',
               '_codecs_hk', '_codecs_kr']


class ModuleDependencyCfgTest(unittest.TestCase):

    def __init__(self, method_name='runTest'):
        unittest.TestCase.__init__(self, methodName=method_name)
        self.cfg_path = os.path.abspath(os.path.join(
                       "..\\..\\..\\tools\\py2sis\\ensymble",
                       "module-repo\\standard-modules\\module_dependency.cfg"))
        self.cfg_data = eval(open(self.cfg_path, 'rt').read())

        self.gold_file_data = eval(open(
                                "module_dependency_goldfile.cfg", 'rt').read())

    def test_cyclic_dependency_type(self):
        errorneous_module_list = []
        for module in self.cfg_data:
            if self.cfg_data[module]['type'] != 'base':
                continue
            try:
                for mod in self.cfg_data[module]['deps']:
                    try:
                        if self.cfg_data[mod]['type'] != 'base' and \
                            self.cfg_data[mod]['type'] != 'excluded' and \
                            mod not in codecs_list:
                            if mod not in errorneous_module_list:
                                errorneous_module_list.append(mod)
                    except KeyError:
                        # Even if the gold file does not have an entry for
                        # this module then its ok to pass this module
                        # else signal the test failure
                        if mod in self.gold_file_data.keys():
                            self.fail('The entry for %s module ' % mod + \
                                      'is missing')
                        else:
                            continue
            except KeyError:
                self.fail("The 'deps' key is missing for module ", module)

        self.assertEqual(errorneous_module_list, [],
                    'The list of Modules whose "type" is not the same as' + \
                    ' its dependent are %s ' % errorneous_module_list)

    def test_duplicate_modules(self):
        config_file = open('modules.cfg', 'r')
        template_data = \
                    eval(open('module_dependency.cfg.template', 'rt').read())
        py_pyd_modules = []
        duplicate_modules = []
        for line in config_file:
            line1 = line.rstrip('\n')
            line = line1.strip()
            if line in ['PYD', 'BUILTIN', 'PY_MODULES', 'EXT-MODULES',
                        'EXT-PYD', 'EXT-MOD-CFGS']:
                module_type = line
                continue
            if line and line[0] != '#':
                if module_type == 'PYD':
                    py_pyd_modules.append(line.split(',')[0])

                elif module_type == 'PY_MODULES':
                    py_pyd_modules.append(os.path.splitext(
                                     line.split(':')[1])[0].replace('\\', '.'))
        config_file.close()
        duplicate_modules = \
                list(set(py_pyd_modules).intersection(template_data.keys()))
        self.assertEqual(duplicate_modules, [],
                    'There are Duplicate entries for %s' % duplicate_modules)

    def test_modules_and_dependencies(self):
        cfg_modules = self.cfg_data.keys()
        gold_file_modules = self.gold_file_data.keys()

        self.assertEqual(cfg_modules, gold_file_modules, "The list of " + \
                         "missing modules are %s" % list(set(
                                gold_file_modules).difference(cfg_modules)))

        for mod in cfg_modules:
            self.assertEqual(self.cfg_data[mod]['deps'],
                             self.gold_file_data[mod]['deps'],
                             "The dependency list for Module %s " % mod + \
                             "is not right")
            self.assertEqual(self.cfg_data[mod]['type'],
                             self.gold_file_data[mod]['type'],
                             "The 'type' of the module %s " % mod + \
                             "is incorrect")


def test_main():
    test.test_support.run_unittest(ModuleDependencyCfgTest)

if __name__ == "__main__":
    test_main()
