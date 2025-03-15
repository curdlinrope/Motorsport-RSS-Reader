QT       += core gui network xml svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MotorsportRSS
TEMPLATE = app
VERSION = 1.0.0

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/rssfeedmodel.cpp \
    src/newsfeedwidget.cpp \
    src/rssparser.cpp

HEADERS += \
    src/mainwindow.h \
    src/rssfeedmodel.h \
    src/newsfeedwidget.h \
    src/rssparser.h

FORMS += \
    src/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/local/bin/$${TARGET}
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources/resources.qrc

# Desktop file for Linux
unix {
    desktop.path = /usr/share/applications
    desktop.files += MotorsportRSS.desktop
    INSTALLS += desktop
    
    icon.path = /usr/share/icons/hicolor/256x256/apps
    icon.files += resources/icons/motorsportrss.png
    INSTALLS += icon
} 