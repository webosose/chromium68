#!/bin/bash

trap 'rm -fR "$USER_DATA_DIR"' EXIT

USER_DATA_DIR=`mktemp -d /tmp/$$.XXXXXX`
BASE=$(readlink -f "`dirname $0`/../../..")
APP_SHELL=$BASE/pc/chrome/chrome
ENACT_BROWSER=$BASE/enact-browser/dist

[ -z $WAYLAND_DISPLAY ] && {
  echo "You shuld run this script from inside weston's terminal"
  exit 1
}

for exe in "$APP_SHELL"
do
	[ -x $exe ] || chmod +x $exe
done

exec -a enact_browser "$APP_SHELL" --no-sandbox --silent-launch --load-and-launch-app=$ENACT_BROWSER --user-data-dir=$USER_DATA_DIR
