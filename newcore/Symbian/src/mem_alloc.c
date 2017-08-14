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

#include "mem_alloc.h"
#include "dlc.h"

static void*(*allocator_alloc)(size_t, void*) = NULL;
static void*(*allocator_realloc)(void*, size_t, void*) = NULL;
static void(*allocator_free)(void*, void*) = NULL;
static void* allocator_context = NULL;

int SPy_DLC_Init()
{
    return dlc_init();
}

void SPy_DLC_Fini()
{
    dlc_fini();
}

void* SPy_DLC_Alloc(size_t nBytes, void* context)
{
    void *ptr = NULL;
    ptr = dlc_alloc(nBytes);
    if (!ptr)
        ptr = malloc(nBytes);
    return ptr;
}

void* SPy_DLC_Realloc(void* ptr, size_t nBytes, void* context)
{
    void *newptr = NULL;
    if (dlc_is_allocated(ptr)) 
    {        
        if ((newptr = malloc(nBytes)))
        {
            memcpy(newptr, ptr, DLC_BLOCK_SIZE);
            dlc_free(ptr);
        }
    }
    else
        newptr = realloc(ptr, nBytes);

    return newptr;
}

void SPy_DLC_Free(void* ptr, void* context)
{     
    if (dlc_is_allocated(ptr))
        dlc_free(ptr);
    else
        free(ptr);
}

void* _PyCore_Malloc(size_t size)
{
    if (allocator_alloc)
        return allocator_alloc(size, allocator_context);

    return malloc(size);
}

void* _PyCore_Realloc(void *p, size_t size)
{
    if (allocator_realloc)
        return allocator_realloc(p, size, allocator_context);

    return realloc(p, size);    
}

void _PyCore_Free(void *ptr)
{
    if (allocator_free)
        allocator_free(ptr, allocator_context);
    else
        free(ptr);
}

void SPy_SetAllocator(void* (*alloc)(size_t, void*), void* (*realloc)(void*, size_t, void*), void (*free)(void*, void*), void *context)
{
    allocator_alloc = alloc;
    allocator_realloc = realloc;
    allocator_free = free;
    allocator_context = context;
}

