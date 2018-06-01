#
#                 Программа для демонстраии работы
#               многоканнального диспетчера очередей.
#

QT       =

TARGET = exness_testwork
CONFIG   += console
CONFIG   -= app_bundle

QMAKE_CXXFLAGS =
QMAKE_CXXFLAGS += -std=c++11
QMAKE_CXXFLAGS += -march=i686
#QMAKE_CXXFLAGS = D:\\Mingw_32_cpp11\\include\\c++\\4.8.3
DEFINES += TARGET=\\\"$TARGET\\\"

TEMPLATE = app

BOOSTPATH = "D:\\work\\build-exness\\lib2"
INCLUDEPATH += "C:\\MySoftware\\exness_testtask\\export2\\exness_testwork\\include"
INCLUDEPATH += "C:\\MySoftware\\exness_testtask\\exness_testwork\\boost158"

SOURCES += .\src\main.cpp \
     .\src\error.cpp \
     .\src\init.cpp \
    .\src\testbench.cpp \
     .\src\node.cpp \
     .\src\syncro.cpp \
    .\src/pipeprocessor3.cpp \
    .\src/pipeprocessor4.cpp \
     .\src/mng_thread.cpp

HEADERS += \
    .\include\pipeprocessor.h \
     .\include\pipeprocessor_cfg.h \
    .\include\common.h \
    .\include\error.h \
    .\include\init.h \
    .\include\testbench.h \
     .\include\node.h \
     .\include\syncro.h \
     .\include/mng_thread.h \
     .\include/nodeImpl.h \
     .\include/pipetraits.h

OTHER_FILES += \
    TODO.txt

LIBS += $$BOOSTPATH\\libboost_iostreams-mgw49-mt-1_58.a
LIBS += $$BOOSTPATH\\libboost_system-mgw49-mt-1_58.a
LIBS += $$BOOSTPATH\\libboost_thread-mgw49-mt-1_58.a
LIBS += $$BOOSTPATH\\libboost_chrono-mgw49-mt-1_58.a
LIBS += $$BOOSTPATH\\libboost_random-mgw49-mt-1_58.a

DISTFILES +=
