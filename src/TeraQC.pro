TEMPLATE = lib
CONFIG += qt plugin warn_off
#CONFIG	+= x86_64

PLUGIN_DEST = D:/Vaa3D/Vaa3D_V4.001_Windows_MSVC_64bit/plugins
V3D_SRC = D:/Vaa3D/v3d_external
OPENCV = D:/opencv/build
#OPENCV = D:/libs/opencv-3.4

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

#LIBS += -L$$OPENCV/x64/vc12/lib \
#    -lopencv_core340 -lopencv_imgproc340

#include the headers used in the project
HEADERS	+= TeraQCPlugin.h \
    TeraQCTypes.h \
    loadUtils.h \
    preprocessing.h \
    roiSampling.h

#include the source files used in the project
SOURCES	+= TeraQCPlugin.cpp \
    loadUtils.cpp \
    $$V3D_SRC/v3d_main/basic_c_fun/v3d_message.cpp \
    preprocessing.cpp \
    roiSampling.cpp

#specify target name and directory
TARGET	= $$qtLibraryTarget(TeraQC)
DESTDIR	= $$PLUGIN_DEST/TeraQC
