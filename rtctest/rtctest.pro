TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

include(../common.pri)

INCLUDEPATH += ../rtcsupport

LIBS += $${VCLOCK_ROOT}/build/rtcsupport/$${CONFIGURATION}/librtcsupport.a -lglog -lgflags -lboost_date_time -lboost_system -lboost_chrono

SOURCES += \
    rtctest.cpp

