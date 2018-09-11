#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2016 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from pal_gen_log import *


def check_availabel_fields(parent, availabel, descr_path):
    for field in parent:
        if field not in availabel:
            print_JSON_not_availabel_field(descr_path, field)
    return True


def check_field_exist(parent, field_name, descr_path, needed=False):
    if parent.get(field_name) is None:
        if not needed:
            return 0
        print_JSON_no_field_error(descr_path, field_name)
        sys.exit(1)
    return 1


def check_simple_field(parent, field_name, descr_path, types, needed=False):
    if check_field_exist(parent, field_name, descr_path, needed) == 0:
        return False
    if not isinstance(parent[field_name], types):
        print_JSON_field_error_type(descr_path, field_name,
                                    type(parent[field_name]), types)
    return True


def check_enum_field(descr_path, value, available_values):
    if value not in available_values:
        print_JSON_error_field_value(descr_path, None, value, available_values)
    return True


def check_list_field(list_elem, descr_path, types, element_num):
    if not isinstance(list_elem, types):
        descr_path_m = add_descr_path_list_element(descr_path, 'element',
                                                   element_num)
        print_JSON_field_error_type(descr_path_m, None, type(list_elem), types)


class BaseType:

    def __init__(self, field_type):
        self.field_type = field_type

    def check_type(self, field_type):
        return isinstance(field_type, self.field_type)

    def get_type(self):
        return self.field_type

    def check(self, pal_description, descr_path):
        return True


class SeveralTypes:

    def __init__(self):
        self.type_elements = tuple()

    def add_type(self, element_type):
        if (element_type.__class__.__name__ == 'SeveralTypes'):
            self.type_elements += element_type.get_type_elements()
        elif isinstance(element_type, tuple):
            self.type_elements += element_type
        else:
            self.type_elements += (element_type,)

    def check_type(self, field_type):
        for element_type in self.type_elements:
            if element_type.check_type(field_type):
                return element_type
        return None

    def get_type_elements(self):
        return self.type_elements

    def get_availabel_types(self):
        ret = tuple()
        for element_type in self.type_elements:
            types = element_type.get_type()
            if isinstance(types, tuple):
                ret += types
            else:
                ret += (types,)
        return ret


class BoolType(BaseType):

    def __init__(self):
        BaseType.__init__(self, bool)


class StringType(BaseType):

    def __init__(self):
        BaseType.__init__(self, basestring)


class EnumType(StringType):

    def __init__(self, availabel_values):
        StringType.__init__(self)
        self.__availabel_values = availabel_values

    def check(self, value, descr_path):
        check_enum_field(descr_path.get_full_description_path(), value,
                         self.__availabel_values)
        return True


class DictElement(SeveralTypes):

    def __init__(self, name, optional, types):
        SeveralTypes.__init__(self)
        self.__name = name
        self.__optional = optional
        self.add_type(types)

    def get_name(self):
        return self.__name

    def check_elem(self, pal_description, descr_path_txt):
        availabel_types = self.get_availabel_types()
        if not check_simple_field(pal_description, self.__name, descr_path_txt,
                                  availabel_types, not self.__optional):
            return False
        checked_el = pal_description[self.__name]
        type_el = self.check_type(checked_el)
        dp_txt = descr_path_txt
        dp_txt = add_descr_path(dp_txt, 'field', self.__name)
        dp = DescriptionPath(dp_txt)
        return type_el.check(checked_el, dp)


class DictType(BaseType):

    def __init__(self):
        BaseType.__init__(self, dict)
        self.__dict_elements = []
        self.__name_element = None

    def add_dict_field(self, name, optional, element_type,
                       is_name=False, alias=None):
        element = DictElement(name, optional, element_type)
        self.__dict_elements.append(element)
        if is_name:
            self.__name_element = element
            if alias is None:
                alias = name
            self.__alias = alias

    def check(self, pal_description, descr_path):
        dp_txt = descr_path.get_full_description_path()
        if self.__name_element is not None:
            if self.__name_element.check_elem(pal_description, dp_txt):
                descr_path.replace_temporary(
                    pal_description, self.__name_element.get_name(),
                    self.__alias)
                dp_txt = descr_path.get_full_description_path()

        availabel_fields = tuple()
        for dict_element in self.__dict_elements:
            availabel_fields += (dict_element.get_name(),)
        check_availabel_fields(pal_description, availabel_fields, dp_txt)

        for req_element in self.__dict_elements:
            req_element.check_elem(pal_description, dp_txt)


class ListElement(SeveralTypes):

    def __init__(self, type_elements):
        SeveralTypes.__init__(self)
        self.add_type(type_elements)

    def check_elem(self, checked_el, descr_path_txt, index):
        check_list_field(checked_el, descr_path_txt,
                         self.get_availabel_types(), index)
        type_el = self.check_type(checked_el)
        dp_txt = add_descr_path_list_element(descr_path_txt, 'element', index)
        dp = DescriptionPath(dp_txt)
        return type_el.check(checked_el, dp)


class ListType(BaseType):

    def __init__(self, type_elements):
        BaseType.__init__(self, list)
        self.__list_element = ListElement(type_elements)

    def check(self, pal_description, descr_path):
        dp_txt = descr_path.get_full_description_path()
        for element_num, list_checked_el in enumerate(pal_description):
            self.__list_element.check_elem(
                list_checked_el, dp_txt, element_num)


class DescriptionPath:

    def __init__(self, general_path,
                 temporary_part=None, replace_prefix=None):
        self.__general_path = general_path
        self.__temporary_part = temporary_part
        self.__replace_prefix = replace_prefix

        self.__full_path = general_path
        if temporary_part is not None:
            self.__full_path += temporary_part

    def get_full_description_path(self):
        return self.__full_path

    def replace_temporary(self, pal_description, name, alias=None):
        if alias is None:
            alias = name
        self.__full_path = self.__general_path
        if self.__replace_prefix is not None:
            self.__full_path += self.__replace_prefix
        self.__full_path = \
            add_descr_path(self.__full_path, alias, pal_description[name])

    def get_description_path_copy(self):
        return (self.__general_path, self.__temporary_part,
                self.__replace_prefix)


def verify_JSON(pal_description):
    descr = DictType()
    descr.add_dict_field('name', False, StringType(), True, 'Interface')
    comment_type = SeveralTypes()
    comment_type.add_type(StringType())
    comment_type.add_type(ListType(StringType()))
    descr.add_dict_field('comment', True, comment_type)

    descr.add_dict_field('json_file', False, StringType())

    methods_element = DictType()
    methods_element.add_dict_field('method', False, StringType(), True)
    methods_element.add_dict_field(
        'in_args', False, ListType(StringType()), False)
    methods_element.add_dict_field(
        'out_args', False, ListType(StringType()), False)
    methods_element.add_dict_field('comment', True, comment_type)
    methods_element.add_dict_field(
        'return', True, EnumType(('sync', 'sync_delayed', 'async')),
        False)
    methods_element.add_dict_field('pal_ret', True, BoolType(), False)
    methods_element.add_dict_field('no_IPC', True, BoolType(), False)
    methods_element.add_dict_field('const', True, BoolType(), False)

    methods = ListType(methods_element)
    descr.add_dict_field('methods', True, methods)

    broadcasts_element = DictType()
    broadcasts_element.add_dict_field('method', False, StringType(), True)
    broadcasts_element.add_dict_field(
        'args', False, ListType(StringType()), False)
    broadcasts_element.add_dict_field('no_IPC', True, BoolType(), False)
    broadcasts = ListType(broadcasts_element)
    descr.add_dict_field('broadcasts', True, broadcasts)

    descr_path_txt = ""
    descr_path_txt = add_descr_path(descr_path_txt,
                                    'File', pal_description['json_file'])
    descr_path = DescriptionPath("", descr_path_txt)

    descr.check(pal_description, descr_path)
