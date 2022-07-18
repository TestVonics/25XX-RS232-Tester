# 25XX-RS232-Tester - Entry Level Software Engineer Assessment

This take home assignment is meant to verify basic C, Unix, and other development knowledge. It is expected to take 1-3 hours, if it is taking longer you may not be a good fit for this position. Your submission will only be used for the purpose of your interview with TestVonics/Raptor Scientific.

25XX-RS232-Tester is an internal test application at TestVonics for verifying calibrator performance, this branch is a stripped down version without the serial communication code for the purpose of interview assessment.

## Requirements

A linux or unix environment with gcc or gcc-like. It was tested in Debian and Cygwin, but WSL or Mac OS X should be doable too.

## Setup
Clone this repo to your computer

Checkout the assessment branch.

Compile with `make debug`, it should make a `25XXTester` executable in the `bin` directory. If it fails there is some issue with your build environment. Verify the executable runs. `make clean` should delete the executable and the build files.

## Task #1

A build with just `make` fails. Fix the issue with the build and commit the result. Your first commit should include "Task #1" in the commit message.

## Task #2

25XX-RS232-Tester only has a basic console input interface. Write a new interface using a library and/or programming language of your choosing and commit the result. Your last commit should have "Task #2" in the commit message.

This task is an open ended problem to demonstrate basic C knowledge and the ability to extend or develop based on existing software. Some example technologies that could be used to accomplish the task include: GNU Readline, ncurses, Qt, GTK, shared library, FFI, fork, and exec. Be sure to add build instructions to the README, so I can reproduce on my machine. You may modify anything in the repo as needed.

## Submission

`make clean` and create a tarball of the repo (including the `.git` folder): `make clean && cd .. && tar -cvz -f YourName.tar.gz 25XX-RS232-Tester`, DO NOT tar binaries, so your submission isn't picked up as virus by various filters.

Email it to `hayesgav at gmail dawt com` either as link (Google Drive, Dropbox, etc) or attachment. Then, email me WITHOUT the submission to `gavin.hayes at testvonics dawt com` letting me know you submitted. I will reply back to confirm when I have received your submission. Sorry for the complexity, we are in the process of migrating email providers.

## Copyright 2022 Raptor Scientific

Licensed as 3-clause BSD, see LICENSE.