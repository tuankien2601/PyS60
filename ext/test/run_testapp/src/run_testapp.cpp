/* Copyright (c) 2008 - 2009 Nokia Corporation
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
#include <dirent.h>
#include <hal.h>
#include <Python.h>

// This is a GCCE toolchain workaround needed when compiling with GCCE
// and using main() entry point
#ifdef __GCCE__
#include <staticlibinit_gcce.h>
#endif

TInt memory_before = 0;
TInt memory_after = 0;

#define MAX_LEN 256
#define EXE "c:\\sys\\bin\\testapp.exe "

void executeTestSets()
{
    struct dirent **testsets;
    int n;
    unsigned long diff_mem, max_mem = 0;
    char testset_name[20];

    n = scandir("\\\\data\\\\python\\\\test\\\\testcases", &testsets, 0, 0);

    if(n > 0)
    {
        // Execute testapp with all the files in testcases dir
        while(n--)
        {
            printf("Running... %s\n",testsets[n]->d_name);
            char command[MAX_LEN];
            strcpy(command, EXE);
            strcat(command, testsets[n]->d_name);
            
            HAL::Get(HAL::EMemoryRAMFree, memory_before);
            system(command);
            HAL::Get(HAL::EMemoryRAMFree, memory_after);
            
            diff_mem = memory_before - memory_after;
            if (diff_mem > max_mem)
            {
                max_mem = diff_mem;
                strcpy(testset_name, testsets[n]->d_name);
            }
        }
    }
    FILE *p=fopen("\\\\data\\\\python\\\\test\\\\regrtest_emu.log", "a");
    fprintf(p, "\nMeasurement -- Name : <Max RAM was consumed by %s>, Value :\
 <%u>, Unit : <Kb>, Threshold : <100>, higher_is_better : <no>\n", \
testset_name, diff_mem/1024);
    fclose(p);
}

double timeval_subtract(struct timeval * x, struct timeval * y)
{
    double elapsed_seconds = x->tv_sec - y->tv_sec;
    double elapsed_useconds = x->tv_usec - y->tv_usec;
    return elapsed_seconds + elapsed_useconds/1000000;
}
 
int main(int argc, char **argv)
{
    printf("Testapp runner application.\n");
    SPy_DLC_Init();
    SPy_SetAllocator(SPy_DLC_Alloc, SPy_DLC_Realloc, SPy_DLC_Free, NULL);
    struct timeval time1, time2;
    double result, iptr;
    char diff_time[20];
    
    HAL::Get(HAL::EMemoryRAMFree, memory_before);
    gettimeofday(&time1, NULL);
    Py_Initialize();
    gettimeofday(&time2, NULL);
    HAL::Get(HAL::EMemoryRAMFree, memory_after);
    result = timeval_subtract(&time2, &time1);
    
    // The result is split into integral and fractional part
    // because OpenC has known issues with %f in printf
    double fractional = modf(result, &iptr);
    int integral = iptr;
    char tempfract[20], fraction[20];
    sprintf(tempfract, "%f", fractional);
    // +2 done to remove the "0." from the fractional part
    // A length of 7 specified to get the accuracy in microseconds,
    // 6 characters and the '\0'
    strncpy(fraction, tempfract+2, 7);
    sprintf(diff_time, "%d.%s", integral, fraction);
    
    double diff_mem = memory_before - memory_after;
    FILE *p = fopen("\\\\data\\\\python\\\\test\\\\regrtest_emu.log", "a");
    fprintf(p, "Measurement -- Name : <Py_Initialize time>, Value : <%s>, \
Unit : <seconds>, Threshold : <0.1>, higher_is_better : <no>  \n", diff_time);
    fprintf(p, "\nMeasurement -- Name : <Py_Initialize RAM>, Value : <%f>, \
Unit : <Mb>, Threshold : <1>, higher_is_better : <no> \n", diff_mem/(1024 * 1024));
    fclose(p);
    
    PyRun_SimpleString("execfile('\\\\data\\\\python\\\\test\\\\create_testsets.py')\n");
    Py_Finalize();
    SPy_DLC_Fini();
    printf("Created test sets\n");

    executeTestSets();
    return 0;
}
