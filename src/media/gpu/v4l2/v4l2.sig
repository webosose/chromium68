// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//------------------------------------------------
// Functions from libv4l2 used in chromium code.
//------------------------------------------------
LIBV4L_PUBLIC int v4l2_close(int fd);
LIBV4L_PUBLIC int v4l2_ioctl(int fd, unsigned long int request, ...);
LIBV4L_PUBLIC int v4l2_fd_open(int fd, int v4l2_flags);
LIBV4L_PUBLIC void *v4l2_mmap(void *start, size_t length, int prot, int flags, int fd, int64_t offset);
LIBV4L_PUBLIC int v4l2_munmap(void *_start, size_t length);
