#
#                 Программа для демонстраии работы
#               многоканнального диспетчера очередей.
#

QT       -= core

QT       -= gui

TARGET = exness_testwork
CONFIG   += console
CONFIG   -= app_bundle

QMAKE_CXXFLAGS += -std=c++11
DEFINES += TARGET=\\\"$TARGET\\\"


TEMPLATE = app

BOOSTPATH = "D:\\work\\build-exness\\lib"
INCLUDEPATH += "C:\\MySoftware\\exness_testtask\\exness_testwork\\include"

SOURCES += .\src\main.cpp \
    .\src\error.cpp \
    .\src\init.cpp \
    .\src\testbench.cpp \
    .\src\node.cpp \
    .\src\syncro.cpp \
    src/pipeprocessor3.cpp

HEADERS += \
    .\include\pipeprocessor.h \
    .\include\pipeprocessor_cfg.h \
    .\include\common.h \
    .\include\error.h \
    .\include\init.h \
    .\include\testbench.h \
    .\include\node.h \
    .\include\syncro.h

OTHER_FILES += \
    TODO.txt


LIBS += $$BOOSTPATH\\libboost_iostreams-mgw48-mt-1_55.a
LIBS += $$BOOSTPATH\\libboost_system-mgw48-mt-1_55.a
LIBS += $$BOOSTPATH\\libboost_thread-mgw48-mt-1_55.a
LIBS += $$BOOSTPATH\\libboost_chrono-mgw48-mt-1_55.a
LIBS += $$BOOSTPATH\\libboost_random-mgw48-mt-1_55.a
