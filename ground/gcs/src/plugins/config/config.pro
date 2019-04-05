TEMPLATE = lib
TARGET = Config
DEFINES += CONFIG_LIBRARY
DEFINES += QWT_DLL

QT += svg
QT += network
QT += charts

include(../../gcsplugin.pri)

include(../../libs/eigen/eigen.pri)
include(../../libs/qwt/qwt.pri)
include(../../libs/utils/utils.pri)

include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/uavobjects/uavobjects.pri)
include(../../plugins/uavobjectutil/uavobjectutil.pri)
include(../../plugins/uavobjectwidgetutils/uavobjectwidgetutils.pri)
include(../../plugins/uavsettingsimportexport/uavsettingsimportexport.pri)
include(../../plugins/uavtalk/uavtalk.pri)

OTHER_FILES += Config.pluginspec

HEADERS += calibration.h \
    configplugin.h \
    configgadgetconfiguration.h \
    configgadgetwidget.h \
    configgadgetfactory.h \
    configgadgetoptionspage.h \
    configgadget.h \
    configinputwidget.h \
    configoutputwidget.h \
    configvehicletypewidget.h \
    configstabilizationwidget.h \
    assertions.h \
    defaulthwsettingswidget.h \
    inputchannelform.h \
    configtxpidwidget.h \
    outputchannelform.h \
    cfg_vehicletypes/configccpmwidget.h \
    cfg_vehicletypes/configfixedwingwidget.h \
    cfg_vehicletypes/configgroundvehiclewidget.h \
    cfg_vehicletypes/configmultirotorwidget.h \
    cfg_vehicletypes/vehicleconfig.h \
    configattitudewidget.h \
    config_global.h \
    mixercurve.h \
    dblspindelegate.h \
    configautotunewidget.h \
    tempcompcurve.h \
    textbubbleslider.h \
    vehicletrim.h \
    configmodulewidget.h \
    configosdwidget.h \
    configradiowidget.h \
    expocurve.h \
    qreadonlycheckbox.h

SOURCES += calibration.cpp \
    configplugin.cpp \
    configgadgetconfiguration.cpp \
    configgadgetwidget.cpp \
    configgadgetfactory.cpp \
    configgadgetoptionspage.cpp \
    configgadget.cpp \
    configinputwidget.cpp \
    configoutputwidget.cpp \
    configvehicletypewidget.cpp \
    configstabilizationwidget.cpp \
    defaulthwsettingswidget.cpp \
    inputchannelform.cpp \
    configattitudewidget.cpp \
    configtxpidwidget.cpp \
    cfg_vehicletypes/configccpmwidget.cpp \
    cfg_vehicletypes/configfixedwingwidget.cpp \
    cfg_vehicletypes/configgroundvehiclewidget.cpp \
    cfg_vehicletypes/configmultirotorwidget.cpp \
    cfg_vehicletypes/vehicleconfig.cpp \
    outputchannelform.cpp \
    mixercurve.cpp \
    dblspindelegate.cpp \
    configautotunewidget.cpp \
    tempcompcurve.cpp \
    textbubbleslider.cpp \
    vehicletrim.cpp \
    configmodulewidget.cpp \
    configosdwidget.cpp \
    configradiowidget.cpp \
    expocurve.cpp

FORMS += airframe.ui \
    ccpm.ui \
    stabilization.ui \
    input.ui \
    output.ui \
    defaulthwsettings.ui \
    inputchannelform.ui \
    outputchannelform.ui \
    attitude.ui \
    txpid.ui \
    mixercurve.ui \
    autotune.ui \
    modules.ui \
    osd.ui \
    osdpage.ui \
    integratedradio.ui \
    autotunebeginning.ui \
    autotuneproperties.ui \
    autotunesliders.ui \
    autotunefinalpage.ui

RESOURCES += configgadget.qrc
