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

import sys

# term colors
COLOR_FAIL = '\033[91m'
COLOR_OK = '\033[92m'
COLOR_WARNING = '\033[93m'
RESTORE_COLOR = '\033[0m'

# Error msg
ERR_FAILED = 'Failed to generate PAL interface'


class Unbuffered(object):
    """Disable buffering on a file object."""

    def __init__(self, stream):
        self.stream = stream

    def write(self, data):
        self.stream.write(data)
        self.stream.flush()

    def __getattr__(self, attr):
        return getattr(self.stream, attr)


def print_success(text):
    print COLOR_OK + text + RESTORE_COLOR


def print_warning(text):
    print COLOR_WARNING + text + RESTORE_COLOR


def print_error(text):
    print COLOR_FAIL + text + RESTORE_COLOR


def print_JSON_no_field_error(descr_path, field):
    print_JSON_error_field(descr_path, field, 'has no field')


def print_JSON_not_availabel_field(descr_path, field):
    print_JSON_error_field(descr_path, field, 'contains not availabel field')


def print_JSON_error_field(descr_path, field, err):
    print_error(ERR_FAILED + '. "' + descr_path +
                '" ' + err + ' "' + field + '".')
    sys.exit(1)


def print_JSON_error(interface_name, field, value):
    print_error(ERR_FAILED + '. Interface "' + interface_name +
                '" has wrong description of field "' + field + '" : ' + value)
    sys.exit(1)


def print_JSON_method_error(interface_name, method_name, field, value):
    print_error(ERR_FAILED + '. Method "' + method_name + '", of interface "' +
                interface_name + '" has wrong description: "' + field + '": ' +
                value)
    sys.exit(1)


def print_JSON_field_error_prefix(descr_path, field_name, err_type):
    prefix = ERR_FAILED + '. '
    if field_name is not None:
        prefix += ('Field "' + field_name + '" in ')
    prefix += '"' + descr_path + '" has wrong ' + err_type + ': '
    return prefix


def print_JSON_field_error_type(descr_path, field, used_type, need_type,
                                subelement=False):
    if (type(need_type) == tuple) and (len(need_type) == 1):
        need_type = need_type[0]
    prefix = print_JSON_field_error_prefix(descr_path, field, 'type')
    print_error(prefix + str(used_type) +
                '. Type has to be "' + str(need_type) + '".')
    sys.exit(1)


def print_JSON_error_field_value(descr_path, field, value, available_values):
    str_err = print_JSON_field_error_prefix(descr_path, field, 'value')
    str_err += (value + '". Available value')
    decorate = '"'
    if isinstance(available_values, tuple):
        str_err += 's'
        decorate = ''
    str_err += ' = ' + decorate + str(available_values) + decorate + '.'
    print_error(str_err)
    sys.exit(1)


def add_descr_path(descr_path, field, value):
    if len(descr_path) > 0:
        descr_path += ", "
    descr_path += (field + '->' + value)
    return descr_path


def add_descr_path_list_element(descr_path, field, index):
    if len(descr_path) > 0:
        descr_path += ", "
    descr_path += (field + '[' + str(index) + ']')
    return descr_path
