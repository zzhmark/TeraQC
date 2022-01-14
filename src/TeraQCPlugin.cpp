#include "TeraQCPlugin.h"
#include "v3d_message.h"

Q_EXPORT_PLUGIN2(TeraQC, TeraQCPlugin);

QStringList TeraQCPlugin::menulist() const
{
    return QStringList();
}

QStringList TeraQCPlugin::funclist() const
{
    return QStringList()
            <<tr("single-test")
           <<tr("batch-test")
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
    if (func_name == tr("single-test"))
    {
    }
    else if (func_name == tr("batch-test"))
    {

    }
    else
    {
        v3d_msg(tr("TODO"), 0);
    }
    return true;
}
