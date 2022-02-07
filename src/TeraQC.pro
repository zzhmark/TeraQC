TEMPLATE = lib
CONFIG += qt plugin warn_off
#CONFIG	+= x86_64

PLUGIN_DEST = D:/Vaa3D/Vaa3D_V4.001_Windows_MSVC_64bit/plugins
V3D_SRC = D:/Vaa3D/v3d_external
OPENCV = D:/opencv/build


#include necessary paths
INCLUDEPATH	+= $$V3D_SRC/v3d_main/basic_c_fun \
    $$V3D_SRC/v3d_main/common_lib/include \
    $$OPENCV/include \
    $$OPENCV/include/opencv \
    $$OPENCV/include/opencv2

LIBS += -L. \
    -L$$V3D_SRC/v3d_main/common_lib/lib

CONFIG(debug, debug|release){
    LIBS += -L$$OPENCV/x64/vc12/lib -lopencv_world310d
} else {
    LIBS += -L$$OPENCV/x64/vc12/lib -lopencv_world310
}

#include the headers used in the project
HEADERS	+= TeraQCPlugin.h \
    TeraQCTypes.h \
    findLocalMaxima.h \
    findMarkers.h \
    loadUtils.h

#include the source files used in the project
SOURCES	+= TeraQCPlugin.cpp \
    findLocalMaxima.cpp \
    findMarkers.cpp \
    loadUtils.cpp \
    $$V3D_SRC/v3d_main/basic_c_fun/v3d_message.cpp

#specify target name and directory
TARGET	= $$qtLibraryTarget(TeraQC)
DESTDIR	= $$PLUGIN_DEST/TeraQC
