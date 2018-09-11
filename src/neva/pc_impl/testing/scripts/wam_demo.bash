#!/bin/bash

trap 'rm -fR "$USER_DATA_DIR"' EXIT

WAM_EMULATOR_PAGE=http://localhost:8888/wam_emulator_page.html
USER_DATA_DIR=`mktemp -d /tmp/$$.XXXXXX`
PIPE=$USER_DATA_DIR/pipe
BASE=$(readlink -f "`dirname $0`/../../..")
WAM_DEMO=$BASE/pc/wam_example/wam_demo
CHROMIUM=$BASE/pc/chrome/chrome
MOCK_SERVER=$BASE/pc/testing/mock_server/run_emulator_server.bash
TESTS_SERVER=$BASE/pc/testing/mock_server/tests_server.py

[ -z $WAYLAND_DISPLAY ] && {
  echo "You shuld run this script from inside weston's terminal"
  exit 1
}

for exe in "$WAM_DEMO" "$MOCK_SERVER" "$TESTS_SERVER" "$CHROMIUM"
do
	[ -x $exe ] || chmod +x $exe
done

mkfifo $PIPE
(
	cd $(dirname "$MOCK_SERVER")
	exec "$MOCK_SERVER"
) <> $PIPE & mock_server_pid=$!

"$WAM_DEMO" & wam_demo_pid=$!
"$CHROMIUM" --no-sandbox --start-maximized --user-data-dir=$USER_DATA_DIR $WAM_EMULATOR_PAGE & chromium_pid=$!

wait $chromium_pid
kill -INT $wam_demo_pid $chromuim_pid $mock_server_pid
