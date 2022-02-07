/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#ifndef PREPROCESSING_H
#define PREPROCESSING_H

#include <v3d_interface.h>
#include "TeraQCTypes.h"

bool findMarkers(const QcImage& input, QcImage& output, const QVariantMap& params);
bool masking(const QcImage& input, QcImage& output, const QcImage& mask);

#endif // PREPROCESSING_H
