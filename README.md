# GenDep

This is a fork of http://www.hep.phy.cam.ac.uk/~lester/gendep/index.html. This LD_PRELOAD method was improved but ultimately found lacking. It won't work for straight system calls (that bypass libc) nor for executables that have a statically linked libc. Therefore a second strace method is also developed and likely superiod.

## Usage
For usage see the tests/ folder