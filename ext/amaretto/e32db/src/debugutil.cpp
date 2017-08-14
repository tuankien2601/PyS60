/* Copyright (c) 2005 Nokia Corporation
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

#include "debugutil.h"

#include "Python.h"

void
pyprintf(const char *format, ...)
{
  va_list args;
  va_start(args,format);
  char buf[128];
  // Unfortunately Symbian doesn't seem to support the vsnprintf
  // function, which would allow us to specify the length of the
  // buffer we are writing to. As a result using this function is
  // unsafe and will lead to a buffer overflow if given an argument
  // that is too big. Do not use this function in production code.
  vsprintf(buf, format, args);    
  char buf2[128];
  sprintf(buf2, "print '%s'", buf);
  PyRun_SimpleString(buf2);
}
