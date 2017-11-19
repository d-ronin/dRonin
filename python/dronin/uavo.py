"""
Implementation of UAV Object Types

Copyright (C) 2014-2015 Tau Labs, http://taulabs.org
Copyright (C) 2015-2017 dRonin, http://dronin.org

Licensed under the GNU LGPL version 2.1 or any later version (see COPYING.LESSER)
"""

import re
from collections import namedtuple, OrderedDict

try:
    import lxml.etree as etree
except:
    import xml.etree.ElementTree as etree

import time

try:
    from struct import Struct, calcsize
except:
    from .structshim import Struct, calcsize

RE_SPECIAL_CHARS = re.compile('[\\.\\-\\s\\+/\\(\\)]')

def flatten(lst):
    result = []
    for element in lst:
        if hasattr(element, '__iter__'):
            result.extend(flatten(element))
        else:
            result.append(element)
    return result

class UAVTupleClass():
    """ This is the prototype for a class that contains a uav object. """

    @classmethod
    def _make_to_send(cls, *args, **kwargs):
        """ Accepts all the uavo fields and creates an object.

        The object is dated as of the current time, and has the name and id
        fields set properly.
        """

        return cls(cls._name, round(time.time() * 1000), cls._id, *args, **kwargs)

    def to_bytes(self):
        """ Serializes this object into a byte stream. """
        return self._packstruct.pack(*flatten(self[3:]))

    @classmethod
    def get_size_of_data(cls):
        return cls._packstruct.size

    @classmethod
    def from_bytes(cls, data, timestamp, offset=0):
        """ Deserializes and creates an instance of this object.

         - data: the data to deserialize
         - timestamp: the timestamp to put on the object instance
         - offset: an optional index into data where to begin deserialization
        """
        unpack_field_values = cls._packstruct.unpack_from(data, offset)

        field_values = []
        field_values.append(cls._name)

        if timestamp is not None:
            field_values.append(timestamp / 1000.0)

        field_values.append(cls._id)

        # add the remaining fields.  If the thing should be nested, construct
        # an appropriate tuple.
        if not cls._flat:
            pos = 0

            for n in cls._num_subelems:
                if n == 1:
                    field_values.append(unpack_field_values[pos])
                else:
                    field_values.append(tuple(unpack_field_values[pos:pos+n]))
                pos += n

            field_values = tuple(field_values)
        else:
            # Short cut; nothing is nested
            field_values = tuple(field_values) + tuple(unpack_field_values)

        return cls._make(field_values)

    def __repr__(self):
        """ String representation of the contents """
        rep = self.__class__.__name__ + '('
        for field in self._fields:
            field_value = getattr(self, field)

            # Show ENUMs in a fancy way (string and value)
            if hasattr(self, 'ENUMR_' + field):
                if not isinstance(field_value, tuple):
                    field_value = (field_value,)
                value_str = ''
                this_enum = getattr(self, 'ENUMR_' + field)
                for ii, v in enumerate(field_value):
                    value_str += '%s(%d)' % (this_enum.get(v, "UNKNOWN"), v)
                    if ii < len(field_value) - 1:
                        value_str += ', '
                if len(field_value) > 1:
                    value_str = '(%s)' % value_str
            elif field == 'uavo_id':
                value_str = '0x%X' % field_value
            else:
                value_str = str(field_value)
            rep += '%s=%s, ' % (field, value_str)
        rep = rep[:-2] + ')'
        return rep

    def get_inst_id(self):
        return getattr(self, 'inst_id', 0)

    def elem_to_string(self, field_name, value):
        mapping = getattr(self, 'ENUMR_' + field_name, None)

        if mapping is None:
            return str(value)

        val = mapping.get(value, None)

        if val is not None:
            return mapping[value]
        else:
            return 'Unknown'

    @classmethod
    def to_xml_description(cls, as_text=False):
        if not as_text:
            return cls._canonical_xml

        # TODO: Eliminate this useless wrapping
        xml_elem = etree.Element('xml')
        xml_elem.append(cls._canonical_xml)

        try:
            return etree.tostring(xml_elem, pretty_print = True)
        except Exception:
            return etree.tostring(xml_elem)

    def to_xml_elem(self):
        munged_name = self._name[5:]
        munged_id = '0x%X' % self._id
        xmlobj = etree.Element('object', { 'name' : munged_name,
            'id' : munged_id })

        raw_dict = self._asdict()

        for field_name in sorted(raw_dict.keys()):
            if field_name == 'name': continue
            if field_name == 'time': continue
            if field_name == 'uavo_id': continue

            field_value = raw_dict[field_name]

            if isinstance(field_value, tuple) or isinstance(field_value, list):
                text_value = ','.join( [ self.elem_to_string(field_name, v) for v in field_value ] )
            else:
                text_value = self.elem_to_string(field_name, field_value)

            xmlfield = etree.SubElement(xmlobj, 'field', { 'name' : field_name, 'values' : text_value })

        return xmlobj


type_enum_map = {
    'int8'    : 0,
    'int16'   : 1,
    'int32'   : 2,
    'uint8'   : 3,
    'uint16'  : 4,
    'uint32'  : 5,
    'float'   : 6,
    'enum'    : 7,
    }

type_numpy_map = {
    'int8'    : 'int8',
    'int16'   : 'int16',
    'int32'   : 'int32',
    'uint8'   : 'uint8',
    'uint16'  : 'uint16',
    'uint32'  : 'uint32',
    'float'   : 'float',
    'enum'    : 'uint8',
    }

struct_element_map = {
    'int8'    : 'b',
    'int16'   : 'h',
    'int32'   : 'i',
    'uint8'   : 'B',
    'uint16'  : 'H',
    'uint32'  : 'I',
    'float'   : 'f',
    'enum'    : 'B',
    }

def canonicalize_defval_to_name(field, val):
    if field['type'] == 'enum':
        for f in field['options'].keys():
            if field['options'][f] == val:
                return f

        raise ValueError('missing enum thing')

    return str(val)

def canonicalize(subs, fields, is_settings):
    # TODO: later, when we accept True, output real singleinstance/settings
    # value as True / False
    attribs = {
            'name' : subs['object'].get('name'),
            'singleinstance' : subs['object'].get('singleinstance'),
            'settings' : subs['object'].get('settings')
            }

    new_tree = etree.Element('object', attribs)

    for elem in ('description', 'access', 'logging', 'telemetrygcs', 'telemetryflight'):
        subs[elem].tail=None    # Yucky to touch this directy. but meh.
        new_tree.append(subs[elem])
        pass

    for field in fields:
        if field['is_clone_of'] is not None:
            field_elem = etree.SubElement(new_tree, 'field',
                    attrib = {
                        'name' : field['name'],
                        'cloneof' : field['is_clone_of']
                    } )
        else:
            attribs = {
                    'name' : field['name'],
                    'type' : field['type']
                    }

            for attr in ( 'units', 'parent', 'limits', 'display' ):
                if field.get(attr) is not None:
                    attribs[attr] = field[attr]

            if len(field['elementnames']) == 0:
                attribs['elements'] = str(field['elements'])

            defaultvalue = None

            try:
                dvi = iter(field['defaultvalue'])

                if len(set(field['defaultvalue'])) > 1:
                    # There's several options
                    defaultvalue = ','.join([ canonicalize_defval_to_name(field, v) for v in field['defaultvalue'] ])
                elif (field['defaultvalue'][0] != 0) or is_settings or True:
                    # TODO: Would be nice to omit this more of the time, but
                    # existing UAVOgen doesn't seem to do right thing.
                    defaultvalue = canonicalize_defval_to_name(field,
                            field['defaultvalue'][0])
            except TypeError:
                #not iterable
                if (field['defaultvalue'] != 0) or is_settings or True:
                    defaultvalue = canonicalize_defval_to_name(field,
                            field['defaultvalue'])

            if defaultvalue is not None:
                attribs['defaultvalue'] = defaultvalue

            field_elem = etree.SubElement(new_tree, 'field', attribs)

            etree.SubElement(field_elem, 'description').text = field['description']

            if len(field['elementnames']):
                elementnames = etree.SubElement(field_elem, 'elementnames')

                for n in field['elementnames']:
                    name_elem = etree.SubElement(elementnames, 'elementname')
                    name_elem.text = n

            if field['type'] == 'enum':
                # omit if set is same as parent
                if field['parent'] is None or not field['options_match']:
                    options_elem = etree.SubElement(field_elem, 'options')
                    for f in field['options'].keys():
                        option_elem = etree.SubElement(options_elem, 'option')
                        option_elem.text = f

    return new_tree

# This is a very long, scary method.  It parses an XML file describing
# a UAVO and builds an implementation class.
def make_class(collection, xml_file, update_globals=True):
    fields = []

    ##### PARSE THE XML FILE INTO INTERNAL REPRESENTATIONS #####

    tree = etree.fromstring(xml_file)

    subs = {
        'object'          : tree.find('object'),
        'fields'          : tree.findall('object/field'),
        'description'     : tree.find('object/description'),
        'access'          : tree.find('object/access'),
        'logging'         : tree.find('object/logging'),
        'telemetrygcs'    : tree.find('object/telemetrygcs'),
        'telemetryflight' : tree.find('object/telemetryflight'),
        }

    name = subs['object'].get('name')
    is_single_inst = int((subs['object'].get('singleinstance').lower() == 'true'))
    is_settings = int((subs['object'].get('settings').lower() == 'true'))

    description = subs['description'].text

    ##### CONSTRUCT PROPER INTERNAL REPRESENTATION OF FIELD DATA #####
    for field in subs['fields']:
        info = {}
        # process typical attributes
        for attr in ['name', 'units', 'type', 'elements', 'elementnames',
                     'parent', 'defaultvalue', 'description', 'limits',
                     'display']:
            info[attr] = field.get(attr)
        if field.get('cloneof'):
            # this is a clone of another field, find its data
            cloneof_name = field.get('cloneof')
            for i, field in enumerate(fields):
                if field['name'] == cloneof_name:
                    clone_info = field.copy()
                    break

            # replace it with the new name
            clone_info['name'] = info['name']
            clone_info['is_clone_of'] = cloneof_name
            # use the expanded/substituted info instead of the stub
            info = clone_info
        else:
            info['is_clone_of'] = None

            if field.find('elementnames') is not None:
                # we must have one or more elementnames/elementname elements in this sub-tree
                info['elementnames'] = []
                info['elements'] = 0
                for elementname_text in [elementname.text for elementname in field.findall('elementnames/elementname')]:
                    info['elementnames'].append(elementname_text)
                    info['elements'] += 1
            elif info['elementnames'] is not None:
                # we've got an inline "elementnames" attribute
                info['elementnames'] = []
                info['elements'] = 0
                for elementname in field.get('elementnames').split(','):
                    info['elementnames'].append(RE_SPECIAL_CHARS.sub('', elementname))
                    info['elements'] += 1
            elif info['elements'] is not None:
                # we've got an inline "elements" attribute
                info['elementnames'] = []
                info['elements'] = int(field.get('elements'))
            else:
                raise ValueError("Missing element count/names")

            if info['type'] == 'enum':
                info['options'] = OrderedDict()
                if field.get('options'):
                    # we've got an inline "options" attribute
                    for ii, option_text in enumerate(field.get('options').split(',')):
                        info['options'][option_text.strip()] = ii
                else:
                    # we must have some 'option' elements in this sub-tree
                    for ii, option_text in enumerate([option.text
                            for option in field.findall('options/option')]):
                        info['options'][option_text.strip()] = ii

            # convert type string to an int
            info['type_val'] = type_enum_map[info['type']]

            # Get parent
            if info['parent'] is not None:
                parent_name, field_name = info['parent'].split('.')

                if parent_name == name:
                    for i, fld in enumerate(fields):
                        if fld['name'] == field_name:
                            enum = fld['options']
                            mapping = dict((key, value) for key, value in enum.items())
                            reverse_mapping = dict((value, key) for key, value in enum.items())
                else:
                    parent_class = collection.find_by_name(parent_name)
                    reverse_mapping = getattr(parent_class, 'ENUMR_' + field_name)
                    mapping = getattr(parent_class, 'ENUM_' + field_name)

                if len(info['options']) == 0:
                    info['options_match'] = True

                    for k, v in sorted(iter(reverse_mapping.items()), key=lambda x: x[0]):
                        info['options'][v] = k
                else:
                    info['options_match'] = len(info['options'].keys()) == len(mapping)

                    for k in info['options'].keys():
                        info['options'][k] = mapping[k]

            # Add default values
            if info['defaultvalue'] is not None:
                if info['type'] == 'enum':
                    try:
                        values = tuple(info['options'][v.strip()]
                                       for v in info['defaultvalue'].split(','))
                    except KeyError:
                        print('Invalid default value: %s.%s has no option %s' % (name, info['name'], info['defaultvalue']))
                        values = (0,)
                else:  # float or int
                    values = tuple(float(v) for v in info['defaultvalue'].split(','))
                    if info['type'] != 'float':
                        values = tuple(int(v) for v in values)

                if len(values) == 1:
                    values = values[0]
                info['defaultvalue'] = values
            else:
                # Use 0 as the default
                info['defaultvalue'] = 0

            if info['elements'] > 1 and not isinstance(info['defaultvalue'], tuple):
                info['defaultvalue'] = (info['defaultvalue'],) * info['elements']

            if field.find('description') != None:
                info['description'] = field.find('description').text

        fields.append(info)

    canonical_xml = canonicalize(subs, fields, is_settings)

    # Sort fields by size (bigger to smaller) to ensure alignment when packed
    fields.sort(key=lambda x: calcsize(struct_element_map[x['type']]), reverse = True)

    ##### CALCULATE THE APPROPRIATE UAVO ID #####
    hash_calc = UAVOHash()

    hash_calc.update_hash_string(name)
    hash_calc.update_hash_byte(is_settings)
    hash_calc.update_hash_byte(is_single_inst)

    for field in fields:
        hash_calc.update_hash_string(field['name'])
        hash_calc.update_hash_byte(field['elements'])
        hash_calc.update_hash_byte(field['type_val'])
        if field['type'] == 'enum':
            next_idx = 0
            for option, idx in field['options'].items():
                if idx != next_idx:
                    hash_calc.update_hash_byte(idx)
                hash_calc.update_hash_string(option)
                next_idx = idx + 1

    uavo_id = hash_calc.get_hash()

    ##### FORM A STRUCT TO PACK/UNPACK THIS UAVO'S CONTENT #####
    formats = []
    num_subelems = []

    is_flat = True

    if not is_single_inst:
        num_subelems.append(1)
        formats.append('H')

    # add formats for each field
    for f in fields:
        if f['elements'] != 1:
            is_flat = False

        num_subelems.append(f['elements'])

        formats.append('' + f['elements'].__str__() + struct_element_map[f['type']])

    fmt = Struct('<' + ''.join(formats))

    ##### CALCULATE THE NUMPY TYPE ASSOCIATED WITH THIS CLASS #####
    dtype  = [('name', 'S20'), ('time', 'double'), ('uavo_id', 'uint')]

    if not is_single_inst:
        dtype += ('inst_id', 'uint'),

    for f in fields:
        if f['elements'] != 1:
            dtype += [(f['name'], '(' + repr(f['elements']) + ",)" + type_numpy_map[f['type']])]
        else:
            dtype += [(f['name'], type_numpy_map[f['type']])]

    ##### DYNAMICALLY CREATE A CLASS TO CONTAIN THIS OBJECT #####
    tuple_fields = ['name', 'time', 'uavo_id']
    if not is_single_inst:
        tuple_fields.append("inst_id")

    tuple_fields.extend([f['name'] for f in fields])

    name = 'UAVO_' + name

    class tmpClass(UAVTupleClass, namedtuple(name, tuple_fields)):
        _packstruct = fmt
        _flat = is_flat
        _name = name
        _id = uavo_id
        _single = is_single_inst
        _num_subelems = num_subelems
        _dtype = dtype
        _is_settings = is_settings
        _canonical_xml = canonical_xml
        _units = {f['name'] : f['units'] for f in fields}

    # This is magic for two reasons.  First, we create the class to have
    # the proper dynamic name.  Second, we override __slots__, so that
    # child classes don't get a dict / keep all their namedtuple goodness
    tuple_class = type(name, (tmpClass,), { "__slots__" : () })

    # Set the default values for the constructor
    defaults = [name, 0, uavo_id]
    defaults.extend(f['defaultvalue'] for f in fields)
    tuple_class.__new__.__defaults__ = tuple(defaults)

    # Add enums
    for field in fields:
        if field['type'] == 'enum':
            enum = field['options']
            mapping = dict((key, value) for key, value in enum.items())
            reverse_mapping = dict((value, key) for key, value in enum.items())
            setattr(tuple_class, 'ENUM_' + field['name'], mapping)
            setattr(tuple_class, 'ENUMR_' + field['name'], reverse_mapping)

    if update_globals:
        globals()[tuple_class.__name__] = tuple_class

    return tuple_class

class UAVOHash():
    def __init__(self):
        self.hval = 0

    def update_hash_byte(self, value):
        self.hval = (self.hval ^ ((self.hval << 5) + (self.hval >> 2) + value)) & 0x0FFFFFFFF

    def update_hash_string(self, string):
        for c in string:
            self.update_hash_byte(ord(c))

    def get_hash(self):
        return self.hval & 0x0FFFFFFFE
