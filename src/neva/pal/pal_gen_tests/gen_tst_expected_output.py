import os
import shutil
import sys

from tst_gen_code import CheckGeneratedCodeClass


def main():
    work_path, gen_out_path, gen_in_path, gen_path, script_name, log_file = \
        CheckGeneratedCodeClass.get_work_paths()
    tst_interfaces = []
    CheckGeneratedCodeClass.get_test_interfaces(tst_interfaces)

    # generate pal output c++ original
    for interface in tst_interfaces:
        CheckGeneratedCodeClass.clear_dir(gen_in_path)
        CheckGeneratedCodeClass.clear_dir(gen_out_path)
        pal_gen_out_path = os.path.join(gen_out_path, 'pal')
        shutil.copy(
            os.path.join(work_path,
                         CheckGeneratedCodeClass.JSON_SUNNY_CASES_DIR,
                         interface) +
            '.json', gen_in_path)
        cmd = 'python ' + script_name + ' -o ' + pal_gen_out_path \
            + ' -i ' + gen_in_path
        os.system(cmd)

        ex_out_path = os.path.join(
            work_path, CheckGeneratedCodeClass.JSON_SUNNY_CASES_DIR,
            (interface + '_expected_output'))
        if os.path.exists(ex_out_path):
            shutil.rmtree(ex_out_path)
        shutil.copytree(gen_out_path, ex_out_path)

    # generate .gni original
    CheckGeneratedCodeClass.clear_dir(gen_in_path)
    CheckGeneratedCodeClass.clear_dir(gen_out_path)
    for interface in tst_interfaces:
        shutil.copy(
            os.path.join(
                work_path, CheckGeneratedCodeClass.JSON_SUNNY_CASES_DIR,
                interface) +
            '.json', gen_in_path)
    cmd = ('python ' + script_name + ' -o ' + gen_out_path +
           ' -i ' + gen_in_path + ' -g')
    os.system(cmd)
    gen_fn = os.path.join(gen_out_path,
                          CheckGeneratedCodeClass.GNI_FILE_NAME)
    save_fn = os.path.join(
        work_path, CheckGeneratedCodeClass.JSON_SUNNY_CASES_DIR,
        CheckGeneratedCodeClass.EXP_OUT_GNI_FILE_NAME)
    shutil.copy(gen_fn, save_fn)

    CheckGeneratedCodeClass.clear_dir(gen_in_path, False)
    CheckGeneratedCodeClass.clear_dir(gen_out_path, False)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.stderr.write('interrupted\n')
        sys.exit(1)
