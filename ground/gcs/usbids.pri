include(gcs.pri)

USB_ID_SOURCE_DIR = $$clean_path($$GCS_SOURCE_TREE/../../shared/usb_ids)
USB_ID_JSON = $$USB_ID_SOURCE_DIR/usb_ids.json

USB_ID_OUT_DIR = $$clean_path($$GCS_BUILD_TREE/../../shared/usb_ids)
USB_ID_HEADER = $$USB_ID_OUT_DIR/board_usb_ids.h

# Behold the USB Header Compiler
uhc.output  = $$USB_ID_HEADER
uhc.commands = $$USB_ID_SOURCE_DIR/generate_usb_files.py -i ${QMAKE_FILE_NAME} -c ${QMAKE_FILE_OUT}
uhc.depends = $$USB_ID_JSON
uhc.input = USB_ID_JSON
uhc.CONFIG = explicit_dependencies no_link
QMAKE_EXTRA_COMPILERS += uhc

INCLUDEPATH *= $$USB_ID_OUT_DIR
