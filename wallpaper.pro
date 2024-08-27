QT += core gui widgets multimedia multimediawidgets webenginewidgets
LIBS += -lUser32
#CONFIG += c++11 console

CONFIG -= console
CONFIG += windows
RC_FILE = myicon.rc

QMAKE_CXXFLAGS_RELEASE += $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE += $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO



# DEFINES +=QT_NO_DEBUG_OUTPUT # disable debug output
DEFINES -=QT_NO_DEBUG_OUTPUT # enable debug output

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
win32-msvc*: {
    QMAKE_CFLAGS *= /utf-8
    QMAKE_CXXFLAGS *= /utf-8
}
#不加上面这个在MSVC下编译会导致样式表编码出错
SOURCES += \
    src/ImageGroup.cpp \
    src/MyWebEngineView.cpp \
    src/desktopwidget.cpp \
    src/imageview.cpp \
    src/listwidgetitem.cpp \
    src/main.cpp \
    src/mainwidget.cpp \
    src/sliderfilter.cpp \
    src/videoview.cpp \
    src/MyWebEngineView.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    include/desktopwidget.h \
    include/EnterKeyEventFilter.h \
    include/ImageGroup.h \
    include/MyWebEngineView.h \
    include/imageview.h \
    include/listwidgetitem.h \
    include/mainwidget.h \
    include/sliderfilter.h \
    include/videoview.h \
    include/MyWebEngineView.h

RESOURCES += \
    Resource.qrc

FORMS += \
    mainwidget.ui

DISTFILES += \
    myicon.rc \
    web_scrollbarstyle.css
