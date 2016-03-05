TEMPLATE = lib
CONFIG += c++11
CONFIG += c99
CONFIG -= app_bundle
CONFIG -= lib_bundle
CONFIG -= qt
CONFIG += staticlib

VERSION=

include(../common.pri)

SOURCES += \
    I2CBuffer.cpp \
    IoBuffer.cpp \
    DS3231RealTimeClock.cpp \
    gpio/i2c.c \
    gpio/pierrors.c \
    gpio/pigpio.c \
    I2CSmbus.cpp \
    gpio/script.c \
    gpio/remoteclient.c \
    I2CGpioHardwareBus.cpp \
    I2CGpioSoftwareBus.cpp \
    I2CGpioBus.cpp \
    RegisterSet.cpp \
    RtcDebugger.cpp

HEADERS += \
    I2CBuffer.h \
    I2CBus.h \
    IoBuffer.h \
    RealTimeClock.h \
    RealTimeClockPrivate.h \
    RtcAlarm.h \
    RtcTime.h \
    DS3231RealTimeClock.h \
    DS3231RegisterSet.h \
    DS3231Registers.h \
    gpio/pierrors.h \
    gpio/pigpio.h \
    gpio/private.h \
    I2CSmbus.h \
    gpio/i2c.h \
    I2CGpioHardwareBus.h \
    I2CGpioSoftwareBus.h \
    I2CGpioBus.h \
    IoError.h \
    LogSupport.h \
    LocalTypes.h \
    RtcDebugger.h

