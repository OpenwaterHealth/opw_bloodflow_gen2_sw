QT += core quick gui serialbus serialport qml widgets network

QTPLUGINS += qtvirtualkeyboardplugin

CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = Gen2GUI.bin

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        cryptomanager.cpp \
        filelist.cpp \
        filemanager.cpp \
        fwupdate.cpp \
        main.cpp \
        owconnector.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /data/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += qml.qrc
DEFINES += QT_DEPRECATED_WARNINGS

HEADERS += \
    cryptomanager.h \
    filelist.h \
    filemanager.h \
    fwupdate.h \
    owconnector.h
