// Copyright (c) 2020, XMOS Ltd, All rights reserved
#ifndef DISPATCH_SPINLOCK_H_
#define DISPATCH_SPINLOCK_H_

typedef unsigned swlock_t;

#define SWLOCK_INITIAL_VALUE 0

enum { SWLOCK_NOT_ACQUIRED = 0 };

/** Initialize a software lock.
 *
 *  This function will initialize a software lock for use. Note that unlike
 *  hardware locks, there is no need to allocate or free a software lock from a
 *  limited pool.
 */
void swlock_init(swlock_t *lock);

/** Try and acquire a software lock.
 *
 *  This function tries to acquire a lock for the current logical core.
 *  If another core holds the lock then the function will fail and return.
 *
 *  \param   lock  the software lock to acquire.
 *
 *  \returns a value that is equal to ``SWLOCK_NOT_ACQUIRED`` if
 *           the attempt fails. Any other value indicates that the
 *           acquisition has succeeded.
 */
int swlock_try_acquire(swlock_t *lock);

/** Acquire a software lock.
 *
 *  This function acquires a lock for the current logical core.
 *  If another core holds the lock then the function will wait until
 *  it becomes available.
 *
 *  \param   lock  the software lock to acquire.
 *
 */
void swlock_acquire(swlock_t *lock);

/** Release a software lock.
 *
 *  This function releases a previously acquired software lock for other cores
 *  to use.
 *
 *  \param lock   the software lock to release.
 *
 */
void swlock_release(swlock_t *lock);

#endif  // DISPATCH_SPINLOCK_H_