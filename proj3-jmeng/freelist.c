/*-------------------------------------------------------------------------
 *
 * freelist.c
 *	  routines for managing the buffer pool's replacement strategy.
 *
 *
 * Portions Copyright (c) 1996-2006, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  $PostgreSQL: pgsql/src/backend/storage/buffer/freelist.c,v 1.57 2006/10/04 00:29:57 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "storage/buf_internals.h"
#include "storage/bufmgr.h"


/*
 * The shared freelist control information.
 */
typedef struct
{
	/* Clock sweep hand: index of next buffer to consider grabbing */
	int			nextVictimBuffer;

	int			firstFreeBuffer;	/* Head of list of unused buffers */
	int			lastFreeBuffer; /* Tail of list of unused buffers */

	/*
	 * NOTE: lastFreeBuffer is undefined when firstFreeBuffer is -1 (that is,
	 * when the list is empty)
	 */

	/*LRU_CHANGE: add lru_head and lru_tail*************/
	int lruHead;
	int lruTail;


} BufferStrategyControl;

/* Pointers to shared state */
static BufferStrategyControl *StrategyControl = NULL;

/* Backend-local state about whether currently vacuuming */
bool		strategy_hint_vacuum = false;

/*LRU_CHANGE: LRU_insert insert to tail************/
void lruInsert(volatile BufferDesc *buf)
{	
	buf->lruPrev = StrategyControl->lruTail;	
	buf->lruNext = LRUNEXT_END_OF_LIST;
	
	//link old tail next to it
	BufferDescriptors[StrategyControl->lruTail].lruNext = buf->buf_id;
	//update lruTail
	StrategyControl->lruTail = buf->buf_id; 
	
	if (buf->lruPrev == LRUPREV_HEAD_OF_LIST) {
		// when list is empty, update lruHead
		StrategyControl->lruHead = buf->buf_id; 
	} else { 
		// else when list is NON-empty
		BufferDescriptors[buf->lruPrev].lruNext = buf->buf_id;  
	}
		elog(LOG, "Add buf %d\n", buf->buf_id);
}  
 
/*LRU_CHANGE: LRU delete delete from list*************/
void lruDelete(volatile BufferDesc *buf)
{
	//check if the buffer is already deleted, if so, do nothing
    if (buf->lruNext == LRU_NOT_IN_LIST) {
        return;
    }
 
    Assert(StrategyControl->lruHead != LRUNEXT_END_OF_LIST);
    // delete current buffer block from next
    if (buf->lruNext == LRUNEXT_END_OF_LIST) {
	//buf is the tail block, update lru tail pointer
	StrategyControl->lruTail = buf->lruPrev;
	}
    else {
	//buf is NOT tail, just skip it
	BufferDescriptors[buf->lruNext].lruPrev = buf->lruPrev;
	}
 
    // delete current buffer block from prev
    if (buf->lruPrev == LRUPREV_HEAD_OF_LIST) {
	//buf is the head of lru list, need to update lruHead after deletion
	StrategyControl->lruHead = buf->lruNext;
	}
        
    else {
	BufferDescriptors[buf->lruPrev].lruNext = buf->lruNext;
	}
 
    buf->lruPrev = LRU_NOT_IN_LIST;
    buf->lruNext = LRU_NOT_IN_LIST;
}

/*
 * StrategyGetBuffer
 *
 *	Called by the bufmgr to get the next candidate buffer to use in
 *	BufferAlloc(). The only hard requirement BufferAlloc() has is that
 *	the selected buffer must not currently be pinned by anyone.
 *
 *	To ensure that no one else can pin the buffer before we do, we must
 *	return the buffer with the buffer header spinlock still held.  That
 *	means that we return with the BufFreelistLock still held, as well;
 *	the caller must release that lock once the spinlock is dropped.
 */
volatile BufferDesc *
StrategyGetBuffer(void)
{
	volatile BufferDesc *buf;
	int			trycounter;

	LWLockAcquire(BufFreelistLock, LW_EXCLUSIVE);

	/*
	 * Try to get a buffer from the freelist.  Note that the freeNext fields
	 * are considered to be protected by the BufFreelistLock not the
	 * individual buffer spinlocks, so it's OK to manipulate them without
	 * holding the spinlock.
	 */
	 #if 0
	while (StrategyControl->firstFreeBuffer >= 0)
	{
		buf = &BufferDescriptors[StrategyControl->firstFreeBuffer];
		Assert(buf->freeNext != FREENEXT_NOT_IN_LIST);

		/* Unconditionally remove buffer from freelist */
		StrategyControl->firstFreeBuffer = buf->freeNext;
		buf->freeNext = FREENEXT_NOT_IN_LIST;

		/*
		 * If the buffer is pinned or has a nonzero usage_count, we cannot use
		 * it; discard it and retry.  (This can only happen if VACUUM put a
		 * valid buffer in the freelist and then someone else used it before
		 * we got to it.)
		 */
		LockBufHdr(buf);
		/*LRU_CHANGE: usage_count is not used in lru algorithm*/
		//if (buf->refcount == 0 && buf->usage_count == 0) 
		if (buf->refcount == 0)
			{
		/*LRU_CHANGE: debug************/
		 elog(LOG, "Get buf %d", buf->buf_id);
			return buf;
		}
		UnlockBufHdr(buf);
	}


	/* Nothing on the freelist, so run the "clock sweep" algorithm */
	trycounter = NBuffers;
	for (;;)
	{	 
		  buf =  &BufferDescriptors[StrategyControl->lruHead];
                // &BufferDescriptors[StrategyControl->nextVictimBuffer];

		if (++StrategyControl->nextVictimBuffer >= NBuffers)
			StrategyControl->nextVictimBuffer = 0;

		/*
		 * If the buffer is pinned or has a nonzero usage_count, we cannot use
		 * it; decrement the usage_count and keep scanning.
		 */
		LockBufHdr(buf);
		if (buf->refcount == 0 && buf->usage_count == 0) {
			elog(LOG, "Found victim is buf %d\n", buf->buf_id);
			return buf;
			}
			
		if (buf->usage_count > 0)
		{
			buf->usage_count--;
			trycounter = NBuffers;
		}
		else if (--trycounter == 0)
		{
			/*LRU_CHANGE: ********************/
			lruDelete(buf);
			/*
			 * We've scanned all the buffers without making any state changes,
			 * so all the buffers are pinned (or were when we looked at them).
			 * We could hope that someone will free one eventually, but it's
			 * probably better to fail than to risk getting stuck in an
			 * infinite loop.
			 */
			UnlockBufHdr(buf);
			elog(ERROR, "no unpinned buffers available");
		}
		UnlockBufHdr(buf);
	}

#endif
/*LRU_CHANGE: lru algorithm**************/
while (StrategyControl->lruHead >= 0)
	{
		buf = &BufferDescriptors[StrategyControl->lruHead];
		Assert(buf->lruNext != LRUNEXT_NOT_IN_LIST);//make sure the lru list is not empty

		/*remove buffer from lrulist head*/
		StrategyControl->lruHead = buf->lruNext;
		buf->lruNext = LRU_NOT_IN_LIST;

		/*
		 * If the buffer is pinned we cannot use
		 * it; delete it from lru list and retry.  (This can only happen if VACUUM put a
		 * valid buffer in the lrulist and then someone else used it before
		 * we got to it.)
		 */
		LockBufHdr(buf);
		/*LRU_CHANGE: usage_count is not used in lru algorithm*/
		//if (buf->refcount == 0 && buf->usage_count == 0) 
		if (buf->refcount == 0)
			{
		/*LRU_CHANGE: debug************/
		 elog(LOG, "Get buf %d\n", buf->buf_id);
			return buf;
		}
		UnlockBufHdr(buf);
	}

/*LRU_CHANGE: lru algorithm**************/
/* Nothing on the freelist, we check on the buffer ring to see if any buffer is just unpinned***/
	trycounter = NBuffers;
	for (;;)
	{	 
		  buf = &BufferDescriptors[StrategyControl->nextVictimBuffer];

		if (++StrategyControl->nextVictimBuffer >= NBuffers)
			StrategyControl->nextVictimBuffer = 0;

		/*
		 * If the buffer is pinned or has a nonzero usage_count, we cannot use
		 * it; decrement the usage_count and keep scanning.
		 */
		LockBufHdr(buf);
		if (buf->refcount == 0) { // if so this is supposed to be add to lru but lru is empty and we 
					// need to add it and then pick it then I just get it here
			elog(LOG, "Get buf %d\n", buf->buf_id);
			return buf;
			}
			
		/*if (buf->usage_count > 0)
		{
			buf->usage_count--;
			trycounter = NBuffers;
		}
		else */
		if (--trycounter == 0)
		{
			/*LRU_CHANGE: ********************/
			lruDelete(buf);
			/*
			 * We've scanned all the buffers without making any state changes,
			 * so all the buffers are pinned (or were when we looked at them).
			 * We could hope that someone will free one eventually, but it's
			 * probably better to fail than to risk getting stuck in an
			 * infinite loop.
			 */
			UnlockBufHdr(buf);
			elog(ERROR, "no unpinned buffers available");
		}
		UnlockBufHdr(buf);
	}
  

	/* not reached */
	return NULL;
}

/*
 * StrategyFreeBuffer: put a buffer on the freelist
 *
 * The buffer is added either at the head or the tail, according to the
 * at_head parameter.  This allows a small amount of control over how
 * quickly the buffer is reused.
 */
void
StrategyFreeBuffer(volatile BufferDesc *buf, bool at_head)
{
	LWLockAcquire(BufFreelistLock, LW_EXCLUSIVE);

	/*
	 * It is possible that we are told to put something in the freelist that
	 * is already in it; don't screw up the list if so.
	 */
	if (buf->freeNext == FREENEXT_NOT_IN_LIST)
	{
		if (at_head)
		{
			buf->freeNext = StrategyControl->firstFreeBuffer;
			if (buf->freeNext < 0)
				StrategyControl->lastFreeBuffer = buf->buf_id;
			StrategyControl->firstFreeBuffer = buf->buf_id;
		}
		else
		{
			buf->freeNext = FREENEXT_END_OF_LIST;
			if (StrategyControl->firstFreeBuffer < 0)
				StrategyControl->firstFreeBuffer = buf->buf_id;
			else
				BufferDescriptors[StrategyControl->lastFreeBuffer].freeNext = buf->buf_id;
			StrategyControl->lastFreeBuffer = buf->buf_id;
		}
	}

	LWLockRelease(BufFreelistLock);
}

/*
 * StrategySyncStart -- tell BufferSync where to start syncing
 *
 * The result is the buffer index of the best buffer to sync first.
 * BufferSync() will proceed circularly around the buffer array from there.
 */
int
StrategySyncStart(void)
{
	int			result;

	/*
	 * We could probably dispense with the locking here, but just to be safe
	 * ...
	 */
	LWLockAcquire(BufFreelistLock, LW_EXCLUSIVE);
	result = StrategyControl->nextVictimBuffer;
	LWLockRelease(BufFreelistLock);
	return result;
}

/*
 * StrategyHintVacuum -- tell us whether VACUUM is active
 */
void
StrategyHintVacuum(bool vacuum_active)
{
	strategy_hint_vacuum = vacuum_active;
}


/*
 * StrategyShmemSize
 *
 * estimate the size of shared memory used by the freelist-related structures.
 *
 * Note: for somewhat historical reasons, the buffer lookup hashtable size
 * is also determined here.
 */
Size
StrategyShmemSize(void)
{
	Size		size = 0;

	/* size of lookup hash table ... see comment in StrategyInitialize */
	size = add_size(size, BufTableShmemSize(NBuffers + NUM_BUFFER_PARTITIONS));

	/* size of the shared replacement strategy control block */
	size = add_size(size, MAXALIGN(sizeof(BufferStrategyControl)));

	return size;
}

/*
 * StrategyInitialize -- initialize the buffer cache replacement
 *		strategy.
 *
 * Assumes: All of the buffers are already built into a linked list.
 *		Only called by postmaster and only during initialization.
 */
void
StrategyInitialize(bool init)
{
	bool		found;

	/*
	 * Initialize the shared buffer lookup hashtable.
	 *
	 * Since we can't tolerate running out of lookup table entries, we must be
	 * sure to specify an adequate table size here.  The maximum steady-state
	 * usage is of course NBuffers entries, but BufferAlloc() tries to insert
	 * a new entry before deleting the old.  In principle this could be
	 * happening in each partition concurrently, so we could need as many as
	 * NBuffers + NUM_BUFFER_PARTITIONS entries.
	 */
	InitBufTable(NBuffers + NUM_BUFFER_PARTITIONS);

	/*
	 * Get or create the shared strategy control block
	 */
	StrategyControl = (BufferStrategyControl *)
		ShmemInitStruct("Buffer Strategy Status",
						sizeof(BufferStrategyControl),
						&found);

	if (!found)
	{
		/*
		 * Only done once, usually in postmaster
		 */
		Assert(init);

		/*
		 * Grab the whole linked list of free buffers for our strategy. We
		 * assume it was previously set up by InitBufferPool().
		 */
		StrategyControl->firstFreeBuffer = 0;
		StrategyControl->lastFreeBuffer = NBuffers - 1;

		/* Initialize the clock sweep pointer */
		StrategyControl->nextVictimBuffer = 0;

		/*LRU_CHANGE: init lru list************/
		StrategyControl->lruHead = LRUNEXT_END_OF_LIST;
		StrategyControl->lruTail = LRUPREV_HEAD_OF_LIST;
	}
	else
		Assert(!init);
}
