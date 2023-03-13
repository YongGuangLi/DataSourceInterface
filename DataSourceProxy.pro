QT += core concurrent network sql
QT -= gui

CONFIG += c++11

TARGET = DataSourceProxy
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    RedisHelper.cpp \
    dataswitchcenter.cpp \
    XdbThrift.cpp \
    xdb_types.cpp

HEADERS += \
    RedisHelper.h \
    dataswitchcenter.h \
    XdbThrift.h \
    xdb_types.h

unix:!macx: LIBS += -L$$PWD/../hiredis/lib/ -lhiredis

INCLUDEPATH += $$PWD/../hiredis/include
DEPENDPATH += $$PWD/../hiredis/include


INCLUDEPATH += $$PWD/../msgpack/include

unix:!macx: LIBS += -L$$PWD/../thrift-0.13.0/lib/ -lthrift

INCLUDEPATH += $$PWD/../thrift-0.13.0/include
DEPENDPATH += $$PWD/../thrift-0.13.0/include


INCLUDEPATH += $$PWD/../boost_1_71_0/include
