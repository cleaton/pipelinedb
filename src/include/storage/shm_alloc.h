/*-------------------------------------------------------------------------
 *
 * shm_alloc.h
 *	  shared memory allocator
 *
 * Copyright (c) 2013-2015, PipelineDB
 *
 * src/include/storage/shm_alloc.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef SHM_ALLOC_H
#define SHM_ALLOC_H

extern void ShmemDynAllocShmemInit(void);

extern void *ShmemDynAlloc(Size size);
extern void *ShmemDynAlloc0(Size size);
extern void ShmemDynFree(void *addr);
extern bool ShmemDynAddrIsValid(void *);
extern Size ShmemDynAllocSize(void);

#endif   /* SHM_ALLOC_H */
