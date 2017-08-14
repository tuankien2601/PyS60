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

#include <hal.h>
#include <stdio.h>
#include <sys/time.h>
#include <python.h>

TInt memory_before, memory_after;
extern "C"
void end_time(int lineno, char *p)
{
    FILE *fp;
    char *file_name = p;
    long result;
    struct timeval next_time;
    unsigned elapsed_seconds, elapsed_useconds;
    /* Get timestamp for the statement */
    gettimeofday(&next_time, NULL);
    HAL::Get(HAL::EMemoryRAMFree, memory_after);
    result = memory_before - memory_after;
    elapsed_seconds = ((&next_time) -> tv_sec);
    elapsed_useconds = ((&next_time)-> tv_usec);
    fp = fopen("\\data\\python\\test\\log_file.log", "a+");
    fprintf(fp, "%s:%d:%u.%06u:%ld\n", file_name, (lineno - 1), elapsed_seconds, elapsed_useconds, result);
    fclose(fp);
    /* The free RAM value at this point will be the RAM value to begin with
       for the next statement profiled */
    memory_before = memory_after;
}