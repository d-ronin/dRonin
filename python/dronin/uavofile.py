#!/usr/bin/env python3

# Copyright (C) 2018 dRonin, http://dronin.org

class UAVFileImport(dict):
    def __init__(self, contents, uavo_defs=None, githash='HEAD', warnings_fatal=False, sparse_ok=True):
        try:
            import lxml.etree as etree
        except:
            import xml.etree.ElementTree as etree

        from dronin import uavo_collection

        if uavo_defs is None:
            uavo_defs = uavo_collection.UAVOCollection()

            uavo_defs.from_git_hash(githash)
            self.githash = githash

        tree = etree.fromstring(contents)

        uavobjects = tree.findall('./settings/object')

        if not uavobjects:
            raise Exception("Can't find anything here")

        for xmlobj in uavobjects:
            uavo_key = int(xmlobj.get('id'), 0)
            name = xmlobj.get('name')

            obj = uavo_defs.find_by_name(name)

            if (obj is None):
                print("XML file contains UAVO %s which is missing in collection" % name)
                if (warnings_fatal):
                    raise KeyError("Missing UAVO")

                continue

            field_problem_expected = False

            if (uavo_key != obj._id):
                print("Object ID mismatch %s %x %x"%(name, obj._id, uavo_key))

                if (warnings_fatal):
                    raise KeyError("Object IDs mismatch")

                field_problem_expected = True

            obj_conv_fields = {}

            # Because we want to be able to import old UAV files into this
            # version, it's essential that we iterate our fields, and look
            # for the according XML tag.  Then we can pick the default if
            # it is not present or not a valid value.  (And likewise, it's
            # easy to ignore fields we don't care about anymore).
            for of in obj._fields[3:]:
                found_field = None

                for xf in xmlobj.findall('./field'):
                    xfname = xf.get('name')

                    if of == xfname:
                        found_field = xf
                        break

                if found_field is None:
                    if not sparse_ok:
                        raise KeyError("Couldn't find field %s" % (of))
                    continue

                raw_values = xf.get('values')

                #print("Found field, val=%s"%(raw_values))

                values = raw_values.split(',')

                type_name = obj._types[of]

                try:
                    for i in range(len(values)):
                        if type_name == 'enum':
                            values[i] = obj.string_to_enum(of, values[i])
                        elif type_name == 'float':
                            values[i] = float(values[i])
                        elif 'int' in type_name:
                            values[i] = int(values[i])
                        else:
                            raise KeyError("Field type invalid for %s" %(of))
                except Exception:
                    print("Unable to convert field %s" % (of))

                    if field_problem_expected:
                        continue

                    raise

                if len(values) == 1:
                    values = values[0]
                else:
                    values = tuple(values)

                obj_conv_fields[of] = values

            obj_instance = obj._make_to_send(**obj_conv_fields)

            self[name] = obj_instance

def main(argv):
    from dronin import uavo_collection
    import argparse

    parser = argparse.ArgumentParser(description="Import a logfs settings partition and convert to XML")
    parser.add_argument('filename', metavar='filename.bin', help="the filename in which the settings dump is stored")
    parser.add_argument('-g', dest='githash', metavar='githash', help="the githash to use to interpret the settings dump", default="HEAD")
    parser.add_argument("-n", "--not-defaults",
                        action  = "store_true",
                        default = False,
                        dest    = "not_defaults",
                        help    = "output only non-default values")
    arg = parser.parse_args()

    with open(arg.filename, "rb") as f:
        contents = f.read()

    uf = UAVFileImport(contents, githash=arg.githash)

    sys.stdout.write(uavo_collection.UAVOCollection.export_xml(uf.values(),
        only_nondefault=arg.not_defaults))

if __name__ == "__main__":
    import sys

    sys.path.append('.')

    main(sys.argv[1:])

