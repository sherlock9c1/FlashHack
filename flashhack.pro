QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    binfile.cpp \
    binfile_ee.cpp \
    binvisual.cpp \
    datastream/datastream_8192.cpp \
    datastream/datastream_interface.cpp \
    datastream/datastream_message.cpp \
    datastream/datastream_obdii.cpp \
    datastream/datastream_obdxpro.cpp \
    dialog_interface_select.cpp \
    flash/chip_id.cpp \
    flash/processor.cpp \
    flash/processor_8192dualflash.cpp \
    flash/processor_8192flash.cpp \
    flash/processor_8192read.cpp \
    flash/processor_ccm.cpp \
    flash/processor_ee.cpp \
    flash/processor_obdii.cpp \
    flash/processor_p66.cpp \
    log_widget.cpp \
    main.cpp \
    mainwindow.cpp \
    region_map.cpp \
    setting_ui.cpp \
    settings_dialog.cpp \
    tool_ee.cpp \
    tool_flash.cpp \
    useful.cpp

HEADERS += \
    binfile.h \
    binfile_ee.h \
    binvisual.h \
    config.h \
    datastream/datastream_8192.h \
    datastream/datastream_interface.h \
    datastream/datastream_message.h \
    datastream/datastream_obdii.h \
    datastream/datastream_obdxpro.h \
    dialog_interface_select.h \
    enums.h \
    flash/chip_id.h \
    flash/processor.h \
    flash/processor_8192dualflash.h \
    flash/processor_8192flash.h \
    flash/processor_8192read.h \
    flash/processor_ccm.h \
    flash/processor_ee.h \
    flash/processor_obdii.h \
    flash/processor_p66.h \
    log_widget.h \
    mainwindow.h \
    region_map.h \
    setting_ui.h \
    settings_dialog.h \
    tool_ee.h \
    tool_flash.h \
    useful.h

FORMS += \
    dialog_interface_select.ui \
    log_widget.ui \
    mainwindow.ui \
    setting_ui.ui \
    settings_dialog.ui \
    tool_ee.ui \
    tool_flash.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
  graphics.qrc

DISTFILES +=
