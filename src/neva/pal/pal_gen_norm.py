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

import re

from pal_gen_log import print_JSON_method_error


def normalize_method_name(method):
    p = re.compile(r'(.*\b)\s+(\w+)$')
    result = p.match(method)
    if result and len(result.groups()) == 2:
        return True, result.group(1), result.group(2)
    return False, None, None


def normalize_arg(arg):
    err_ret = False, None, None, None, None
    arg_comment = None
    com_pos = arg.find("//")
    if com_pos != -1:
        p = re.compile(r'(.*\w\b)\s*//(.*)$')
        result = p.match(arg)
        if result and len(result.groups()) == 2:
            arg_comment = result.group(2)
            arg = result.group(1)
        else:
            return err_ret
    # Treat properties
    arg_props = re.findall(r'(?:\[)([^\[]*)(?:\])', arg)
    if len(arg_props) > 0:
        arg = re.sub(r'\[[^\[]*\]\s*', '', arg)
    else:
        arg_props = None
    p = re.compile(r'(.*[\w&\*])\s+(\w+\b)\s*$')
    result = p.match(arg)
    if result and len(result.groups()) == 2:
        return True, result.group(1), result.group(2), arg_comment, arg_props
    return err_ret


def normilize_args(pal_description, method, args_name):
    if not method.get(args_name):
        return
    for idx, origin_arg in enumerate(method[args_name]):
        result, arg_type, arg_name, arg_comment, arg_props = \
            normalize_arg(origin_arg)
        if (not result):
            print_JSON_method_error(pal_description['name'], method['method'],
                                    args_name, origin_arg)
        arg = {'name': arg_name, 'type': arg_type}
        if arg_comment:
            arg['comment'] = arg_comment
        if arg_props:
            for prop in arg_props:
                if prop == 'context_var' and args_name == 'in_args':
                    arg['context_var'] = 'true'
                else:
                    print_JSON_method_error(
                        pal_description['name'], method['method'],
                        args_name, origin_arg)
        method[args_name][idx] = arg
