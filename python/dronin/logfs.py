# Copyright (C) 2016 dRonin, http://dronin.org
# Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)

from struct import Struct

config_arena_magic = 0x3bb141cf

# magic, state .. 8 byte offset in
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

slot_size = 256

class LogFSImport(dict):
    # vastly varying arena_size -- 0x1000 to 0x20000 ... though 0x20000 (aq32) is
    # probably illegit   all slot sizes 0x100 for settings

    def __init__(self, githash, contents):
        from dronin import uavo_collection

        uavo_defs = uavo_collection.UAVOCollection()

        if githash:
            uavo_defs.from_git_hash(githash)

        pos = 0

        prev_state_good = False
        in_good_arena = False

        while pos + slot_size < len(contents):
            #check if we're in a sane state

            # assumption: arena sizes are power of 2, at least 2048 bytes
            if (pos & 0x7ff) == 0:
                magic,arena_state = arena_header.unpack_from(contents, pos)

                if magic == config_arena_magic:
                    # let's assume this is good.

                    # print "Found probable arena at %x, state=%08x" % (pos, arena_state)

                    if arena_state == arena_states['ACTIVE']:
                        in_good_arena = True
                    else:
                        in_good_arena = False

                    pos += slot_size
                    prev_state_good = True

                    continue

            state, obj_id, inst_id, size = slot_header.unpack_from(contents, pos)

            prev_state_good = False

            if state == slot_states['ACTIVE']:
                if in_good_arena:
                    # print "  slot state=%08x, objid=%08x, instid=%04x, size=%d"%(state, obj_id, inst_id, size)

                    uavo_key = '{0:08x}'.format(obj_id)

                    obj = uavo_defs.get(uavo_key)

                    if obj is not None:
                        objInstance = obj.from_bytes(contents, 0, None, offset=pos + slot_header.size)

                        self[obj._name] = objInstance

                prev_state_good = True

            pos += slot_size

