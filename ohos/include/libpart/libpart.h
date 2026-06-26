/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef OHOS_LIBPART_H
#define OHOS_LIBPART_H

#include <unistd.h>
#include <linux/fs.h>
#include <sys/ioctl.h>

int IoctlRetry(int fd);

#endif // OHOS_LIBPART_H
