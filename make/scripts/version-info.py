#!/usr/bin/env python
#
# Utility functions to access git repository info and
# generate source files and binary objects using templates.
#
# (c) 2011, The OpenPilot Team, http://www.openpilot.org
# (c) 2017, dRonin
#
# See also: The GNU Public License (GPL) Version 3
#

from __future__ import print_function

from codecs import open
from cgi import escape
from datetime import datetime
import hashlib
import argparse
import re
from string import Template
from subprocess import Popen, PIPE
import sys

class Repo:
    """A simple git repository HEAD commit info class

    This simple class provides object notation to access
    the git repository current commit info.  If one needs
    better access one can try the GitPython class available
    here:
            http://packages.python.org/GitPython
    It is not installed by default, so we cannot rely on it.

    Example:
        r = Repo('/path/to/git/repository')
        print "path:       ", r.path()
        print "origin:     ", r.origin()
        print "hash:       ", r.hash()
        print "short hash: ", r.hash(8)
        print "Unix time:  ", r.time()
        print "commit date:", r.time("%Y%m%d")
        print "commit tag: ", r.tag()
        print "branch:     ", r.branch()
        print "release tag:", r.reltag()
    """

    def _exec(self, cmd):
        """Execute git using cmd as arguments"""
        self._git = 'git'
        git = Popen(self._git + " " + cmd, cwd=self._path,
                    shell=True, stdout=PIPE, stderr=PIPE)
        self._out, self._err = git.communicate()

        self._out = self._out.decode('utf-8')
        self._err = self._err.decode('utf-8')

        self._rc = git.poll()

    def _get_origin(self):
        """Get and store the repository fetch origin path"""
        self._origin = None
        self._exec('remote -v')
        if self._rc == 0:
            m = re.search(r"^origin\s+(.+)\s+\(fetch\)$", self._out, re.MULTILINE)
            if m:
                self._origin = m.group(1)

    def _get_ancestor(self):
        """Finds our closest ancestor in next"""
        self._ancestor = None
        self._exec('merge-base upstream/next HEAD')
        if self._rc == 0:
            self._ancestor = self._out.strip()
            return
        self._exec('merge-base origin/next HEAD')
        if self._rc == 0:
            self._ancestor = self._out.strip()
            return

    def _get_time(self):
        """Get and store HEAD commit timestamp in Unix format

        We use commit timestamp rather than the build time,
        so it always is the same for the current commit or tag.
        """
        self._time = None
        self._exec('log -n1 --no-color --format=format:%ct HEAD')
        if self._rc == 0:
            self._time = self._out

    def _get_tag(self):
        """Get and store git tag for the HEAD commit"""
        self._tag = None
        self._exec('describe --tags --exact-match HEAD')
        if self._rc == 0:
            self._tag = self._out.strip(' \t\n\r')

    def _get_branch(self):
        """Get and store current branch containing the HEAD commit"""
        self._branch = None
        self._exec('branch --contains HEAD')
        if self._rc == 0:
            m = re.search(r"^\*\s+(.+)$", self._out, re.MULTILINE)
            if m:
                self._branch = m.group(1)

    def _get_dirty(self):
        """Check for dirty state of repository"""
        self._dirty = False
        self._exec('update-index --refresh --unmerged')
        self._exec('diff-index --name-only --exit-code --quiet HEAD')
        if self._rc:
            self._dirty = True

    def _get_authors_since_release(self):
        """Get list of authors since the last full release tag (.n are ignored)"""
        self._authors_since_release = []
        self._exec('show-ref --tags')
        pat = re.compile('^refs/tags/(?P<type>[A-Za-z]*)\-(?P<rel>(?P<date>[0-9]{8})(\.(?P<suffix>[0-9]*))?)$')
        tags = []
        if self._rc == 0:
            for l in self._out.splitlines():
                commit, ref = l.split(' ')
                match = pat.match(ref)
                if match:
                    tags.append({
                        'commit': commit,
                        'ref': ref,
                        'type': match.group('type'),
                        'date': match.group('date'),
                        'suffix': match.group('suffix'),
                        'rel': match.group('rel')
                    })
            tags.sort(reverse=True, key=lambda t: t['rel'])

            date = tags[0]['date']
            if self._tag:
                match = pat.match('refs/tags/'+self._tag)
                if match:
                    date = match.group('date')

            last_rel = tags[-1]
            for t in tags[::-1]:
                if t['date'] == date:
                    break
                if not t['suffix'] and t['type'] == 'Release':
                    last_rel = t

            self._exec('shortlog -s {}..HEAD'.format(last_rel['ref']))
            if self._rc == 0:
                self._authors_since_release = sorted([l.strip().split(None, 1)[1] for l in self._out.splitlines()])

    def _get_authors(self):
        """Get all-time list of authors sorted by contributions"""
        self._authors = []
        self._exec('shortlog -sn HEAD')
        if self._rc == 0:
            self._authors = [l.strip().split(None, 1)[1] for l in self._out.splitlines()]


    def __init__(self, path = "."):
        """Initialize object instance and read repo info"""
        self._path = path
        self._exec('rev-parse --verify HEAD')
        if self._rc == 0:
            self._hash = self._out.strip(' \t\n\r')
            self._get_origin()
            self._get_time()
            self._get_tag()
            self._get_branch()
            self._get_dirty()
            self._get_ancestor()
            self._get_authors()
            self._get_authors_since_release()
        else:
            self._hash = None
            self._origin = None
            self._time = None
            self._tag = None
            self._branch = None
            self._dirty = None
            self._ancestor = None
            self._authors = []
            self._authors_since_release = []

    def path(self):
        """Return the repository path"""
        return self._path

    def origin(self, none = None):
        """Return fetch origin of the repository"""
        if self._origin == None:
            return none
        else:
            return self._origin

    def hash(self, n = 40, none = None):
        """Return hash of the HEAD commit"""
        if self._hash == None:
            return none
        else:
            return self._hash[:n]

    def time(self, format = None, none = None):
        """Return Unix or formatted time of the HEAD commit"""
        if self._time == None:
            return none
        else:
            if format == None:
                return self._time
            else:
                return datetime.utcfromtimestamp(float(self._time)).strftime(format)

    def tag(self, none = None):
        """Return git tag for the HEAD commit or given string if none"""
        if self._tag == None:
            return none
        else:
            return self._tag

    def branch(self, none = None):
        """Return git branch containing the HEAD or given string if none"""
        if self._branch == None:
            return none
        else:
            return self._branch

    def ancestor(self, n = 40, none = None):
        """Return hash of the HEAD commit"""
        if self._ancestor == None:
            return none
        else:
            return self._ancestor[:n]

    def dirty(self, dirty = "-dirty", clean = ""):
        """Return git repository dirty state or empty string"""
        if self._dirty:
            return dirty
        else:
            return clean

    def authors(self):
        return self._authors

    def authors_since_last_release(self):
        return self._authors_since_release

    def info(self):
        """Print some repository info"""
        print("path:       ", self.path())
        print("origin:     ", self.origin())
        print("Unix time:  ", self.time())
        print("commit date:", self.time("%Y%m%d"))
        print("hash:       ", self.hash())
        print("short hash: ", self.hash(8))
        print("branch:     ", self.branch())
        print("commit tag: ", self.tag(''))
        print("dirty:      ", self.dirty('yes', 'no'))
        print("ancestor:   ", self.ancestor())

def file_from_template(tpl_name, out_name, dict):
    """Create or update file from template using dictionary

    This function reads the template, performs placeholder replacement
    using the dictionary and checks if output file with such content
    already exists.  If no such file or file data is different from
    expected then it will be ovewritten with new data.  Otherwise it
    will not be updated so make will not update dependent targets.

    Example:
        # template.c:
        #    char source[] = "${OUTFILENAME}";
        #    uint32_t timestamp = ${UNIXTIME};
        #    uint32_t hash = 0x${HASH8};

        r = Repo('/path/to/git/repository')
        tpl_name = "template.c"
        out_name = "output.c"

        dictionary = dict(
            HASH8 = r.hash(8),
            UNIXTIME = r.time(),
            OUTFILENAME = out_name,
        )

        file_from_template(tpl_name, out_name, dictionary)
    """

    # Read template first
    tf = open(tpl_name, "rb", encoding="utf-8")
    tpl = tf.read()
    tf.close()

    # Replace placeholders using dictionary
    out = Template(tpl).substitute(dict)

    # Check if output file already exists
    try:
        of = open(out_name, "rb", encoding="utf-8")
    except IOError:
        # No file - create new
        of = open(out_name, "wb", encoding="utf-8")
        of.write(out)
        of.close()
    else:
        # File exists - overwite only if content is different
        inp = of.read()
        of.close()
        if inp != out:
            of = open(out_name, "wb", encoding="utf-8")
            of.write(out)
            of.close()

def sha1(file):
    """Provides C source representation of sha1 sum of file"""
    if file == None:
        return ""
    else:
        sha1 = hashlib.sha1()
        with open(file, 'rb') as f:
            for chunk in iter(lambda: f.read(8192), ''):
                sha1.update(chunk)
        hex_stream = lambda s:",".join(['0x'+hex(ord(c))[2:].zfill(2) for c in s])
        return hex_stream(sha1.digest())

def xtrim(string, suffix, length):
    """Return string+suffix concatenated and trimmed up to length characters

    This function appends suffix to the end of string and returns the result
    up to length characters. If it does not fit then the string will be
    truncated and the '+' will be put between it and the suffix.
    """

    if len(string) + len(suffix) <= length:
        return ''.join([string, suffix])
    else:
        n = length-1-len(suffix)
        assert n > 0, "length of truncated string+suffix exceeds maximum length"
        return ''.join([string[:n], '+', suffix])

def GetHashofDirs(directory, verbose=0, raw=0, what_to_hash='..*.xml$'):
  import hashlib, os, re
  SHAhash = hashlib.sha1()
  if not os.path.exists (directory):
    return -1
    
  try:
    for root, dirs, files in os.walk(directory):
      # os.walk() is unsorted.  Must make sure we process files in sorted order so
      # that the hash is stable across invocations and across OSes.
      if files:
          files.sort()

      regexp = re.compile(what_to_hash)
      
      for names in [f for f in files if regexp.match(f)]:
        if verbose == 1:
          print('Hashing', names)
        filepath = os.path.join(root,names)
        try:
          f1 = open(filepath, 'rU')
        except:
          # You can't open the file for some reason
          continue

        # Compute file hash.  Same as running "sha1sum <file>".
        f1hash = hashlib.sha1()
        while 1:
          # Read file in as little chunks
          buf = f1.read(4096)
          if not buf : break
          f1hash.update(buf)
        f1.close()

        if verbose == 1:
          print('Hash is', f1hash.hexdigest())

        # Append the hex representation of the current file's hash into the cumulative hash
        SHAhash.update(f1hash.hexdigest())

  except:
    import traceback
    # Print the stack traceback
    traceback.print_exc()
    return -2

  if verbose == 1:
      print('Final hash is', SHAhash.hexdigest())

  if raw == 1:
      return SHAhash.hexdigest()
  else:
      hex_stream = lambda s:",".join(['0x'+hex(ord(c))[2:].zfill(2) for c in s])
      return hex_stream(SHAhash.digest())

def main():
    """This utility uses git repository in the current working directory
or from the given path to extract some info about it and HEAD commit.
Then some variables in the form of ${VARIABLE} could be replaced by
collected data. Optional board type, board revision and sha1 sum
of given image file could be applied as well or will be replaced by
empty strings if not defined.

If --info option is given, some repository info will be printed to
stdout.

If --format option is given then utility prints the format string
after substitution to the standard output.

If --outfile option is given then the --template option should be
defined too. In that case the utility reads a template file, performs
variable substitution and writes the result into output file. Output
file will be overwritten only if its content differs from expected.
Otherwise it will not be touched, so make utility will not remake
dependent targets.  Multiple sets of outfile / template can be
provided in pairs.

Optional positional arguments may be used to add more dictionary
strings for replacement. Each argument has the form:
    VARIABLE=replacement
and each ${VARIABLE} reference will be replaced with replacement
string given.
    """

    parser = argparse.ArgumentParser(
        description = "Performs variable substitution in template file or string.",
        formatter_class = argparse.RawDescriptionHelpFormatter,
        epilog = "\n" + main.__doc__ + "\n")

    parser.add_argument('--path', default='.',
                        help='path to the git repository')
    parser.add_argument('--info', action='store_true',
                        help='print repository info to stdout')
    parser.add_argument('--format',
                        help='format string to print to stdout')
    parser.add_argument('--template', action='append', default=[],
                        help='name of template file')
    parser.add_argument('--outfile', action='append', default=[],
                        help='name of output file')
    parser.add_argument('--image',
                        help='name of image file for sha1 calculation')
    parser.add_argument('--type', default="",
                        help='board type, for example, 0x04 for CopterControl')
    parser.add_argument('--revision', default = "",
                        help='board revision, for example, 0x01')
    parser.add_argument('--uavodir', default = "",
                        help='uav object definition directory')
    parser.add_argument('--htmlsafe', action='store_true',
                        help='make substituted values html safe')
    parser.add_argument('pos', metavar='a=b', type=int, nargs='*',
                        help='Additional predefined words for template subst')
    args = parser.parse_args()

    # Process arguments.  No advanced error handling is here.
    # Any error will raise an exception and terminate process
    # with non-zero exit code.
    r = Repo(args.path)

    dictionary = dict(
        TEMPLATE = args.template,
        OUTFILENAME = args.outfile,
        ORIGIN = r.origin(),
        HASH = r.hash(),
        HASH8 = r.hash(8),
        ANCESTOR16 = r.ancestor(16),
        ANCESTOR = r.ancestor(),
        TAG = r.tag(''),
        TAG_OR_BRANCH = r.tag(r.branch('unreleased')),
        TAG_OR_HASH8 = r.tag(r.hash(8, 'untagged')),
        DIRTY = r.dirty(),
        FWTAG = xtrim(r.tag(r.branch('unreleased')), r.dirty(), 25),
        UNIXTIME = r.time(),
        DATE = r.time('%Y%m%d'),
        DATETIME = r.time('%Y%m%d %H:%M'),
        BOARD_TYPE = args.type,
        BOARD_REVISION = args.revision,
        SHA1 = sha1(args.image),
        UAVOSHA1TXT = GetHashofDirs(args.uavodir,verbose=0,raw=1),
        UAVOSHA1 = GetHashofDirs(args.uavodir,verbose=0,raw=0),
        AUTHORS = ' \n'.join(r.authors()),
        AUTHORS_SINCE_LAST_REL = ' \n'.join(r.authors_since_last_release())
    )

    if args.htmlsafe:
        for k in dictionary:
            dictionary[k] = escape(dictionary[k]).encode('ascii', 'xmlcharrefreplace')

    # Process positional arguments in the form of:
    #   VAR1=str1 VAR2="string 2"
    for var in args.pos:
        (key, value) = var.split('=', 1)
        dictionary[key] = value

    if args.info:
        r.info()

    if args.format != None:
        print(Template(args.format).substitute(dictionary))

    for i in range(len(args.outfile)):
        file_from_template(args.template[i], args.outfile[i], dictionary)

    return 0

if __name__ == "__main__":
    sys.exit(main())
