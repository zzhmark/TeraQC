/*
 * Copyright 2022 Zuohan Zhao
 * SPDX-License-Identifier: Apache-2.0
*/

#include "TeraQCPlugin.h"
#include "loadUtils.h"
#include "findMarkers.h"
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
            // 1. Load image
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

            // 2. finding markers
            cout << "\tFinding markers.." << endl;
            if(!findMarkers(imgInput, imgMarker, params))
                throw runtime_error("Finding markers failed.");

            // 3. Saving the mask
            cout << "\tSaving the mask to path " << outlist->at(0) << endl;
            if (!simple_saveimage_wrapper(callback, outlist->at(0),
                                          imgMarker.buffer, imgMarker.sz, imgMarker.datatype))
                throw runtime_error("Saving failed");

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
        auto inlist = (vector<char*>*)(input.at(0).p);
        auto outlist = (vector<char*>*)(output.at(0).p);
        auto arglist = (vector<char*>*)(input.at(1).p);
        QVariantMap params;
        try
        {
            cout << "[TeraQC Plugin: Find Local Maxima]" << endl;

            // 1. Set argument list
            for (int i = 0; i < arglist->size(); i+=2)
                params[arglist->at(i)] = arglist->at(i + 1);

            // 2. Load image
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

            // 3. Optional marker masking

            // 4. Find local maxima

            // 5. Saving the mask
            cout << "\tSaving the mask to path " << outlist->at(0) << endl;
            if (!simple_saveimage_wrapper(callback, outlist->at(0),
                                          imgMaxima.buffer, imgMaxima.sz, imgMaxima.datatype))
                throw runtime_error("Saving failed");

            imgInput.clear();
            imgMarker.clear();
            imgMaxima.clear();
            cout << "Done." << endl;
        }
        catch(exception& e)
        {
            cerr << "ERROR: " << e.what() << endl;
            imgInput.clear();
            imgMarker.clear();
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
