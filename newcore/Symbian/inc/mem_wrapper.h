/* Copyright (c) 2009 Nokia Corporation
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
 * ====================================================================
 */

#ifndef MEM_WRAPPER_H
#define MEM_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"
#include <dlfcn.h>

#define LIBC_MALLOC_ORDINAL "221"
#define LIBC_REALLOC_ORDINAL "276"
#define LIBC_FREE_ORDINAL "98"

PyAPI_FUNC (void) track_allocations(bool track_flag);
PyAPI_FUNC (void*) malloc_wrapper(size_t bytes);
PyAPI_FUNC (void*) realloc_wrapper(void *ptr, size_t bytes);
PyAPI_FUNC (void) free_wrapper(void *ptr);
PyAPI_FUNC (void) free_all_allocations();

#ifdef __cplusplus
}
#endif

#endif
