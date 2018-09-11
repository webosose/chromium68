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
import unittest

import tst_pal_gen_conf as conf


class CheckGeneratedCodeClass(unittest.TestCase):
    work_path = ''
    gen_out_path = ''
    gen_in_path = ''
    gen_path = ''
    script_name = '../pal_gen.py'

    JSON_SUNNY_CASES_DIR = 'sunny_cases'
    EXPECTED_OTPUT_PATH_POSTFIX = '_expected_output'
    DIFF_TOOL = 'diff -ur'

    GNI_FILE_NAME = 'pal_gen.gni'
    EXP_OUT_GNI_FILE_NAME = GNI_FILE_NAME + '_eo'

    @staticmethod
    def get_work_paths():
        work_path = os.path.abspath(os.path.dirname(__file__))
        gen_out_path = os.path.join(work_path, 'tst_gen_out')
        gen_path = os.path.abspath(os.path.join(work_path, '..'))
        return (
            work_path,
            gen_out_path,
            os.path.join(work_path, 'tst_input'),
            gen_path,
            os.path.join(gen_path, 'pal_gen.py'),
            os.path.join(gen_out_path, 'gen_out.txt')
        )

    def setUp(self):
        self.work_path, self.gen_out_path, self.gen_in_path, \
            self.gen_path, self.script_name, self.log_file = \
            self.get_work_paths()

    @staticmethod
    def clear_dir(dir_name, create_new=True):
        if os.path.exists(dir_name):
            shutil.rmtree(dir_name)
        if create_new:
            os.makedirs(dir_name)

    def clear_dirs(self, create_new=True):
        self.clear_dir(self.gen_in_path, create_new)
        self.clear_dir(self.gen_out_path, create_new)

    def show_log(self):
        cmd = 'cat ' + self.log_file
        return os.system(cmd)

    def execute_pal_gen(self, JSON_file_name):
        self.clear_dirs()
        shutil.copy(
            os.path.join(self.work_path, self.JSON_SUNNY_CASES_DIR,
                         JSON_file_name) +
            '.json', self.gen_in_path)
        pal_gen_out_path = os.path.join(self.gen_out_path, 'pal')
        cmd = 'python ' + self.script_name + ' -o ' + pal_gen_out_path \
            + ' -i ' + self.gen_in_path + ' >> ' + self.log_file
        return os.system(cmd)

    def execute_pal_gen_gn(self, JSON_file_names):
        self.clear_dirs()
        for JSON_file_name in JSON_file_names:
            shutil.copy(
                os.path.join(self.work_path, self.JSON_SUNNY_CASES_DIR,
                             JSON_file_name) +
                '.json', self.gen_in_path)
        cmd = 'python ' + self.script_name + ' -o ' + self.gen_out_path \
            + ' -i ' + self.gen_in_path + ' -g >> ' + self.log_file
        return os.system(cmd)

    def compare_gen_dir(self, interface_file_name, gen_dir):
        exp_out_path = os.path.join(
            self.work_path, self.JSON_SUNNY_CASES_DIR,
            interface_file_name + self.EXPECTED_OTPUT_PATH_POSTFIX, gen_dir)

        compare_cmd = (self.DIFF_TOOL + ' ' + exp_out_path +
                       ' ' + os.path.join(self.gen_out_path, gen_dir))
        return os.system(compare_cmd)

    def compare_generated_code_set(self, interface_file_name):
        gen_dirs = ('pal', 'remote_pal_ipc')
        for gen_dir in gen_dirs:
            self.assertEqual(0, self.compare_gen_dir(interface_file_name,
                                                     gen_dir))

    def compare_generated_gn(self):
        origin_gn = os.path.join(self.work_path, self.JSON_SUNNY_CASES_DIR,
                                 self.EXP_OUT_GNI_FILE_NAME)
        gen_gn = os.path.join(self.gen_out_path, self.GNI_FILE_NAME)
        compare_cmd = self.DIFF_TOOL + ' ' + origin_gn + ' ' + gen_gn
        self.assertEqual(
            0, os.system(compare_cmd), 'Wrong GNI file was generated')

    @staticmethod
    def get_test_interfaces(interfaces_list):
        tst_cases = [
            'test1',
            'test2',
            'test3',
            'test4',
            'test5'
        ]
        for tst in tst_cases:
            interfaces_list.append(tst)

    def test_pal_code_generation(self):
        test_cases = []
        self.get_test_interfaces(test_cases)
        for test_case in test_cases:
            ret = self.execute_pal_gen(test_case)
            if ret != 0:
                self.show_log()
                self.assertEqual(0, ret, 'Error run "pal_gen" script for "' +
                                 test_case + '.json"')
            self.compare_generated_code_set(test_case)

    def test_pal_GN_generation(self):
        test_cases = []
        self.get_test_interfaces(test_cases)
        ret = self.execute_pal_gen_gn(test_cases)
        if ret != 0:
            self.show_log()
            self.assertEqual(0, ret, 'Error run "pal_gen -g" script for "')
        self.compare_generated_gn()

    def tearDown(self):
        if conf.DEBUG == 0:
            self.clear_dirs(False)
        else:
            pass


if __name__ == '__main__':
    unittest.main()
