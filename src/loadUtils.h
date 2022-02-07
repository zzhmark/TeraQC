/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#ifndef LOADUTILS_H
#define LOADUTILS_H

#include <v3d_interface.h>
#include <functional>
#include "TeraQCTypes.h"

// loader type define (for convenient loading image with any callback)
typedef std::function<bool(const char*, QcImage&)> Loader;

bool load_teraconvert_dir(const QString& path, QcImage& img, const Loader& loader, int datatype);

#endif // LOADUTILS_H
