/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#ifndef FINDMARKERS_H
#define FINDMARKERS_H

#include <v3d_interface.h>
#include "TeraQCTypes.h"

bool findMarkers(const QcImage& input, QcImage& output, const QVariantMap& params);

#endif // FINDMARKERS_H
