# -------------------------------------------------
# Project created by QtCreator 2011-11-06T21:37:41
# -------------------------------------------------
QT += core \
    gui
contains(QT_VERSION, ^5\\..*\\..*):QT += widgets
TARGET = uartassistant
TEMPLATE = app
include(../../src/qextserialport.pri)
SOURCES += ../enumerator/main.cpp \
    ../event/PortListener.cpp \
    ../event/main.cpp \
    ../qespta/MainWindow.cpp \
    ../qespta/MessageWindow.cpp \
    ../qespta/QespTest.cpp \
    ../qespta/main.cpp \
    dialog.cpp \
    hled.cpp \
    main.cpp \
    ../../src/qextserialenumerator.cpp \
    ../../src/qextserialenumerator_osx.cpp \
    ../../src/qextserialenumerator_unix.cpp \
    ../../src/qextserialenumerator_win.cpp \
    ../../src/qextserialport.cpp \
    ../../src/qextserialport_unix.cpp \
    ../../src/qextserialport_win.cpp \
    ../../src/qextwineventnotifier_p.cpp \
    ../../tests/qextwineventnotifier/tst_qextwineventnotifier.cpp \
    main.cpp \
    dialog.cpp \
    hled.cpp
HEADERS += dialog.h \
    hled.h
FORMS += dialog.ui \
    dialog.ui
