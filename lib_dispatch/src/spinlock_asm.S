// Copyright 2021 XMOS LIMITED. This Software is subject to the terms of the 
// XMOS Public License: Version 1

          .file     "spinlock_metal_asm.S"
          .text
.cc_top spinlock_try_acquire.function
          .align    4
.globl spinlock_try_acquire
.globl spinlock_try_acquire.nstackwords
.globl spinlock_try_acquire.maxthreads
.globl spinlock_try_acquire.maxtimers
.globl spinlock_try_acquire.maxchanends
.type  spinlock_try_acquire, @function
.set spinlock_try_acquire.locnochandec, 1
.set spinlock_try_acquire.nstackwords, 0
spinlock_try_acquire:
    ENTSP_lu6 0
    get r11, id
    add r11, r11, 1
    ldw r1, r0[0]            // Get the current mutex value.
    bt  r1, .Lfailed         // Check if it is already claimed.
    stw r11, r0[0]           // Claim it.
#if !defined(__XS1B__)
    // On XS2 a high priority core needs to ensure that a low priority core
    // has executed the store before we load
    nop
    nop
    nop
    nop
    nop
    nop
#endif
    nop
    ldw r2, r0[0]            // Reload the mutex.
    eq  r0, r11, r2          // If the value hasn't changed we've claimed the
    retsp 0                  // mutex.
.Lfailed:
    ldc r0, 0
    retsp 0
.size spinlock_try_acquire, .-spinlock_try_acquire
.cc_bottom spinlock_try_acquire.function
.set spinlock_try_acquire.maxchanends, 0
.set spinlock_try_acquire.maxtimers, 0
.set spinlock_try_acquire.maxthreads, 1