"""
UAVO collection interface.

Copyright (C) 2014-2015 Tau Labs, http://taulabs.org
Copyright (C) 2015 dRonin, http://dronin.org

Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)
"""

import uavo

import operator
import os.path as op

GITHASH_OF_LAST_RESORT = 'Release-20160120.3'

class UAVOCollection(dict):
    def __init__(self):
        self.clear()

    def find_by_name(self, uavo_name):
        if uavo_name[0:5]!='UAVO_':
            uavo_name = 'UAVO_' + uavo_name

        for u in self.itervalues():
            if u._name == uavo_name:
                return u

        return None

    def get_settings_objects(self):
        objs = [ u for u in self.itervalues() if u._is_settings ]
        objs.sort(key=operator.attrgetter('_name'))

        return objs

    def from_tar_file(self, t):
        # Get the file members
        f_members = []
        for f_info in t.getmembers():
            if not f_info.isfile():
                continue
            # Make sure the shared definitions are parsed first
            if op.basename(f_info.name).lower().find('shared') > 0:
                f_members.insert(0, f_info)
            else:
                f_members.append(f_info)

        # Build up the UAV objects from the xml definitions
        for f_info in f_members:
            f = t.extractfile(f_info)

            u = uavo.make_class(self, f)

            # add this uavo definition to our dictionary
            self.update([('{0:08x}'.format(u._id), u)])

    def from_tar_bytes(self, contents):
        from cStringIO import StringIO
        import tarfile

        # coerce the tar file data into a file object so that tarfile likes it
        fobj = StringIO(contents)

        # feed the tar file data to a tarfile object
        with tarfile.open(fileobj=fobj) as t:
            self.from_tar_file(t)

    def from_git_hash(self, githash):
        import subprocess
        # Get the directory where the code is located
        src_dir = op.join(op.dirname(__file__), "..", "..")
        #
        # Grab the exact uavo definition files from the git repo using the header's git hash
        #
        try:
            p = subprocess.Popen(['git', 'archive', githash, '--', 'shared/uavobjectdefinition/'],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 cwd=src_dir)
            # grab the tar file data
            git_archive_data, git_archive_errors = p.communicate()

            if p.returncode == 0:
                self.from_tar_bytes(git_archive_data)
                return
        except:
            # Popen isn't available on GAE, so all of the above fails.
            pass

        #print "Error exit status, falling back to cloud"

        import urllib2

        web_data = urllib2.urlopen("http://dronin-autotown.appspot.com/uavos/%s?altGitHash=%s" % (
            githash, GITHASH_OF_LAST_RESORT)).read()

        self.from_tar_bytes(web_data)

    def from_uavo_xml_path(self, path):
        import os
        import glob

        # Make sure the shared definitions are parsed first
        file_names = []
        for file_name in glob.glob(os.path.join(path, '*.xml')):
            if op.basename(file_name).lower().find('shared') > 0:
                file_names.insert(0, file_name)
            else:
                file_names.append(file_name)

        for file_name in file_names:
            with open(file_name, 'rU') as f:
                u = uavo.make_class(self, f)

                # add this uavo definition to our dictionary
                self.update([('{0:08x}'.format(u._id), u)])
