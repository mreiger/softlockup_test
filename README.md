## Kernel module to introduce soft lock-up/panic for testing

The idea is to try to understand and if possible simulate how the *softlockup*
condition happens in the kernel.

Basic there is a busy wait loop that runs for a period of time (BUSY_LOOP) and a
main loop (MAIN_LOOP) that loops this busy wait loop.

The default initial parameters are 5 seconds for both. Those parameters can be
changed by writing the values to the following proc filesystem files :

* /proc/softlockup_test/softlockup_test_busy
* /proc/softlockup_test/softlockup_test_main

**WARING:** This module can and probably will freeze your system, so be careful.
