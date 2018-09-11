#!/bin/bash

BASE=$(readlink -f "`dirname $0`/../../..")
APP_SHELL=$BASE/pc/app_shell/app_shell
ENACT_BROWSER=$BASE/enact-browser/dist

[ -z $WAYLAND_DISPLAY ] && {
  echo "You shuld run this script from inside weston's terminal"
  exit 1
}

for exe in "$APP_SHELL"
do
	[ -x $exe ] || chmod +x $exe
done

exec -a enact_browser "$APP_SHELL" --no-sandbox --load-apps=$ENACT_BROWSER
