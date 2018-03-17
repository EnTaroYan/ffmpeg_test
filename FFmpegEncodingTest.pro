QT += core
QT -= gui
QT += network

CONFIG += c++11

TARGET = FFmpegEncodingTest
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    encodeandsend.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which asqt been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0



LIBS+=-L/usr/local/lib -lavformat -lavcodec -lavutil -lavdevice -lswscale -lx264
LIBS+=-lpthread -lm -ldl -lfreetype -lz -lv4l2
LIBS+=-L/opt/vc/lib/ -lmmal_core -lmmal_util -lmmal_vc_client -lbcm_host

INCLUDEPATH+=/usr/local/include

HEADERS += \
    encodeandsend.h


