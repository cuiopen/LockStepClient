#-------------------------------------------------
#
# Project created by QtCreator 2017-06-10T08:27:16
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test
TEMPLATE = app

CONFIG += C++11
INCLUDEPATH += C:\Users\khaki\Downloads\Compressed\protobuf-cpp-3.3.0_2\protobuf-3.3.0\cmake\build\release\include
LIBS += D:\qt\test\proto\lib\libprotobuf.lib

QT += network

SOURCES += main.cpp\
        widget.cpp \
    proto/cs.pb.cc \
    buffer.cpp

HEADERS  += widget.h \
    proto/cs.pb.h \
    buffer.h \
    log.h

FORMS    += widget.ui
