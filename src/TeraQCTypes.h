/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#ifndef TERAQCTYPES_H
#define TERAQCTYPES_H

#include <v3d_interface.h>

struct QcImage
{
    // initialization
    QcImage():
        buffer(NULL), datatype(V3D_UNKNOWN)
    {
        for(int i = 0; i < 4; ++i) sz[i] = 0;
    }
    ~QcImage()
    {
        clear();
    }
    void create(const V3DLONG sz[4], int datatype)
    {
        for(int i = 0; i < 4; ++i) this->sz[i] = sz[i];
        this->datatype = datatype;
        int t;
        switch (datatype)
        {
        case V3D_UINT8:
            t = 1;
            break;
        case V3D_UINT16:
            t = 2;
            break;
        case V3D_FLOAT32:
            t = 4;
            break;
        default:
            t = 1;
        };
        buffer = new uchar[ sz[0] * sz[1] * sz[2] * sz[3] * t ];
    }
    void clear() // shallow, doesn't deal with memory dealloc
    {
        if (buffer != NULL)
        {
            delete [] buffer;
            buffer = NULL;
        }
        for(int i = 0; i < 4; ++i) sz[i] = 0;sizeof(QcImage);
        datatype = V3D_UNKNOWN;
    }
    // pointer to image
    uchar* buffer;
    // dimensions
    V3DLONG sz[4];
    // pixel type
    int datatype;
};

#endif // TERAQCTYPES_H
