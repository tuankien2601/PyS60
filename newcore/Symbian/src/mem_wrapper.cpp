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

#include "mem_wrapper.h"

extern "C" {

/* This linked list will contain all the allocations made by the interpreter
 * which are not yet freed */
struct alloc_list_node {
  void *ptr;
  struct alloc_list_node *prev;
};

// Always points to the tail of the alloc list node
struct alloc_list_node *alloc_chain_tail_ptr = NULL;
bool track_alloc = true;
void *libc_handle = NULL;

void track_allocations(bool track_flag)
{
    track_alloc = track_flag;
}

void new_alloc_list_node(void *ptr)
{
    typedef void* (*MALLOC)(size_t);
    MALLOC malloc = (MALLOC) dlsym(libc_handle, LIBC_MALLOC_ORDINAL);
    struct alloc_list_node *new_node;
    new_node = (struct alloc_list_node *) malloc (sizeof(struct alloc_list_node));
    new_node->prev = alloc_chain_tail_ptr;
    new_node->ptr = ptr;
    alloc_chain_tail_ptr = new_node;
}

void check_and_load_libc()
{
    if (libc_handle == NULL)
        libc_handle = dlopen("libc.dll", RTLD_NOW | RTLD_LOCAL);
}

void *malloc_wrapper(size_t bytes)
{
    check_and_load_libc();
    typedef void* (*MALLOC)(size_t);
    MALLOC malloc = (MALLOC) dlsym(libc_handle, LIBC_MALLOC_ORDINAL);
    void *ptr = (*malloc)(bytes);

    if (track_alloc)
        new_alloc_list_node(ptr);

    return ptr;
}


void *realloc_wrapper(void *ptr, size_t bytes)
{
    check_and_load_libc();
    typedef void* (*REALLOC)(void*, size_t);
    REALLOC realloc = (REALLOC) dlsym(libc_handle, LIBC_REALLOC_ORDINAL);
    void *realloc_ptr = (*realloc)(ptr, bytes);

    if (!track_alloc)
        return realloc_ptr;

    struct alloc_list_node *temp;
    for(temp = alloc_chain_tail_ptr; ptr!= NULL && temp != NULL; temp = temp->prev)
    {
        if (temp->ptr == ptr)
        {
            temp->ptr = realloc_ptr;
            break;
        }
    }
    // If we didn't find it or if ptr was NULL in the first place,
    // create a new node at the end of the chain
    if (temp == NULL || ptr == NULL)
            new_alloc_list_node(realloc_ptr);

    return realloc_ptr;
}

void free_wrapper(void *ptr)
{
    check_and_load_libc();
    typedef void* (*FREE)(void*);
    FREE free = (FREE) dlsym(libc_handle, LIBC_FREE_ORDINAL);
    (*free)(ptr);

    if (!track_alloc)
        return;

    struct alloc_list_node *temp, *temp_next;
    for(temp = alloc_chain_tail_ptr; temp != NULL; temp_next = temp, temp = temp->prev)
    {
        if (temp->ptr == ptr)
        {
            if (alloc_chain_tail_ptr == temp)
                alloc_chain_tail_ptr = temp->prev;
            else
                temp_next->prev = temp->prev;
            free(temp);
            break;
        }
    }
}

/* This method is called by Py_Finalize to free all non-freed allocations */
void free_all_allocations()
{
    if (track_alloc)
    {
        while(alloc_chain_tail_ptr != NULL)
        {
            struct alloc_list_node *to_be_freed = alloc_chain_tail_ptr;
            alloc_chain_tail_ptr = alloc_chain_tail_ptr->prev;
            free(to_be_freed->ptr);
            free(to_be_freed);
        }
    }    
    dlclose(libc_handle);
    libc_handle = NULL;
}
}
