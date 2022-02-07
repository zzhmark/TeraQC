/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#ifndef FINDLOCALMAXIMA_H
#define FINDLOCALMAXIMA_H

#include <v3d_interface.h>
#include "TeraQCTypes.h"

bool findLocalMaxima(const QcImage& input, QcImage& output, const QVariantMap& params);

#endif // FINDLOCALMAXIMA_H
