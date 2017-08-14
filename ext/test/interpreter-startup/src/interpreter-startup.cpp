/* Copyright (c) 2008 Nokia Corporation
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
#include <e32cmn.h>
#include <e32base.h>
#include <e32cons.h>
#include <Python.h>

// This is a GCCE toolchain workaround needed when compiling with GCCE
// and using main() entry point
#ifdef __GCCE__
#include <staticlibinit_gcce.h>
#endif

int main(void)
{
    TTime iEndTime;
    printf("Python 2.5 script runner application.\n");
    
    SPy_DLC_Init();
    SPy_SetAllocator(SPy_DLC_Alloc, SPy_DLC_Realloc, SPy_DLC_Free, NULL);
    
    Py_Initialize();      
    
    //Record The Time for Py_Initilize completion
    iEndTime.UniversalTime();
    FILE *fp = fopen("\\data\\python\\test\\temp.log", "w+");
    fprintf(fp, "%lld", iEndTime.Int64());
    fclose(fp);
       
    Py_Finalize();
    SPy_DLC_Fini();
    return 0;
}

