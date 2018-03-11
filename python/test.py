#!/usr/bin/env python3

def main():

    # Load the UAVO xml files in the workspace
    import dronin
    uavo_defs = dronin.uavo_collection.UAVOCollection()
    uavo_defs.from_uavo_xml_path('shared/uavobjectdefinition')

#-------------------------------------------------------------------------------
if __name__ == "__main__":
    main()
