TEMPLATE = app
TARGET = qRController
QT += core \
    gui
HEADERS += qbtscanner.h \
    qrcontroller.h
SOURCES += qbtscanner.cpp \
    main.cpp \
    qrcontroller.cpp
FORMS += qbtscanner.ui \
    qrcontroller.ui
RESOURCES += 

include(qextserialport-1.2beta1/src/qextserialport.pri)