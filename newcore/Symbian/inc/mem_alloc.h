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

#ifndef MEM_ALLOC_H
#define MEM_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"

PyAPI_FUNC(void*) _PyCore_Malloc(size_t size);
PyAPI_FUNC(void*) _PyCore_Realloc(void *p, size_t size);
PyAPI_FUNC(void) _PyCore_Free(void *ptr);
PyAPI_FUNC(void) SPy_SetAllocator(void* (*alloc)(size_t, void*), void* (*realloc)(void*, size_t, void*), void (*free)(void*, void*), void *context);
PyAPI_FUNC(int) SPy_DLC_Init();
PyAPI_FUNC(void) SPy_DLC_Fini();
PyAPI_FUNC(void*) SPy_DLC_Alloc(size_t nBytes, void *context);
PyAPI_FUNC(void*) SPy_DLC_Realloc(void* ptr, size_t nBytes, void *context);
PyAPI_FUNC(void) SPy_DLC_Free(void* ptr, void *context);

#ifdef __cplusplus
}
#endif

#endif /* MEM_ALLOC_H */
