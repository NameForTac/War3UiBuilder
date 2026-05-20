QT       += core gui widgets
CONFIG   += c++17

TARGET = War3UiBuilder
TEMPLATE = app

DESTDIR = bin
OBJECTS_DIR = tmp/obj
MOC_DIR = tmp/moc
RCC_DIR = tmp/rcc
UI_DIR = tmp/ui

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    canvas/uiscene.cpp \
    canvas/uiview.cpp \
    elements/uielement.cpp \
    elements/uielementdata.cpp \
    panels/treepanel.cpp \
    panels/propertypanel.cpp \
    panels/texturepanel.cpp \
    panels/searchreplacedialog.cpp \
    managers/projectmanager.cpp \
    managers/undomanager.cpp \
    export/inigenerator.cpp \
    export/fdfgenerator.cpp \
    3rdparty/stb_image.cpp

HEADERS += \
    mainwindow.h \
    canvas/uiscene.h \
    canvas/uiview.h \
    elements/uielement.h \
    elements/uielementdata.h \
    panels/treepanel.h \
    panels/propertypanel.h \
    panels/texturepanel.h \
    panels/searchreplacedialog.h \
    managers/projectmanager.h \
    managers/undomanager.h \
    export/inigenerator.h \
    export/fdfgenerator.h

INCLUDEPATH += 3rdparty

RESOURCES += resources/resources.qrc
