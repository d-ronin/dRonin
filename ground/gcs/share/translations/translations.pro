TEMPLATE = aux

include(../../gcs.pri)

LANGUAGES = de es fr ru zh_CN

# var, prepend, append
defineReplace(prependAll) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}

XMLPATTERNS = $$targetPath($$[QT_INSTALL_BINS]/xmlpatterns)
LUPDATE = $$targetPath($$[QT_INSTALL_BINS]/lupdate) -locations relative -no-ui-lines -no-sort
LRELEASE = $$targetPath($$[QT_INSTALL_BINS]/lrelease)
LCONVERT = $$targetPath($$[QT_INSTALL_BINS]/lconvert)

wd = $$replace(GCS_SOURCE_TREE, /, $$QMAKE_DIR_SEP)

TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/dronin_,.ts)

win32: \
    PREFIX = "file:///"
else: \
    PREFIX = "file://"

plugin_sources = $$files($$GCS_SOURCE_TREE/src/plugins/*)
plugin_sources ~= s,^$$re_escape($$GCS_SOURCE_TREE/),,g$$i_flag
plugin_sources -= src/plugins/plugins.pro
sources = src/app src/libs $$plugin_sources share/welcome

for(path, INCLUDEPATH): include_options *= -I$$shell_quote($$path)

files = $$files($$PWD/*_??.ts) $$PWD/dronin_untranslated.ts
for(file, files) {
    lang = $$replace(file, .*_([^/]*)\\.ts, \\1)
    v = ts-$${lang}.commands
    $$v = cd $$wd && $$LUPDATE $$include_options $$sources -ts $$file
    QMAKE_EXTRA_TARGETS += ts-$$lang
}
ts-all.commands = cd $$wd && $$LUPDATE $$include_options $$sources -ts $$files
QMAKE_EXTRA_TARGETS += ts-all

ts.commands = \
    @echo \"The \'ts\' target has been removed in favor of more fine-grained targets.\" && \
    echo \"Use \'ts-<lang>\' instead. To add a language, use \'ts-untranslated\',\" && \
    echo \"rename the file and re-run \'qmake\'.\"
QMAKE_EXTRA_TARGETS += ts

updateqm.input = TRANSLATIONS
updateqm.output = $$GCS_BUILD_TREE/share/translations/${QMAKE_FILE_BASE}.qm
isEmpty(vcproj):updateqm.variable_out = PRE_TARGETDEPS
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

qmfiles.files = $$prependAll(LANGUAGES, $$GCS_BUILD_TREE/share/translations/dronin_,.qm)
qmfiles.path = $$INSTALL_DATA_PATH/translations
qmfiles.CONFIG += no_check_exist
INSTALLS += qmfiles
