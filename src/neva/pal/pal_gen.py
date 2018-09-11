#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2016-2017 LG Electronics, Inc.
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

import json
import optparse
import os
import platform
import re
import sys
import traceback

from pprint import pprint
from subprocess import call

from pal_gen_check import verify_JSON
from pal_gen_log import *
from pal_gen_norm import *

# Path handling for libraries and templates
# Paths have to be normalized because Jinja uses the exact template path to
# determine the hash used in the cache filename, and we need a pre-caching step
# to be concurrency-safe. Use absolute path because __file__ is absolute if
# module is imported, and relative if executed directly.
# If paths differ between pre-caching and individual file compilation,
# the cache is regenerated, which causes a race condition and breaks concurrent
# build, since some compile processes will try to read the partially
# written cache.

# Import jinja2 from third_party/jinja2
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__),
                                             '..', '..', 'third_party')))
import jinja2


def render_template(template_env, template_filename, template_vars,
                    out_dir, filename, format=True):
    try:
        template = template_env.get_template(template_filename)

        rendered_text = template.render(template_vars)
        if not os.path.exists(out_dir):
            os.makedirs(out_dir)
        out_filename = out_dir + '/' + filename
        with open(out_filename, "w") as fd:
            fd.write(rendered_text)
        if format:
            clang_format(out_filename)
        return out_filename
    except Exception as err:
        print_error("Error at rendering template:" + template_filename + ". ")
        raise


def clang_format(filename):
    formatter = "clang-format"
    if platform.system() == "Windows":
        formatter += ".bat"
    call([formatter, "-i", "-style=chromium", filename])


def camel_to_under_score(camel_name):
    p = re.compile('(.)([A-Z]+)', re.VERBOSE)
    under_score_name = p.sub(r'\1_\2', camel_name).lower()
    return under_score_name


def methods_name_to_under_score(interface):
    if interface.get('methods'):
        for method in interface['methods']:
            method['under_score_name'] = camel_to_under_score(
                method['name'])
    if interface.get('broadcasts'):
        for broadcast in interface['broadcasts']:
            broadcast['under_score_name'] = camel_to_under_score(
                broadcast['name'])


def is_needed_IPC(interface):
    if interface.get('methods'):
        for method in interface['methods']:
            no_IPC = method.get('no_IPC')
            if no_IPC != True:  # One of possble values is None
                return True
    if interface.get('broadcasts'):
        for broadcast in interface['broadcasts']:
            no_IPC = broadcast.get('no_IPC')
            if no_IPC != True:
                return True
    return False


def generate_pal_interface(templates_path, out_dir, pal_description, options):
    try:
        template_loader = jinja2.FileSystemLoader(templates_path)
        template_env = jinja2.Environment(
            loader=template_loader,
            extensions=['jinja2.ext.loopcontrols', 'jinja2.ext.do'])
        template_env.globals.update(check_IPC_needed=is_needed_IPC)

        source_jsons = tuple()

        for interface in pal_description:
            interface['under_score_name'] = camel_to_under_score(
                interface['name'])
            methods_name_to_under_score(interface)
            source_jsons += (interface['json_file'],)

        template_vars = {"source_jsons": source_jsons,
                         "pal_description": pal_description}
        interface_template_vars = {"source_jsons": tuple(),
                                   "interface": ""}

        if options.gni:
            template = 'pal_gen.gni'
            out_path = out_dir

            render_template(template_env, template + '.jinja2',
                            template_vars, out_path, template, False)
            return True

        # Generate files that includ information about many/all interfaces
        # (1 output files for all interfaces)
        # 0 - File name, 1 - output path
        templates = (
            ('pal_gen.h', ('public',)),
            ('pal_classes_gen.h', ('public',)),
            ('pal_gen.cc', ('public',)),
            ('pal_macros_gen.h', ('ipc',)),
            ('pal_message_generator_gen.h', ('ipc',)),
            ('remote_pal_observers_gen.h', ('..', 'remote_pal_ipc',)),
            ('remote_pal_observers_gen_impl.cc', ('..', 'remote_pal_ipc',)),
            ('remote_pal_ipc_gen.cc', ('..', 'remote_pal_ipc',)),
            ('remote_pal_ipc_gen.h', ('..', 'remote_pal_ipc',)),
        )
        for template in templates:
            out_path = out_dir
            for subdir in template[1]:
                out_path = os.path.join(out_path, subdir)
            render_template(
                template_env,
                template[0] + '.jinja2',
                template_vars,
                out_path,
                template[0]
            )

        # Generate files that includ interface specific information
        # (1 output files for each interface)

        # 0 - File name, 1 - output path under out_dir,
        # 2 - if additional subdirectory with intarface under score name is
        # needed
        templates = (
            ('interface.h', ('public', 'interfaces'), False),
            ('proxy.h', ('ipc', 'renderer'), True),
            ('proxy.cc', ('ipc', 'renderer'), True),
            ('host.h', ('ipc', 'browser'), True),
            ('host.cc', ('ipc', 'browser'), True),
            ('messages.h', ('ipc',), False),
            ('remote_pal_interface_ipc.cc', ('..', 'remote_pal_ipc'), True),
            ('remote_pal_interface_ipc.h', ('..', 'remote_pal_ipc'), True),
            ('remote_pal_interface_notify_ipc.cc',
                ('..', 'remote_pal_ipc',), True),
            ('remote_pal_observer.cc', ('..', 'remote_pal_ipc'), True),
            ('remote_pal_observer.h', ('..', 'remote_pal_ipc'), True),
        )

        for interface in pal_description:
            if options.verbose:
                print "interface name = ", interface['name']
            interface_template_vars['interface'] = interface
            interface_template_vars['source_jsons'] = (interface['json_file'],)
            interface_template_vars['is_needed_IPC'] = is_needed_IPC(interface)

            for template in templates:
                out_path = out_dir
                for subdir in template[1]:
                    out_path = os.path.join(out_path, subdir)
                if template[2]:
                    out_path = os.path.join(
                        out_path, interface['under_score_name'])
                render_template(
                    template_env,
                    template[0] + ".jinja2",
                    interface_template_vars,
                    out_path,
                    interface['under_score_name'] + '_' + template[0]
                )

    except Exception, e:
        print_error(ERR_FAILED + ': ' + str(e))
        traceback.print_exc()
        return False
    return True


def main():
    sys.stdout = Unbuffered(sys.stdout)
    parser = optparse.OptionParser(description=sys.modules[__name__].__doc__)

    parser.add_option(
        '-g', '--gni',
        action="store_true", default=False,
        help="Generate GNI file." +
        " Otherwise PAL's c++ & headers files will be generated." +
        " Default: False")

    parser.add_option(
        '-i', '--input-dir',
        help='relative input directory for a JSON files.')

    parser.add_option(
        '-o', '--output-dir',
        help='relative output directory for a PAL files.')

    parser.add_option(
        '-v', '--verbose',
        action="store_true", default=False,
        help='Verbose mode. Default: False.')

    options, args = parser.parse_args()

    if not options.output_dir:
        parser.error('Output directory is not set.')

    module_path, module_filename = os.path.split(os.path.realpath(__file__))
    if (os.path.isabs(options.output_dir)):
        out_dir = options.output_dir
    else:
        out_dir = os.path.join(module_path, options.output_dir)
    try:
        if not os.path.exists(out_dir):
            os.mkdir(out_dir)
    except Exception, e:
        parser.error('Wrong output directory: ' + str(e))

    if options.input_dir:
        if (os.path.isabs(options.input_dir)):
            in_dir = options.input_dir
        else:
            in_dir = os.path.join(module_path, options.input_dir)
    else:
        in_dir = module_path

    if options.verbose:
        print_success('Input dir = ' + os.path.abspath(in_dir) +
                      ', out dir = ' + os.path.abspath(out_dir))

    files = os.listdir(in_dir)
    jsons = filter(lambda x: x.endswith('.json'), files)
    pal_descriptions = []
    for json_file_name in jsons:
        json_file_path = os.path.normpath(os.path.join(in_dir, json_file_name))
        with open(json_file_path) as json_file:
            if options.verbose:
                print_success('Load file: ' + json_file_name)

            try:
                pal_description = json.load(json_file)
            except Exception, e:
                print_error(
                    ERR_FAILED + '. Error JSON file "' + json_file_name +
                    '" loading: ' + str(e))
                sys.exit(1)

            pal_description['json_file'] = json_file_name

            verify_JSON(pal_description)

            descr_path = ""
            descr_path = add_descr_path(descr_path, 'Interface',
                                        pal_description['name'])

            if pal_description.get('methods'):
                for method in pal_description['methods']:
                    result, method['type'], method['name'] = \
                        normalize_method_name(method['method'])
                    if (not result):
                        print_JSON_error(
                            pal_description['name'], 'methods\'s method',
                            method['method'])
                    normilize_args(pal_description, method, 'in_args')
                    normilize_args(pal_description, method, 'out_args')
            if pal_description.get('broadcasts'):
                for method_num, method in enumerate(
                        pal_description['broadcasts']):
                    if method.get('method') is None:
                        descr_path_m = add_descr_path_list_element(
                            descr_path, 'broadcasts', method_num)
                        print_JSON_no_field_error(descr_path_m, 'method')

                    result, method['type'], method['name'] = \
                        normalize_method_name(method['method'])
                    if (not result):
                        print_JSON_error(
                            pal_description['name'], 'broadcast\'s method',
                            broadcast['method'])
                    normilize_args(pal_description, method, 'args')

            if options.verbose:
                print_success('Interface: ' + pal_description['name'])
            pal_descriptions.append(pal_description)
    if options.verbose:
        pprint(pal_descriptions)

    templates_dir = os.path.normpath(os.path.join(module_path, 'templates'))
    if generate_pal_interface(
            templates_dir, out_dir, pal_descriptions, options):
        if options.verbose:
            print_success('#####################################')
            print_success('#####       Success!!!       ########')
            print_success('#####################################')
    else:
        print_error('generation PAL interface failed')
        return 1

    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.stderr.write('interrupted\n')
        sys.exit(1)
else:
    print "__name__ = ", __name__
