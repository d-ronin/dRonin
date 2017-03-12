#!/usr/bin/env python

from __future__ import print_function

import sys
import os
import platform
import subprocess
import logging
import argparse
import shutil
import errno
from logging import debug, info, warning, error, critical, exception


def recursive_file_gen(mydir):
    for root, dirs, files in os.walk(mydir):
        for file in files:
            yield os.path.join(root, file)

def mkdir_if_not_exist(path):
		if not os.path.isdir(path):
			if os.path.exists(path):
				raise OSError("Cannot create directory, file with same name exists", errno=errno.EEXISTS, filename=path)
			debug("Creating directory: " + path)
			os.makedirs(path)

def set_logging(file = None):
	rootLogger = logging.getLogger()
	rootLogger.setLevel(logging.DEBUG)

	if file:
		fileHandler = logging.FileHandler(file)
		fileHandler.setLevel(logging.DEBUG)
		rootLogger.addHandler(fileHandler)

	consoleHandler = logging.StreamHandler()
	consoleHandler.setLevel(logging.INFO)
	rootLogger.addHandler(consoleHandler)

class GenericSymbolDump(object):
	def __init__(self):
		raise NotImplementedError("Use OS-specific child classes")

	def init(self, path):
		self.root_path = path
		self.targets = {}
		self.breakpad_path = os.path.realpath("./")
		self.debug_path = os.path.realpath("./")
		self.dump_syms = "dump_syms" # let's hope it's on path if this stays
		self.output_map = False
		self.map_file = None

	def set_dump_syms_path(self, path):
		self.dump_syms = os.path.realpath(path)

	def generate_map(self, enable):
		self.output_map = bool(enable)

	def enumerate_targets(self):
		for file in recursive_file_gen(self.root_path):
			# might have to move down into os specific subclass if we ever package outside mingw
			sig = subprocess.check_output(['file', '-b', file]).split(" ")
			if self.is_object_file(file, sig):
				info("Found object file: " + os.path.relpath(file, self.root_path))
				self.add_target(file)
		return len(self.targets)

	def add_target(self, file):
		real_path = os.path.realpath(file)
		# this logic causes us to prefer files with matching debug symbols in the case
		# of multiple symlinked copies (qmake does this with libraries)
		# while also catching things without debug symbols so we can still get public symbols
		# (except on Windows where that doesn't work)
		if real_path not in self.targets:
			self.targets[real_path] = {"source_file": file, "debug_symbols": None}
		elif os.path.islink(file) and os.path.basename(file) == os.path.basename(real_path):
			return
		if self.targets[real_path]["debug_symbols"] == None:
			self.targets[real_path]["source_file"] = file
			self.targets[real_path]["debug_symbols"] = self.find_debug_symbols(file)

	def set_breakpad_out_dir(self, path):
		self.breakpad_path = os.path.realpath(path)

	def set_debug_out_dir(self, path):
		self.debug_path = os.path.realpath(path)

	def generate_breakpad_symbols(self):
		info("Using dump_syms: " + self.dump_syms)
		if self.output_map:
			mkdir_if_not_exist(self.breakpad_path)
			with open(os.path.join(self.breakpad_path, 'breakpad.map'), 'w') as fp:
				print('binary|uuid', file=fp)
				self._generate_breakpad_symbols(fp)
		else:
			self._generate_breakpad_symbols()

	def _generate_breakpad_symbols(self, mapfile = None):
		for t in self.targets:
			info("Processing breakpad symbols for: " + os.path.basename(self.targets[t]["source_file"]))
			cmd = self.get_dump_syms_cmd(self.targets[t]["source_file"], self.targets[t]["debug_symbols"])
			if cmd:
				self.process_dump_syms(cmd, self.targets[t]["source_file"], mapfile)

	def process_dump_syms(self, cmd, objpath, mapfile = None):
		syms = ""
		try:
			proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
		except OSError as e:
			error("Cannot execute dump_syms, is it installed? (try make breakpad_install)")
			error(str(e))
			return
		syms, err = proc.communicate()

		if proc.returncode != 0:
			warning("Invalid symbols [{0}]: {1}".format(proc.returncode, objpath))
			warning("Command: " + " ".join(cmd))
			warning(err)
			return
		elif err:
			debug(err)

		lines = syms.splitlines()
		assert(len(lines) > 0)
		module = lines[0].split(" ")
		if len(module) != 5 or module[0] != "MODULE":
			warning("Invalid symbol dump: {1}".format(cmd[-1]))
			return
		osys, arch, uuid, name = module[1:]

		out_path = os.path.join(self.breakpad_path, name, uuid, self.get_symbol_name(name))
		mkdir_if_not_exist(os.path.dirname(out_path))
		with open(out_path, "wb") as fp:
			fp.write(syms)

		if mapfile:
			self._add_map_entry(mapfile, os.path.relpath(objpath, self.root_path), uuid)

	def move_debug_syms(self):
		for t in self.targets:
			if self.targets[t]["debug_symbols"] != None:
				rel = os.path.relpath(self.targets[t]["debug_symbols"], self.root_path)
				new_path = os.path.join(self.debug_path, rel)
				info("Moving {0}".format(rel))
				mkdir_if_not_exist(os.path.dirname(new_path))
				shutil.move(self.targets[t]["debug_symbols"], new_path)

	def _add_map_entry(self, fp, bin_name, uuid):
		print('{}|{}'.format(bin_name, uuid), file=fp)

class MacOSSymbolDump(GenericSymbolDump):
	def __init__(self, path):
		self.init(path)

	def is_object_file(self, file, sig):
		if len(sig) < 3:
			return False
		# avoids catching DWARF info inside .dSYM directories
		return sig[0] == "Mach-O" and sig[2] != "dSYM"

	def find_debug_symbols(self, file):
		# TODO: make this prettier
		dsym = os.path.join(self.debug_path, "dRonin-GCS.app", os.path.relpath(file, self.root_path) + ".dSYM")
		debug("Looking for .dSYM: " + dsym)
		if os.path.isdir(dsym):
			info("Found symbol file: " + os.path.relpath(dsym, self.debug_path))
			return os.path.realpath(dsym)
		return None

	def get_dump_syms_cmd(self, objpath, sympath = None):
		cmd = [self.dump_syms]
		if sympath != None:
			cmd.append('-g')
			cmd.append(sympath)
		cmd.append(objpath)
		return cmd

	def get_symbol_name(self, objname):
		return objname + '.sym'

class WindowsSymbolDump(GenericSymbolDump):
	def __init__(self, path):
		self.init(path)

	def is_object_file(self, file, sig):
		if len(sig) < 3:
			return False
		# windows dump_syms can only dump from pdbs, so just find them
		return sig[0] == "MSVC" and sig[2] == "database"

	def find_debug_symbols(self, file):
		name, ext = os.path.splitext(file)
		pdb = name + ".pdb"
		if os.path.isfile(pdb):
			return pdb
		return None

	def get_dump_syms_cmd(self, objpath, sympath = None):
		# windows dumper can't dump public symbols from shared libs, need pdb to get anything
		if not sympath:
			debug("Skipping: {0} (no .pdb)".format(os.path.relpath(objpath, self.root_path)))
			return None
		cmd = [self.dump_syms]
		cmd.append(sympath)
		return cmd

	def get_symbol_name(self, objname):
		name, ext = os.path.splitext(objname)
		return name + '.sym'


class LinuxSymbolDump(GenericSymbolDump):
	def __init__(self, path):
		self.init(path)

	def is_object_file(self, file, sig):
		# sadly can't easily distinguish debug symbols except by filename
		name, ext = os.path.splitext(file)
		if ext == ".debug":
			return False
		return sig[0].startswith("ELF")

	def find_debug_symbols(self, file):
		dsym = file + ".debug"
		if os.path.isfile(dsym):
			info("Found symbol file: " + os.path.relpath(dsym, self.root_path))
			return os.path.realpath(dsym)
		return None

	def get_dump_syms_cmd(self, objpath, sympath = None):
		cmd = [self.dump_syms]
		cmd.append(objpath)
		if sympath != None:
			cmd.append(os.path.dirname(sympath))
		return cmd

	def get_symbol_name(self, objname):
		return objname + '.sym'


def main():
	parser = argparse.ArgumentParser()
	parser.add_argument("-m", "--map", action='count', help="Generate mapfile with object -> UUID mapping")
	parser.add_argument("-p", "--pkgpath", default="package", nargs="?", help="Package directory, -symbols will be appended")
	parser.add_argument("-d", "--dumpsyms", nargs="?", help="dump_syms binary to use when generating breakpad symbols")
	parser.add_argument("-l", "--logfile", nargs="?", help="Logfile path to output debug information")
	parser.add_argument("binpath", help="Path to search for binaries and symbols")
	args = parser.parse_args()

	set_logging(args.logfile)
	debug("args: " + str(args))

	# append -symbols suffix to package directory name so we're outside the package
	symbol_dir = os.path.normpath(args.pkgpath) + "-symbols"
	breakpad_dir = symbol_dir + "/breakpad"
	debug_dir = symbol_dir + "/debug"

	if platform.system() == "Linux":
		dumper = LinuxSymbolDump(args.binpath)
	elif platform.system() == "Darwin":
		dumper = MacOSSymbolDump(args.binpath)
	elif platform.system() == "Windows":
		# sadly have to deal with MinGW builds for now, ick
		if 'USE_MSVC' in os.environ and os.environ['USE_MSVC'] == 'NO':
			info('MinGW build, not collecting symbols (Note: MinGW support is deprecated)')
			return 0
		dumper = WindowsSymbolDump(args.binpath)
	else:
		raise NotImplementedError("Unsupported OS")

	# dumpers expect these to exist
	mkdir_if_not_exist(breakpad_dir)
	mkdir_if_not_exist(debug_dir)
	dumper.set_debug_out_dir(debug_dir)
	dumper.set_breakpad_out_dir(breakpad_dir)

	if dumper.enumerate_targets() == 0:
		error("No object files found!")
		return 1

	# Don't want to get mixed up with old symbols
	if os.path.exists(breakpad_dir):
		info("Removing directory: " + breakpad_dir)
		shutil.rmtree(breakpad_dir, True)

	if args.dumpsyms:
		dumper.set_dump_syms_path(args.dumpsyms)
	dumper.generate_map(args.map > 0)
	dumper.generate_breakpad_symbols()
	# this is done earlier in build process to work around macdeployqt bug
	if platform.system() != "Darwin":
		dumper.move_debug_syms()

	return 0

if __name__ == "__main__":
	sys.exit(main())
