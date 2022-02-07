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
            << tr("findMarkers")
            << tr("removeMarkers")
            << tr("findLocalMaxima")
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
        }
        else if (info.isDir())
        {
            cout << "\tLoading teraconverted images in " << inlist->at(0) << ".." << endl;
            if (!load_teraconvert_dir(inlist->at(0), imgInput,
                                      loader, params.value("datatype", V3D_UINT16).toInt()))
                throw runtime_error("Loading failed.");
        } else
            throw runtime_error("Illegal loading path. Neither an image nor teraconvert data.");
    };

    auto FIND_MARKERS = [&]() {
        cout << "\tFinding markers.." << endl;
        if(!findMarkers(imgInput, imgMarker, params))
            throw runtime_error("Finding markers failed.");
    };

    auto SAVE_IMAGE = [&](QcImage& img) {
        cout << "\tSaving the mask to path " << outlist->at(0) << endl;
        if (!simple_saveimage_wrapper(callback, outlist->at(0), img.buffer, img.sz, img.datatype))
            throw runtime_error("Saving failed");
    };

    auto REMOVE_MARKERS = [&]() {
        cout << "\tRemove markers from the input image.." << endl;
        if(!masking(imgInput, imgWoMarker, imgMarker))
            throw runtime_error("Removing markers failed.");
    };

    auto FIND_LOCAL_MAXIMA = [&](const QcImage& img) {
        findLocalMaxima(img, imgMaxima, params);
    };

    // commands
    if (input.size() > 1)
    {
        arglist = (vector<char*>*)(input.at(1).p);
        for (int i = 0; i < arglist->size(); i+=2)
        params[arglist->at(i)] = arglist->at(i + 1);
    }

    if (func_name == tr("findMarkers"))
    {
        try
        {
            cout << "[TeraQC Plugin: Find Markers]" << endl;

            LOAD_IMAGE();
            FIND_MARKERS();
            SAVE_IMAGE(imgMarker);

            imgInput.clear();
            imgMarker.clear();
            cout << "Done." << endl;
        }
        catch(exception& e)
        {
            cerr << "ERROR: " << e.what() << endl;
            imgInput.clear();
            imgMarker.clear();
            return false;
        }
    }
    else if (func_name == tr("removeMarkers"))
    {
        try
        {
            cout << "[TeraQC Plugin: Remove Markers]" << endl;

            LOAD_IMAGE();
            FIND_MARKERS();
            REMOVE_MARKERS();
            SAVE_IMAGE(imgWoMarker);

            imgInput.clear();
            imgMarker.clear();
            cout << "Done." << endl;
        }
        catch(exception& e)
        {
            cerr << "ERROR: " << e.what() << endl;
            imgInput.clear();
            imgMarker.clear();
            return false;
        }
    }
    else if (func_name == tr("findLocalMaxima"))
    {
        try
        {
            cout << "[TeraQC Plugin: Find Local Maxima]" << endl;
            LOAD_IMAGE();
            QcImage* pImg;
            if (params.value("preprocessing", "y").toString().toLower().startsWith("y"))
            {
                FIND_MARKERS();
                REMOVE_MARKERS();
                pImg = &imgWoMarker;
            }
            else pImg = &imgInput;
            SAVE_IMAGE(imgMaxima);
            FIND_LOCAL_MAXIMA(*pImg);
            imgInput.clear();
            imgMarker.clear();
            imgWoMarker.clear();
            imgMaxima.clear();
            cout << "Done." << endl;
        }
        catch(exception& e)
        {
            cerr << "ERROR: " << e.what() << endl;
            imgInput.clear();
            imgMarker.clear();
            imgWoMarker.clear();
            imgMaxima.clear();
            cout << "Done." << endl;
            return false;
        }
    }
    else
    {
        cout << "TODO" << endl;
    }
    return true;
}
