/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#include "TeraQCPlugin.h"
#include "loadUtils.h"
#include "preprocessing.h"
#include "roiSampling.h"
#include <iostream>

Q_EXPORT_PLUGIN2(TeraQC, TeraQCPlugin);

using namespace std;

QStringList TeraQCPlugin::menulist() const
{
    return QStringList();
}

QStringList TeraQCPlugin::funclist() const
{
    return QStringList()
            << tr("preprocess")
            << tr("findLocalMaxima")
            << tr("one-pot")
            << tr("help");
}

void TeraQCPlugin::domenu(const QString& menu_name,
                          V3DPluginCallback2& callback,
                          QWidget* parent)
{
    return;
}

bool TeraQCPlugin::dofunc(const QString& func_name,
                          const V3DPluginArgList& input,
                          V3DPluginArgList& output,
                          V3DPluginCallback2& callback,
                          QWidget* parent)
{
    auto loader = [&callback](const char* path, QcImage& output) {
        return simple_loadimage_wrapper(callback, path, output.buffer, output.sz, output.datatype);
    };
    vector<char*>* inlist = NULL;
    vector<char*>* outlist = NULL;
    vector<char*>* arglist = NULL;
    QVariantMap params;
    inlist = (vector<char*>*)(input.at(0).p);
    outlist = (vector<char*>*)(output.at(0).p);

    // procedures
    auto LOAD_IMAGE = [&]() {
        auto info = QFileInfo(inlist->at(0));
        if (info.isFile())
        {
            cout << "\tLoading image " << inlist->at(0) << ".." << endl;
            if(!loader(inlist->at(0), imgInput))
                throw runtime_error("Loading failed.");
            return info.baseName();
        }
        else if (info.isDir())
        {
            cout << "\tLoading teraconverted images in " << inlist->at(0) << ".." << endl;
            if (!loadTeraconvert(inlist->at(0), imgInput,
                                      loader, params.value("datatype", V3D_UINT16).toInt()))
                throw runtime_error("Loading failed.");
            return QDir(inlist->at(0)).dirName();
        } else
            throw runtime_error("Illegal loading path. Neither an image nor teraconvert data.");
    };

    auto FIND_MARKERS = [&]() {
        cout << "\tFinding markers.." << endl;
        if(!findMarkers(imgInput, imgMarker, params))
            throw runtime_error("Finding markers failed.");
    };

    auto SAVE_IMAGE = [&](QcImage& img, const QString& path) {
        cout << "\tSaving the mask to path " << path.toStdString() << endl;
        if (!simple_saveimage_wrapper(callback, path.toStdString().c_str(),
                                      img.buffer, img.sz, img.datatype))
            throw runtime_error("Saving failed");
    };

    auto APPLY_MARKERS = [&](bool invert=true) {
        cout << "\tRemove markers from the input image.." << endl;
        if(!masking(imgInput, imgMasked, imgMarker, invert))
            throw runtime_error("Removing markers failed.");
    };

    auto FIND_LOCAL_MAXIMA = [&](const QcImage& img) {
        findLocalMaxima(img, imgMaxima, params);
    };

    // arguments
    if (input.size() > 1)
    {
        arglist = (vector<char*>*)(input.at(1).p);
        for (int i = 0; i < arglist->size(); i+=2)
            params[arglist->at(i)] = arglist->at(i + 1);
    }

    // commands
    try
    {
        if (func_name == tr("preprocess"))
        {
            cout << "[TeraQC Plugin: Preprocessing]" << endl;
            auto mode = params.value("mode", "default").toString();
            /* mode
             * default: output the mask & image wo mask
             * onlyMarker: only the 8bit marker mask
             * onlyRemove: only the image wo markers
             * validation: same as default but also output 8bit xy projection to see the effect
            */
            auto prefix = outlist->at(0) + LOAD_IMAGE();
            FIND_MARKERS();
            if (mode != "onlyRemove")
                SAVE_IMAGE(imgMarker, prefix + '_mask.tif');
            if (mode != "onlyMarker")
            {
                if (mode == "validation")
                {
                    QcImage proj;
                    APPLY_MARKERS(false);
                    if (!maxProjection8bit(imgMasked, proj))
                        throw "Something wrong with projection.";
                    SAVE_IMAGE(proj, prefix + "_marker_2d.tif");
                    proj.clear();
                    APPLY_MARKERS();
                    if (!maxProjection8bit(imgMasked, proj))
                        throw "Something wrong with projection.";
                    SAVE_IMAGE(proj, prefix + "_removed_2d.tif");
                }
                else
                    APPLY_MARKERS();
                SAVE_IMAGE(imgMasked, prefix + "_removed.tif");
            }
            cout << "Done." << endl;
        }
        else if (func_name == tr("findLocalMaxima"))
        {
            cout << "[TeraQC Plugin: Find Local Maxima]" << endl;
            auto prefix = outlist->at(0) + LOAD_IMAGE();
            QcImage* pImg;
            if (params.value("preprocessing", "y").toString().toLower().startsWith("y"))
            {
                FIND_MARKERS();
                APPLY_MARKERS();
                pImg = &imgMasked;
            }
            else pImg = &imgInput;
            SAVE_IMAGE(imgMaxima, prefix + "_maxima.tif");
            FIND_LOCAL_MAXIMA(*pImg);
            cout << "Done." << endl;
        }
        else if (func_name == tr("one-pot"))
        {
            // TODO
        }
        else
        {
            // TODO
        }
        return true;
    }
    catch(exception& e)
    {
        cerr << "ERROR: " << e.what() << endl;
        return false;
    }
}
