QT       += core gui widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET = HospitalManagementSystem
TEMPLATE = app

SOURCES += \
    main.cpp \
    login.cpp \
    mainwindow.cpp \
    patient.cpp \
    doctor.cpp \
    appointment.cpp \
    emergency.cpp \
    filemanager.cpp

HEADERS += \
    login.h \
    mainwindow.h \
    patient.h \
    doctor.h \
    appointment.h \
    emergency.h \
    filemanager.h

RESOURCES += \
    resources.qrc

# Default data files are looked for in the application's working directory.
# Copy them alongside the executable for first run.
