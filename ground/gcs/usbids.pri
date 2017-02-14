USB_ID_SOURCE_DIR = $$clean_path($$GCS_SOURCE_TREE/../../shared/usb_ids)
USB_ID_JSON = $$USB_ID_SOURCE_DIR/usb_ids.json

USB_ID_OUT_DIR = $$clean_path($$GCS_BUILD_TREE/../../shared/usb_ids)
USB_ID_HEADER = $$USB_ID_OUT_DIR/board_usb_ids.h

usbidjson.target = $$USB_ID_JSON

usbids.target = $$USB_ID_HEADER
usbids.depends = usbidjson
usbids.command = $$PYTHON $$USB_ID_SOURCE_DIR/generate_usb_files.py -i "$$usbidjson.target" -c "$$usbids.target"

INCLUDEPATH *= $$USB_ID_OUT_DIR
