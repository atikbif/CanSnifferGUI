#-------------------------------------------------
#
# Project created by QtCreator 2018-07-31T15:25:34
#
#-------------------------------------------------

QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

RC_ICONS = logo.ico
VERSION = 1.1.0.0

QMAKE_TARGET_COMPANY = modern technical solutions
QMAKE_TARGET_PRODUCT = Can Sniffer 1.0
QMAKE_TARGET_DESCRIPTION = CAN sniffer archive reader
QMAKE_TARGET_COPYRIGHT = modern technical solutions

TARGET = ReadCanData
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    udpcontroller.cpp \
    udpreader.cpp \
    checksum.cpp \
    archiveanalyzer.cpp \
    canrequest.cpp \
    bootloadercontroller.cpp \
    bootloader.cpp

HEADERS += \
        mainwindow.h \
    udpcontroller.h \
    udpreader.h \
    checksum.h \
    archiveanalyzer.h \
    canrequest.h \
    bootloadercontroller.h \
    bootloader.h

FORMS += \
        mainwindow.ui
