/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "libpart/libpart.h"

#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

int IoctlRetry(int fd) {
   int ret = -1;
   int retry_count = 0;
   const int max_retries = 6;
   const int retry_delay_us = 50000;

   while (retry_count < max_retries) {
      ret = ioctl(fd, BLKRRPART);
      if (ret == 0) {
         break;
      }
      retry_count++;
      usleep(retry_delay_us * retry_count);
   }

   return ret;
}
