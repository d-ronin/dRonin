"""
UAVO collection interface.

Copyright (C) 2014-2015 Tau Labs, http://taulabs.org
Copyright (C) 2015 dRonin, http://dronin.org

Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)
"""

from . import uavo

import operator
import os.path as op

GITHASH_OF_LAST_RESORT = 'Release-20160120.3'

class UAVOCollection(dict):
    def __init__(self):
        self.clear()

    def find_by_name(self, uavo_name):
        if uavo_name[0:5]!='UAVO_':
            uavo_name = 'UAVO_' + uavo_name

        for u in self.values():
            if u._name == uavo_name:
                return u

        return None

    def get_settings_objects(self):
        objs = [ u for u in self.values() if u._is_settings ]
        objs.sort(key=operator.attrgetter('_name'))

        return objs

    def from_file_contents(self, content_list):
        some_processed = True

        # There are dependencies here
        while some_processed:
            some_processed = False

            unprocessed = []

            # Build up the UAV objects from the xml definitions
            for contents in content_list:
                try:
                    u = uavo.make_class(self, contents)

                    # add this uavo definition to our dictionary
                    self.update([('{0:08x}'.format(u._id), u)])

                    some_processed = True
                except Exception:
                    unprocessed.append(contents)

            content_list = unprocessed

        if len(content_list):
            raise Exception("Unable to parse some uavo files")

    def from_tar_file(self, t):
        # Get the file members
        content_list = []

        for f_info in t.getmembers():
            if f_info.name.count("oplinksettings") > 0:
                continue

            if not f_info.name.endswith('.xml'):
                continue

            f = t.extractfile(f_info)
            content_list.append(f.read())
            f.close()

        self.from_file_contents(content_list)

    def from_tar_bytes(self, contents):
        from io import BytesIO
        import tarfile

        # coerce the tar file data into a file object so that tarfile likes it
        fobj = BytesIO(contents)

        # feed the tar file data to a tarfile object
        with tarfile.open(fileobj=fobj) as t:
            self.from_tar_file(t)

    def from_git_hash(self, githashes):
        if not isinstance(githashes, list):
            githashes = [ githashes ]

        for h in githashes:
            try:
                #
                # Grab the exact uavo definition files from the git repo using 
                # the provided hash
                #
                import subprocess

                # Get the directory where the code is located
                src_dir = op.join(op.dirname(__file__), "..", "..")

                p = subprocess.Popen(['git', 'archive', h, '--', 'shared/uavobjectdefinition/'],
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                     cwd=src_dir)
                # grab the tar file data
                git_archive_data, git_archive_errors = p.communicate()

                if p.returncode == 0:
                    self.from_tar_bytes(git_archive_data)
                    return
            except Exception:
                # Popen isn't available on GAE, so all of the above fails.
                pass

            try:
                from six.moves.urllib.request import urlopen

                # TODO -- this can be bundled into a single request and unrolled.
                web_data = urlopen("http://dronin-autotown.appspot.com/uavos/%s" % (h))

                if web_data.getcode() != 200:
                    continue

                self.from_tar_bytes(web_data.read())
                return
            except Exception:
                pass

        raise Exception("Couldn't get any UAVO definitions by githash")

    def from_uavo_xml_path(self, path):
        import os
        import glob

        content_list = []

        for file_name in glob.glob(os.path.join(path, '*.xml')):
            with open(file_name, 'rU') as f:
                content_list.append(f.read())

        self.from_file_contents(content_list)
