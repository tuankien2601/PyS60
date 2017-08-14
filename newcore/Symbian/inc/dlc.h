/*
 * ====================================================================
 *  dlc.h
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

#ifndef DLC_H
#define DLC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <e32def.h>

#ifdef __WINS__
#define DLC_MAX_SIZE (1024*1024*16)
#else
#define DLC_MAX_SIZE (1024*1024*64)
#endif

int dlc_init();
void dlc_fini();  
void *dlc_alloc(int size);
void dlc_free(void *ptr);
void dlc_stats(int *allocated_blocks, int *free_blocks);
TBool dlc_is_allocated(void *ptr);

#define DLC_BLOCK_SIZE ARENA_SIZE

#ifdef __cplusplus
}
#endif

#endif
