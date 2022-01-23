#include "functions.h"
#include <iostream>

using namespace std;



bool reassemble_teraconvert(V3DPluginCallback& callback, const QString& path,
                            uchar*& pBuffer, V3DLONG sz[4])
{
    if (pBuffer != NULL)
        cout << "WARNING: Pointer to the buffer is not NULL, risk for memory leak." << endl;

    auto dir = QDir(path);
    // JUDGE WHETHER THE FOLDER PATH IS VALID AS A TERACONVERT RES
    if (!dir.exists() && !dir.dirName().contains(QRegExp("^(RES\(\d+x\d+x\d+\))$")))
    {
        cerr << "ERROR: The path of the folder to load teraconvert data doesn't exist or is invalid." << endl;
        return false;
    }

    // INFER IMAGE SIZE FROM FOLDER NAME & ASSIGN MEMORY FOR OUTPUT IMAGE BUFFER
    auto res = dir.dirName().mid(4);
    res.chop(1);
    // here we assume that input images are all 16bit, so we don't do any conversion.
    sz[0] = res.section('x', 1, 1).toLongLong();
    sz[1] = res.section('x', 0, 0).toLongLong();
    sz[2] = res.section('x', 2, 2).toLongLong();
    sz[3] = 1;
    pBuffer = new uchar[ sz[0] * sz[1] * sz[2] * sz[3] * sizeof(v3d_uint16) ];

    /*
     * READ IMAGE FROM ALL SUBFOLDERS
     * Teraconverted images are sorted by y, x, z slicing.
     * The root folder contains all y slicings folder;
     * each y folder slicing contains its x slicing folders;
     * each x slicing folder contains the tif image blocks sorted by their z slicings.
    */

    auto ySlicings = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    uchar* pBlockBuffer = NULL;
    V3DLONG blockSz[4] = {0};

    for (V3DLONG i = 0, yLen = 0;
         i < ySlicings.size();
         ++i, yLen += blockSz[1], dir.cdUp())
    {
        // cd to subfolder for y
        if (!dir.cd(ySlicings.at(i)))
        {
            cout << dir.dirName().toStdString() << endl;
            cerr << "ERROR: Directory " << dir.filePath(ySlicings.at(i)).toStdString()
                 << " doesn't exist.\nMaybe the directory is modified during reading." << endl;
            delete [] pBuffer;
            pBuffer = NULL;
            return false;
        }
        auto xSlicings = dir.entryList(QDir::Dirs| QDir::NoDotAndDotDot);

        for (V3DLONG j = 0, xLen = 0;
             j < xSlicings.size();
             ++j, xLen += blockSz[0], dir.cdUp())
        {
            // cd to subfolder for x
            if (!dir.cd(xSlicings.at(j)))
            {
                cerr << "ERROR: Directory " << dir.filePath(xSlicings.at(i)).toStdString() <<
                        " doesn't exist.\nMaybe the directory is modified during reading." << endl;

                delete [] pBuffer;
                pBuffer = NULL;
                return false;
            }
            auto blocks = dir.entryList(QDir::Files);

            for (V3DLONG k = 0, zLen = 0;
                 k < blocks.size();
                 ++k, zLen += blockSz[2])
            {
                auto imagePath = dir.filePath(blocks.at(k));
                // READ IMAGE USING V3D INTERFACE
                int datatype;
                if (!simple_loadimage_wrapper(callback,
                                              imagePath.toStdString().c_str(),
                                              pBlockBuffer,
                                              blockSz,
                                              datatype))
                {
                    cerr << "ERROR: Failed to load image at " << imagePath.toStdString() << endl;
                    delete [] pBuffer;
                    pBuffer = NULL;
                    return false;
                }

                // COPY THE BLOCK TO THE OUTPUT IMAGE BUFFER
                // can have a conversion here if the data type is not 16bit.
                // we use ii,jj,kk to iterate through z,y,x of the image block.
                // note its different from i,j,k, which iterate through y,x,z of the whole image.
                for (V3DLONG ii = 0; ii < blockSz[2]; ++ii)
                {
                    for (V3DLONG jj = 0; jj < blockSz[1]; ++jj)
                    {
                        // starting point in output buffer
                        auto dst = pBuffer +
                                (sz[0] * sz[1] * (zLen + ii) + sz[0] * (yLen + jj) + xLen)
                                * sizeof(v3d_uint16);

                        // starting point in block buffer
                        auto src = pBlockBuffer +
                                (blockSz[0] * blockSz[1] * ii + blockSz[0] * jj)
                                * sizeof(v3d_uint16);

                        memcpy(dst, src, blockSz[0] * sizeof(v3d_uint16));
                    }
                }
                // test
                auto s = "ttest.tif";
                if (!simple_saveimage_wrapper(callback, s, pBlockBuffer, blockSz, V3D_UINT16))
                    cerr << 'f' << endl;

                delete [] pBlockBuffer;
                pBlockBuffer = NULL;
                // record the x,y,z position to align blocks to
            }
        }
    }


    // SUCCESS
    return true;

}
