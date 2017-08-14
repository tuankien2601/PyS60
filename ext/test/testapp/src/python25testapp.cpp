/* Copyright (c) 2008-2009 Nokia Corporation
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <e32base.h>
#include <coemain.h>
#include <Python.h>

// This is a GCCE toolchain workaround needed when compiling with GCCE
// and using main() entry point
#ifdef __GCCE__
#include <staticlibinit_gcce.h>
#endif

void set_platform()
{
    FILE *fp = fopen("c:\\data\\python\\test\\platform.txt", "w");
#ifdef __WINS__
    char platform_name[] = "emu";
#else
    char platform_name[] = "device";
#endif
    fprintf(fp, "%s", platform_name);
    fclose(fp);
}

void executeTest(char *testset)
{
    char py_str[256];
    char *runtests_py = "\\\\data\\\\python\\\\test\\\\runtests_sets.py";
    char *except_str = "import traceback\n traceback.print_exc()\n raw_input()";

    sprintf(py_str, "try:\n execfile('%s', {'test_set': '%s'})\nexcept:\n %s\n",
                     runtests_py, testset, except_str);

    Py_Initialize();
    PyRun_SimpleString("import sys\n");
    PyRun_SimpleString("sys.path.append('c:\\\\resource\\\\python25\\\\python25_repo.zip')\n");
    TRAPD(err, PyRun_SimpleString(py_str));
    Py_Finalize();
}

int main(int argc, char **argv)
{
    int c;
    printf("Python 2.5 test application.\n");
    set_platform();

    // Creating CONE environment as it is needed by graphics module
    CCoeEnv* environment = new (ELeave) CCoeEnv();
    TRAPD(err, environment->ConstructL(ETrue, 0));
    delete CActiveScheduler::Current();

    // Installing Active Scheduler is needed when a script imports
    // scriptext module.
    CActiveScheduler* actScheduler = new (ELeave) CActiveScheduler();
    CActiveScheduler::Install( actScheduler );

    SPy_DLC_Init();
    SPy_SetAllocator(SPy_DLC_Alloc, SPy_DLC_Realloc, SPy_DLC_Free, NULL);

    if(argc >= 2)
    {
        /* testapp called with arguments.
         * Execute these tests and return */
        for(int i=1; i<argc; ++i)
        {
            executeTest(argv[i]);
        }
        SPy_DLC_Fini();
        if(environment)
            environment->DestroyEnvironment();
        return 0;
    }

    Py_Initialize();
    PyRun_SimpleString("import sys\n");
    PyRun_SimpleString("sys.path.append('c:\\\\resource\\\\python25\\\\python25_repo.zip')\n");
    /* Find out the mode in which testapp was called and create test_sets if
     * --use-testsets-cfg was passed */
    PyRun_SimpleString("execfile('\\\\data\\\\python\\\\test\\\\create_testsets.py')\n");
    Py_Finalize();
    bool _execute_test_in_suites = FALSE;
    FILE* fp;
    while(1)
    {
        bool file_exists = FALSE;
        /* File "is_testcase_dir_empty.log" is created by create_testsets.py if
         * the testapp is run with --use-testsets-cfg and deleted by runtests_sets.py
         * when all the test-sets are executed. */
        if(fp=fopen("\\\\data\\\\python\\\\test\\\\is_testcase_dir_empty.log","r"))
        {
            file_exists = TRUE;
            fclose(fp);
        }
        if(file_exists)
        {
            /* There are test_sets to be executed, exec runtests_sets.py so that
             * regrtest.py is called with -f option. */
            Py_Initialize();
            PyRun_SimpleString("import sys\n");
            PyRun_SimpleString("sys.path.append('c:\\\\resource\\\\python25\\\\python25_repo.zip')\n");
            TRAPD(err, PyRun_SimpleString("try:\n execfile('\\\\data\\\\python\\\\test\\\\runtests_sets.py')\nexcept:\n import traceback\n traceback.print_exc()\n raw_input()\n"));
            Py_Finalize();
            _execute_test_in_suites = TRUE;
        }
        else
        {
            /* No more test_sets to be executed */
            break;
        }
    }

    if(!_execute_test_in_suites)
    {
        /* testapp was not called with --use-testsets-cfg, call regrtest.py with the
         * options passed to testapp. */
        Py_Initialize();
        PyRun_SimpleString("import sys\n");
        PyRun_SimpleString("sys.path.append('c:\\\\resource\\\\python25\\\\python25_repo.zip')\n");
        TRAPD(err, PyRun_SimpleString("try:\n execfile('\\\\data\\\\python\\\\test\\\\runtests.py')\nexcept:\n import traceback\n traceback.print_exc()\n raw_input()\n"));
        Py_Finalize();

    }
    SPy_DLC_Fini();
    if(environment)
            environment->DestroyEnvironment();
    return 0;
}

