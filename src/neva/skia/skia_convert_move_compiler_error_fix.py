#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2018 LG Electronics, Inc.
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
import subprocess
import sys

os.path.dirname(os.path.realpath(__file__))

commit_msg = ("[skia] Fix of convert_move internal compiler error\n"
              "\n"
              ":Release Notes:\n"
              "Fix of internal compiler error: in convert_move, at expr.c:299\n"
              "\n"
              ":Detailed Notes:\n"
              "Fix of internal compile error:\n"
              "v = (U16)( ((v<<8)|(v>>8)) & 0xffff ) => internal compiler error: in convert_move, at expr.c:299\n"
              "https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86166\n"
              "\n"
              ":Test Scenario:\n"
              "WebOS configure is done without errors\n"
              "\n"
              ":QA Notes:\n"
              "\n"
              ":Issues Addressed:\n"
              "[NEVA-2553] Continuous upgrade upto verion 68 from 65 \n"
             )

def main():
    # First check chromium version information.
    # This stage is needed for chromium68. So don't apply to other versions.
    chrome_version_str = subprocess.check_output("awk -F'MAJOR=' 'NR==1{print $2}' ./src/chrome/VERSION", shell=True)
    try:
        if not chrome_version_str:
            sys.stderr.write('Cannot get version information from src/chrome/VERSION.\n')
            sys.exit(1)

        if int(chrome_version_str) >= 69:
            sys.stderr.write('Check that this stage is still needed.\n')
            sys.exit(1)
    except Exception as e:
            sys.stderr.write('Unexpected error.\n')
            sys.exit(1)


    # v = (U16)( ((v<<8)|(v>>8)) & 0xffff ) => internal compiler error: in convert_move, at expr.c:299
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=86166
    subprocess.call("patch -p1 < ./src/neva/skia/skia_convert_move_compiler_error_fix.patch", shell=True)
    change_result_str = subprocess.check_output("git diff --stat", shell=True, cwd="./src/third_party/skia/")

    if change_result_str:
        subprocess.call("git add .; git commit --author='Vladislav Mukulov <vladislav.mukulov@lge.com>' -m '" + commit_msg + "'", shell=True, cwd="./src/third_party/skia/")

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.stderr.write('interrupted\n')
        sys.exit(1)
else:
    print "__name__ = ", __name__
