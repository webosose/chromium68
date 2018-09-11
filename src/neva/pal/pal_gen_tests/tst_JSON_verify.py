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

import os
import shutil
import sys
import unittest

import tst_pal_gen_conf as conf

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from pal_gen_log import COLOR_FAIL, COLOR_WARNING, RESTORE_COLOR, COLOR_OK


class ValidateJSON_StructureClass(unittest.TestCase):
    work_path = ''
    gen_out_path = ''
    gen_in_path = ''
    gen_path = ''
    script_name = '../pal_gen.py'
    JSON_ERROR_CASES_DIR = 'error_cases'

    def setUp(self):
        self.work_path = os.path.abspath(os.path.dirname(__file__))
        self.gen_out_path = os.path.join(self.work_path, 'tst_gen_out')
        self.gen_in_path = os.path.join(self.work_path, 'tst_input')
        self.gen_path = os.path.abspath(os.path.join(self.work_path, '..'))
        self.script_name = os.path.join(self.gen_path, 'pal_gen.py')
        self.log_file = os.path.join(self.gen_out_path, 'gen_out.txt')

    def clear_dir(self, dir_name, create_new=True):
        if os.path.exists(dir_name):
            shutil.rmtree(dir_name)
        if create_new:
            os.makedirs(dir_name)

    def clear_dirs(self, create_new=True):
        self.clear_dir(self.gen_in_path, create_new)
        self.clear_dir(self.gen_out_path, create_new)

    def execute_pal_gen(self, JSON_file_name):
        self.clear_dirs()
        shutil.copy(
            os.path.join(self.work_path, self.JSON_ERROR_CASES_DIR,
                         JSON_file_name),
            self.gen_in_path)
        cmd = 'python ' + self.script_name + ' -o ' + self.gen_out_path \
            + ' -i ' + self.gen_in_path + ' >> ' + self.log_file

        return os.system(cmd)

    def test_interface_fields_verification(self):
        prefix = COLOR_FAIL + 'Failed to generate PAL interface. '
        postfix = RESTORE_COLOR + '\n'

        tst_cases = [
            ('tst_interface_name.json',
                ['"File->tst_interface_name.json" has no field "name".']),
            ('tst_error_field_in_interface.json',
                ['"Interface->ErrorFieldInInterface" contains not availabel field "error_field".']),
            ('tst_error_name_field_type.json',
                ['Field "name" in "File->tst_error_name_field_type.json" has wrong type: <type \'list\'>. Type has to be "<type \'basestring\'>".']),
            # ('tst_no_interface_methods_field.json',
            #     [''])
            ('tst_error_methods_field_type.json',
                ['Field "methods" in "Interface->ErrorMethodsFieldType" has wrong type: <type \'unicode\'>. Type has to be "<type \'list\'>".']),
            ('tst_error_broadcasts_field_type.json',
                ['Field "broadcasts" in "Interface->ErrorBroadcastsFieldType" has wrong type: <type \'unicode\'>. Type has to be "<type \'list\'>".']),
            ('tst_error_comment_field_type.json',
                ['Field "comment" in "Interface->ErrorCommentFieldType" has wrong type: <type \'dict\'>. Type has to be "(<type \'basestring\'>, <type \'list\'>)".']),
            ('tst_no_methods_method_field.json',
                ['"Interface->NoMethodsMethodField, field->methods, element[0]" has no field "method".']),
            ('tst_methods_method_wrong_type.json',
                ['Field "method" in "Interface->MethodsMethodWrongType, field->methods, element[0]" has wrong type: <type \'list\'>. Type has to be "<type \'basestring\'>".']),
            ('tst_no_methods_in_args_field.json',
                ['"Interface->NoMethodsInArgsField, field->methods, element[0], method->std::string GetValue" has no field "in_args".']),
            ('tst_methods_in_args_wrong_type.json',
                ['Field "in_args" in "Interface->MethodsInArgsWrongType, field->methods, element[0], method->std::string GetValue" has wrong type: <type \'unicode\'>. Type has to be "<type \'list\'>".']),
            ('tst_no_methods_out_args_field.json',
                ['"Interface->NoMethodsOutArgsField, field->methods, element[0], method->std::string GetValue" has no field "out_args".']),
            ('tst_methods_out_args_wrong_type.json',
                ['Field "out_args" in "Interface->MethodsOutArgsWrongType, field->methods, element[0], method->std::string GetValue" has wrong type: <type \'unicode\'>. Type has to be "<type \'list\'>".']),
            ('tst_methods_comment_wrong_type.json',
                ['Field "comment" in "Interface->MethodsCommentWrongType, field->methods, element[0], method->std::string GetValue" has wrong type: <type \'int\'>. Type has to be "(<type \'basestring\'>, <type \'list\'>)".']),
            ('tst_methods_return_wrong_type.json',
                ['Field "return" in "Interface->MethodsReturnWrongType, field->methods, element[0], method->std::string GetValue" has wrong type: <type \'list\'>. Type has to be "<type \'basestring\'>".']),
            ('tst_methods_return_wrong_value.json',
                ['"Interface->MethodsReturnWrongValue, field->methods, element[0], method->std::string GetValue, field->return" has wrong value: error_value". Available values = (\'sync\', \'sync_delayed\', \'async\').']),
            ('tst_methods_pal_ret_wrong_type.json',
                ['Field "pal_ret" in "Interface->MethodsPalRetWrongType, field->methods, element[0], method->std::string GetValue" has wrong type: <type \'int\'>. Type has to be "<type \'bool\'>".']),
            ('tst_methods_no_ipc_wrong_type.json',
                ['Field "no_IPC" in "Interface->MethodsNoIpcWrongType, field->methods, element[0], method->std::string GetValue" has wrong type: <type \'unicode\'>. Type has to be "<type \'bool\'>".']),
            ('tst_methods_in_args_wrong_element_type.json',
                ['"Interface->MethodsInArgsWrongElementType, field->methods, element[0], method->std::string GetValue, field->in_args, element[1]" has wrong type: <type \'int\'>. Type has to be "<type \'basestring\'>".']),
            ('tst_methods_in_args_element_no_ret_type.json',
                ['Method "std::string GetValue", of interface "MethodsInArgsElementNoRetType" has wrong description: "in_args": arg1     // Any value']),
            ('tst_methods_in_args_element_no_name.json',
                ['Method "std::string GetValue", of interface "MethodsInArgsElementNoName" has wrong description: "in_args": std::string      // Any value']),
            ('tst_methods_out_args_wrong_element_type.json',
                ['"Interface->MethodsOutArgsWrongElementType, field->methods, element[0], method->std::string GetValue, field->out_args, element[1]" has wrong type: <type \'list\'>. Type has to be "<type \'basestring\'>".']),
            ('tst_no_broadcasts_method_field.json',
                ['"Interface->NoBroadcastsMethodField, field->broadcasts, element[0]" has no field "method".']),
            ('tst_broadcasts_illegal_field.json',
                ['"Interface->BroadcastsIllegalField, field->broadcasts, element[0], method->void SampleUpdate" contains not availabel field "field".']),
            ('tst_broadcasts_args_wrong_type.json',
                ['Field "args" in "Interface->NoBroadcastsMethodField, field->broadcasts, element[0], method->void SampleUpdate" has wrong type: <type \'unicode\'>. Type has to be "<type \'list\'>".']),
        ]

        for test_case in tst_cases:
            ret = self.execute_pal_gen(test_case[0])
            self.assertNotEqual(0, ret, 'No error message for ' + test_case[0])
            tst_err_str = test_case[1]
            tst_err_str[0] = prefix + tst_err_str[0]
            tst_err_len = len(tst_err_str)
            tst_err_str[tst_err_len - 1] += postfix

            with open(self.log_file) as f:
                err_txt = f.readlines()
            self.assertEqual(len(err_txt), tst_err_len,
                             'No error log for ' + test_case[0])
            for n in range(tst_err_len):
                self.assertEqual(err_txt[n], tst_err_str[n])

    def tearDown(self):
        if conf.DEBUG == 0:
            self.clear_dirs(False)
        else:
            pass


if __name__ == '__main__':
    unittest.main()
