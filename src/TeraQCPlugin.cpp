#include "TeraQCPlugin.h"
#include "v3d_message.h"
#include "functions.h"
#include <vector>
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
            <<tr("test")
          <<tr("help");
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
    if (func_name == tr("test"))
    {
        v3d_msg("[TeraQC Plugin: Test]", 0);
        uchar* buffer = NULL;
        auto inlist = (vector<char*>*)(input.at(0).p);
        auto outlist = (vector<char*>*)(output.at(0).p);

        v3d_msg(QString("Loading from teraconverted images in ") + inlist->at(0) + "..", 0);
        V3DLONG sz[4];
        if (!reassemble_teraconvert(callback, inlist->at(0), buffer, sz))
        {
            v3d_msg("Loading failed.", 0);
            return false;
        }

        v3d_msg(QString("Saving to path ") + outlist->at(0) + "..", 0);
        if (!simple_saveimage_wrapper(callback, outlist->at(0), buffer, sz, V3D_UINT16))
        {
            v3d_msg("Saving failed. Free the memory.", 0);
            delete [] buffer;
            return false;
        }
        v3d_msg("Done. Free the memory.", 0);
        delete [] buffer;
    }
    else
    {
        v3d_msg(tr("TODO"), 0);
    }
    return true;
}
