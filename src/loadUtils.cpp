/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#include "loadUtils.h"
#include <iostream>

using namespace std;

/*
 * Reassemble teraconvert brain image blocks from their directory
 *
 * If the input image is in teraconverted format, it's in multiple blocks.
 * you can refer to terafly's nature methods paper and its supp materials
 * for its details. Here we need to reassemble them together, since it's in
 * the lowest resolution, it can be easily handled in whole.
 *
 * Params:
 *
 * callback: Vaa3D callback used to load images
 *
 * path: path to the directory storing brain blocks of a resolution (16bit)
 *
 * pBuffer: null pointer to save the loaded image
 *
 * sz: 4D V3DLONG array to store image size
 *
 */

bool load_teraconvert_dir(const QString& path, QcImage& output, const Loader& loader, int datatype)
{
    QcImage block;

    try
    {
        // 1. JUDGE WHETHER THE FOLDER PATH IS VALID AS A TERACONVERT RES
        auto dir = QDir(path);
        if (!dir.exists() && !dir.dirName().contains(QRegExp("^(RES\(\d+x\d+x\d+\))$")))
            throw invalid_argument("The path of the folder to load the "
                                   "teraconvert data doesn't exist or is invalid.");

        // 2. INFER IMAGE SIZE FROM FOLDER NAME & ASSIGN MEMORY FOR OUTPUT IMAGE BUFFER
        auto res = dir.dirName().mid(4);
        res.chop(1);
        // here we assume that input images are all 16bit, so we don't do any conversion.
        V3DLONG sz[4] = {
            res.section('x', 1, 1).toLongLong(),
            res.section('x', 0, 0).toLongLong(),
            res.section('x', 2, 2).toLongLong(),
            1
        };
        output.clear();
        output.create(sz, datatype);

        /*
         * 3. READ IMAGE FROM ALL SUBFOLDERS
         * Teraconverted images are sorted by y, x, z slicing.
         * The root folder contains all y slicings folder;
         * each y folder slicing contains its x slicing folders;
         * each x slicing folder contains the tif image blocks sorted by their z slicings.
        */

        auto ySlicings = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

        for (V3DLONG i = 0, yLen = 0;
             i < ySlicings.size();
             ++i, yLen += block.sz[1], dir.cdUp())
        {
            // cd to subfolder for y
            if (!dir.cd(ySlicings.at(i)))
                throw runtime_error("Directory " + dir.filePath(ySlicings.at(i)).toStdString() +
                      " doesn't exist.\nMaybe the directory is modified during reading.");

            auto xSlicings = dir.entryList(QDir::Dirs| QDir::NoDotAndDotDot);

            for (V3DLONG j = 0, xLen = 0;
                 j < xSlicings.size();
                 ++j, xLen += block.sz[0], dir.cdUp())
            {
                // cd to subfolder for x
                if (!dir.cd(xSlicings.at(j)))
                    throw runtime_error("Directory " + dir.filePath(xSlicings.at(i)).toStdString() +
                            " doesn't exist.\nMaybe the directory is modified during reading.");

                auto blocks = dir.entryList(QDir::Files);

                for (V3DLONG k = 0, zLen = 0;
                     k < blocks.size();
                     ++k, zLen += block.sz[2])
                {
                    auto imagePath = dir.filePath(blocks.at(k));
                    // READ IMAGE USING V3D INTERFACE
                    if (!loader(imagePath.toStdString().c_str(), block))
                        throw runtime_error("Failed to load image at " + imagePath.toStdString());
                    if (block.datatype != datatype)
                        throw runtime_error("Found inconsistent image pixel type in teraconvert data.");

                    // COPY THE BLOCK TO THE OUTPUT IMAGE BUFFER
                    // can have a conversion here if the data type is not 16bit.
                    // we use ii,jj,kk to iterate through z,y,x of the image block.
                    // note its different from i,j,k, which iterate through y,x,z of the whole image.
                    for (V3DLONG ii = 0; ii < block.sz[2]; ++ii)
                    {
                        for (V3DLONG jj = 0; jj < block.sz[1]; ++jj)
                        {
                            // starting point in output buffer
                            auto dst = output.buffer + (output.sz[0] * output.sz[1] *
                                    (zLen + ii) + output.sz[0] * (yLen + jj) + xLen) * sizeof(v3d_uint16);

                            // starting point in block buffer
                            auto src = block.buffer +
                                    (block.sz[0] * block.sz[1] * ii + block.sz[0] * jj)
                                    * sizeof(v3d_uint16);

                            memcpy(dst, src, block.sz[0] * sizeof(v3d_uint16));
                        }
                    }
                    delete [] block.buffer;
                    block.buffer = NULL;
                }
            }
        }
        return true;
    }
    catch (exception& e)
    {
        cerr << "ERROR: " << e.what() << endl;
        block.clear();
        output.clear();
        return false;
    }
}
