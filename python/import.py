#!/usr/bin/env python

# (C) Copyright 2015 Michael Lyle; redistribution prohibited

from struct import Struct

# magic, state
config_arena_magic = 0x3bb141cf

arena_header = Struct('II')

arena_states = { 'ERASED' : 0xffffffff,
	'RESERVED'  : 0xe6e6ffff,
	'ACTIVE'    : 0xe6e66666,
	'OBSOLETE'  : 0x00000000 }

# state, obj_id, obj_inst_id, obj_size
slot_header = Struct('IIHH')

slot_states = { 'EMPTY' : 0xffffffff,
	'RESERVED'      : 0xfafaffff,
	'ACTIVE'        : 0xfafaaaaa,
	'OBSOLETE'      : 0x00000000 }

# vastly varying arena_size -- 0x1000 to 0x20000 ... though 0x20000 (aq32) is
# probably illegit   all slot sizes 0x100 for settings

arena_size = 16384
slot_size = 256

githash = '26641eef4a24f'

from dronin import uavo_collection

uavo_defs = uavo_collection.UAVOCollection()

if githash:
    uavo_defs.from_git_hash(githash)


contents = file('magma.bin', 'rb').read()

print len(contents)

config = {}

for i in range(0, len(contents), arena_size):
	magic,arena_state = arena_header.unpack_from(contents, i)
	print "arena magic=%08x state=%08x"%(magic, arena_state)

	if (arena_state != 0xe6e66666):
		continue

	print "contents"

	for i in range(i + slot_size, i + arena_size, slot_size):
# state, obj_id, obj_inst_id, obj_size
		state, obj_id, inst_id, size = slot_header.unpack_from(contents, i)

		if state != 0xfafaaaaa:
			continue

		print "  slot state=%08x, objid=%08x, instid=%04x, size=%d"%(state, obj_id, inst_id, size)


		uavo_key = '{0:08x}'.format(obj_id)

		obj = uavo_defs.get(uavo_key)

		if obj is not None:
			objInstance = obj.from_bytes(contents, 0, None, offset=i+slot_header.size)
			#print objInstance

			config[obj._name] = objInstance

for obj_name in config:
	print config[obj_name]
