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

import sys
import os

DATA_EXPORTS_H_FILE = '..\\inc\\dataexports.h'
DATA_EXPORTS_C_FILE = 'dataexports.c'

exports = [('PyTypeObject', 'PyBool_Type'),
           ('PyIntObject', '_Py_ZeroStruct'),
           ('PyIntObject', '_Py_TrueStruct'),
           ('PyTypeObject', 'PyBuffer_Type'),
           ('PyTypeObject', 'PyCell_Type'),
           ('int', '_Py_CheckRecursionLimit'),
           ('volatile int', '_Py_Ticker'),
           ('int', '_Py_CheckInterval'),
           ('PyTypeObject', 'PyClass_Type'),
           ('PyTypeObject', 'PyInstance_Type'),
           ('PyTypeObject', 'PyMethod_Type'),
           ('PyTypeObject', 'PyCObject_Type'),
           ('PyTypeObject', 'PyCode_Type'),
           ('PyTypeObject', 'PyComplex_Type'),
           ('PyTypeObject', 'PyWrapperDescr_Type'),
           ('PyTypeObject', 'PyProperty_Type'),
           ('PyTypeObject', 'PyDict_Type'),
           ('PyTypeObject', 'PyEnum_Type'),
           ('PyTypeObject', 'PyReversed_Type'),
           ('PyTypeObject', 'PyFile_Type'),
           ('const char *', 'Py_FileSystemDefaultEncoding'),
           ('PyTypeObject', 'PyFloat_Type'),
           ('PyTypeObject', 'PyFrame_Type'),
           ('PyTypeObject', 'PyFunction_Type'),
           ('PyTypeObject', 'PyClassMethod_Type'),
           ('PyTypeObject', 'PyStaticMethod_Type'),
           ('PyTypeObject', 'PyGen_Type'),
           ('struct _inittab *', 'PyImport_Inittab'),
           ('struct _frozen *', 'PyImport_FrozenModules'),
           ('PyTypeObject', 'PyInt_Type'),
           ('PyTypeObject', 'PySeqIter_Type'),
           ('PyTypeObject', 'PyCallIter_Type'),
           ('PyTypeObject', 'PyList_Type'),
           ('PyTypeObject', 'PyLong_Type'),
           ('PyTypeObject', 'PyCFunction_Type'),
           ('char *', '_Py_PackageContext'),
           ('PyTypeObject', 'PyModule_Type'),
           ('PyTypeObject', 'PyType_Type'),
           ('PyTypeObject', 'PyBaseObject_Type'),
           ('PyTypeObject', 'PySuper_Type'),
           ('Py_ssize_t', '_Py_RefTotal', 'Py_REF_DEBUG'),
           ('PyObject', '_Py_NoneStruct'),
           ('PyObject', '_Py_NotImplementedStruct'),
           ('int', '_PyTrash_delete_nesting'),
           ('PyObject *', '_PyTrash_delete_later'),
           ('int', 'Py_DebugFlag'),
           ('int', 'Py_VerboseFlag'),
           ('int', 'Py_InteractiveFlag'),
           ('int', 'Py_OptimizeFlag'),
           ('int', 'Py_NoSiteFlag'),
           ('int', 'Py_UseClassExceptionsFlag'),
           ('int', 'Py_FrozenFlag'),
           ('int', 'Py_TabcheckFlag'),
           ('int', 'Py_UnicodeFlag'),
           ('int', 'Py_IgnoreEnvironmentFlag'),
           ('int', 'Py_DivisionWarningFlag'),
           ('int', '_Py_QnewFlag'),
           ('PyObject *', 'PyExc_BaseException'),
           ('PyObject *', 'PyExc_Exception'),
           ('PyObject *', 'PyExc_StopIteration'),
           ('PyObject *', 'PyExc_GeneratorExit'),
           ('PyObject *', 'PyExc_StandardError'),
           ('PyObject *', 'PyExc_ArithmeticError'),
           ('PyObject *', 'PyExc_LookupError'),
           ('PyObject *', 'PyExc_AssertionError'),
           ('PyObject *', 'PyExc_AttributeError'),
           ('PyObject *', 'PyExc_EOFError'),
           ('PyObject *', 'PyExc_FloatingPointError'),
           ('PyObject *', 'PyExc_EnvironmentError'),
           ('PyObject *', 'PyExc_IOError'),
           ('PyObject *', 'PyExc_OSError'),
           ('PyObject *', 'PyExc_ImportError'),
           ('PyObject *', 'PyExc_IndexError'),
           ('PyObject *', 'PyExc_KeyError'),
           ('PyObject *', 'PyExc_KeyboardInterrupt'),
           ('PyObject *', 'PyExc_MemoryError'),
           ('PyObject *', 'PyExc_NameError'),
           ('PyObject *', 'PyExc_OverflowError'),
           ('PyObject *', 'PyExc_RuntimeError'),
           ('PyObject *', 'PyExc_NotImplementedError'),
           ('PyObject *', 'PyExc_SyntaxError'),
           ('PyObject *', 'PyExc_IndentationError'),
           ('PyObject *', 'PyExc_TabError'),
           ('PyObject *', 'PyExc_ReferenceError'),
           ('PyObject *', 'PyExc_SymbianError', 'SYMBIAN'),
           ('PyObject *', 'PyExc_SystemError'),
           ('PyObject *', 'PyExc_SystemExit'),
           ('PyObject *', 'PyExc_TypeError'),
           ('PyObject *', 'PyExc_UnboundLocalError'),
           ('PyObject *', 'PyExc_UnicodeError'),
           ('PyObject *', 'PyExc_UnicodeEncodeError'),
           ('PyObject *', 'PyExc_UnicodeDecodeError'),
           ('PyObject *', 'PyExc_UnicodeTranslateError'),
           ('PyObject *', 'PyExc_ValueError'),
           ('PyObject *', 'PyExc_ZeroDivisionError'),
           ('PyObject *', 'PyExc_WindowsError', 'MS_WINDOWS'),
           ('PyObject *', 'PyExc_VMSError', '__VMS'),
           ('PyObject *', 'PyExc_MemoryErrorInst'),
           ('PyObject *', 'PyExc_Warning'),
           ('PyObject *', 'PyExc_UserWarning'),
           ('PyObject *', 'PyExc_DeprecationWarning'),
           ('PyObject *', 'PyExc_PendingDeprecationWarning'),
           ('PyObject *', 'PyExc_SyntaxWarning'),
           ('PyObject *', 'PyExc_RuntimeWarning'),
           ('PyObject *', 'PyExc_FutureWarning'),
           ('PyObject *', 'PyExc_ImportWarning'),
           ('PyObject *', 'PyExc_UnicodeWarning'),
           ('int', '_PyOS_opterr'),
           ('int', '_PyOS_optind'),
           ('char *', '_PyOS_optarg'),
           ('PyThreadState *', '_PyThreadState_Current'),
           ('PyThreadFrameGetter', '_PyThreadState_GetFrame'),
           ('t_PyOS_InputHook', 'PyOS_InputHook'),
           ('t_PyOS_ReadlineFunctionPointer', 'PyOS_ReadlineFunctionPointer'),
           ('PyThreadState*', '_PyOS_ReadlineTState'),
           ('PyTypeObject', 'PyRange_Type'),
           ('PyTypeObject', 'PySet_Type'),
           ('PyTypeObject', 'PyFrozenSet_Type'),
           ('PyObject', '_Py_EllipsisObject'),
           ('PyTypeObject', 'PySlice_Type'),
           ('PyTypeObject', 'PyBaseString_Type'),
           ('PyTypeObject', 'PyString_Type'),
           ('PyTypeObject', 'PySTEntry_Type'),
# Commented these three Data, as it seems that they are currently not
# used any where in the Python code.
#          ('PyObject *', '_PySys_TraceFunc'),
#          ('PyObject *', '_PySys_ProfileFunc'),
#          ('int', '_PySys_CheckInterval'),
           ('PyTypeObject', 'PyTraceBack_Type'),
           ('PyTypeObject', 'PyTuple_Type'),
           ('PyTypeObject', 'PyUnicode_Type'),
           ('PyTypeObject', '_PyWeakref_RefType'),
           ('PyTypeObject', '_PyWeakref_ProxyType'),
           ('PyTypeObject', '_PyWeakref_CallableProxyType'),
           ('t__Py_SwappedOp', '_Py_SwappedOp'),
           ('t__PyParser_TokenNames', '_PyParser_TokenNames'),
           ('t__PyLong_DigitValue', '_PyLong_DigitValue')]

include_files = "frameobject.h,pygetopt.h,node.h,"
include_files += "token.h,graminit.h,code.h,compile.h,symtable.h"


licence_text = '''/* Copyright (c) 2008 Nokia Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */\n\n'''

src_file = 'Symbian/src/gendataexports.py'
warning_text = '''/* This file is automatically generated by %s.
 * Do not edit this file manually. */\n\n''' %src_file

start_macro_tags = '''#ifndef DATAEXPORTS_H
#define DATAEXPORTS_H\n
#ifndef Py_BUILD_CORE
#ifdef __cplusplus
extern "C" {
#endif\n\n'''

end_macro_tags = '''
\n#ifdef __cplusplus
}
#endif\n
#endif /* Py_BUILD_CORE */\n
#endif /* DATAEXPORTS_H */\n'''

py_ast_str = '''#include "Python.h"
#define PYTHON_AST_NO_MACRO
#include "Python-ast.h"
#undef PYTHON_AST_NO_MACRO\n'''


def _gen_data_exports():
    c_file = open(DATA_EXPORTS_C_FILE, 'w')
    h_file = open(DATA_EXPORTS_H_FILE, 'w')

    c_file.write(licence_text + warning_text)
    h_file.write(licence_text + warning_text)

    h_file.write(start_macro_tags)

    c_file.write(py_ast_str)
    h_file.write(py_ast_str)

    inc_files = include_files.split(',')
    for file in inc_files:
        c_file.write('#include "%s"\n' %file)
        h_file.write('#include "%s"\n' %file)

    c_file.write('\n')
    h_file.write('\n')

    for item in exports:
        if len(item) == 3:
            (type, name, macro) = item
        else:
            (type, name) = item
            macro = None

        if macro:
            c_file.write('\n#ifdef %s' %macro)
            h_file.write('\n#ifdef %s' %macro)
        c_file.write('\nEXPORT_C %s *\n__DATAEXPORT_%s () { return &%s; }\n'
            %(type, name, name))
        h_file.write('\n#define %s (*__DATAEXPORT_%s())\n' %(name, name))
        h_file.write('\n__declspec(dllimport) %s *__DATAEXPORT_%s ();\n'
                    % (type, name))
        if macro:
            c_file.write('#endif\n')
            h_file.write('#endif\n')

    h_file.write(end_macro_tags)

    c_file.close()
    h_file.close()
