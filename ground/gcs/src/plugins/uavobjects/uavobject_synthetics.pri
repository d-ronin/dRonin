# Provide the path to the auto-generated uavobject source files for the GCS.
UAVOBJECT_SYNTHETICS=$${GCS_BUILD_TREE}/../../uavobject-synthetics/gcs

# Add the include path to the auto-generated uavobject include files.
INCLUDEPATH += $$UAVOBJECT_SYNTHETICS
DEPENDPATH += $$UAVOBJECT_SYNTHETICS
