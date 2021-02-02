// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1
#ifndef DISPATCH_SPINLOCK_METAL_H_
#define DISPATCH_SPINLOCK_METAL_H_

typedef unsigned spinlock_t;

/** Create a software lock.
 *
 *  This function will create a software lock for use. Note that unlike
 *  hardware locks, there is no need to allocate or free a software lock from a
 *  limited pool.
 */
spinlock_t *spinlock_create();

/** Try and acquire a software lock.
 *
 *  This function tries to acquire a lock for the current logical core.
 *  If another core holds the lock then the function will fail and return.
 *
 *  \param   lock  the software lock to acquire.
 *
 *  \returns a value that is equal to ``SPINLOCK_NOT_ACQUIRED`` if
 *           the attempt fails. Any other value indicates that the
 *           acquisition has succeeded.
 */
int spinlock_try_acquire(spinlock_t *lock);

/** Acquire a software lock.
 *
 *  This function acquires a lock for the current logical core.
 *  If another core holds the lock then the function will wait until
 *  it becomes available.
 *
 *  \param   lock  the software lock to acquire.
 *
 */
void spinlock_acquire(spinlock_t *lock);

/** Release a software lock.
 *
 *  This function releases a previously acquired software lock for other cores
 *  to use.
 *
 *  \param lock   the software lock to release.
 *
 */
void spinlock_release(spinlock_t *lock);

/** Delete a software lock.
 *
 *  This function delete the memory associated with the software lock
 *
 *  \param lock   the software lock to delete.
 *
 */
void spinlock_delete(spinlock_t *lock);

#endif  // DISPATCH_SPINLOCK_METAL_H_