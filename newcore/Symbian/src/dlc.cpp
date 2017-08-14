/*
 * ====================================================================
 *  dlc.cpp
 *  
 *  A memory allocator that uses a local disconnected chunk as the underlying
 *  mechanism for requesting memory from and returning memory to the Symbian OS.
 *  The use of a local disconnected chunk makes it possible to free noncontiguous
 *  pieces of memory. Currently only blocks of one size are supported.
 *
 * Copyright (c) 2007-2009 Nokia Corporation
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

#include "Python.h"
#include "dlc.h"

#include <e32std.h>
#include <e32debug.h>

#define DLC_DEBUG 0

#if DLC_DEBUG
#define LOG(x) RDebug::Print x ;
#else
#define LOG(x) 
#endif

/* This is a very simple allocator that can allocate and free just blocks of one size. 
 * DLC_BLOCK_SIZE is defined in dlc.h. */
#define DLC_MAX_BLOCKS ((DLC_MAX_SIZE)/(DLC_BLOCK_SIZE))
#define DLC_BLOCK_OFFSET(i) ((i)*(DLC_BLOCK_SIZE))

/* Shamelessly using WSD for now. */
RChunk *dlc_chunk;
TBool dlc_block_is_free[DLC_MAX_BLOCKS];

extern "C" {
/* Initialize the DLC allocator. Return: 0 on success, -1 on failure. */
int dlc_init()
{
    TInt error=KErrNone;
    int i;
    dlc_chunk=new RChunk;
    if (!dlc_chunk) {
        dlc_chunk=NULL;
        return -1;
    }
    /* Create a disconnected local chunk of maximum size DLC_MAX_SIZE, with no physical memory committed at first. */
    error=dlc_chunk->CreateDisconnectedLocal(0, 0, DLC_MAX_SIZE);
    if (error != KErrNone) {
        delete dlc_chunk;
        dlc_chunk=NULL;
        return -1;
    }
    for (i=0; i<DLC_MAX_BLOCKS; i++)
        dlc_block_is_free[i]=1;
    return 0;
}

void dlc_fini()
{
    if (dlc_chunk != NULL) {    
        dlc_chunk->Close();
        delete dlc_chunk; 
        dlc_chunk=NULL;
    }
}

static void _dlc_log_stats()
{
    int allocated_blocks, free_blocks;
    dlc_stats(&allocated_blocks, &free_blocks);
    LOG((_L("DLC alloced %d free %d"), allocated_blocks, free_blocks));
}

void *dlc_alloc(int size)
{
    int i;

#if DLC_DEBUG    
    LOG((_L("DLC alloc")));
    _dlc_log_stats();
#endif
    
    /* We only accept allocations of DLC_BLOCK_SIZE. */
    if (size != DLC_BLOCK_SIZE)
        return NULL;
    /* Find a free block */
    for (i=0; i<DLC_MAX_BLOCKS; i++)
        if (dlc_block_is_free[i]) {
            TInt error=dlc_chunk->Commit(DLC_BLOCK_OFFSET(i), DLC_BLOCK_SIZE);
            if (error != KErrNone) 
                return NULL;            
            dlc_block_is_free[i]=0;
            LOG((_L("DLC allocated block #%d"),i));
            return dlc_chunk->Base()+DLC_BLOCK_OFFSET(i);
        }
    /* No free blocks - memory exhausted. */
    return NULL;
}

void dlc_free(void *ptr)
{
	int offset=(unsigned char *)ptr-dlc_chunk->Base();
    int block_index=offset/DLC_BLOCK_SIZE;
#if DLC_DEBUG    
    LOG((_L("DLC free block #%d offset %x address %x"),block_index, offset, (int)ptr));
    _dlc_log_stats();
#endif
    if (dlc_block_is_free[block_index]) {
        LOG((_L("DLC PANIC: Block already free")));
        User::Panic(_L("DLCDoubleFree"),1);
    }
    dlc_chunk->Decommit(offset, DLC_BLOCK_SIZE);
    dlc_block_is_free[block_index]=1;
}

void dlc_stats(int *allocated_blocks, int *free_blocks)
{
    int i;
    *allocated_blocks=0;
    *free_blocks=0;
    for (i=0; i<DLC_MAX_BLOCKS; i++) 
        if (dlc_block_is_free[i])
            (*free_blocks)++;
        else
            (*allocated_blocks)++;
}

TBool dlc_is_allocated(void *ptr)
{
    if (ptr)
    {
        int offset = (unsigned char *)ptr - dlc_chunk->Base();
        int block_index = offset/DLC_BLOCK_SIZE;
        if (offset >= 0 && !dlc_block_is_free[block_index])
            return ETrue;	
    }
    return EFalse;
}

}
