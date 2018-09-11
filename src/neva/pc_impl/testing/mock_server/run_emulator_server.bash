#!/bin/bash

MOCKPORT=8888
WEBPORT=8887
SLEEPSTEP=.5
BACKOFFTRIES=10

cleanup() {
    kill -TERM $MOCKSRVPID $WEBSRVPID
    exit ${1:-0}
}

trap cleanup SIGINT

while read bin package
do
    [ -z `which $bin` ] && {
        echo >&2 "You don't have $bin installed: try 'sudo apt-get install $package'"
        exit 1
    }
done <<EOF
java default-jre
curl curl
fuser psmisc
EOF

CUR_DIR=$PWD

# To disable logging from the MockServer and MockServer Proxy classes
# (available debugging levels: WARN, INFO and TRACE)
logoff_flag="-Dmockserver.logLevel=OFF"
# To disable all logging from all other classes (i.e. all none
# MockServer and MockServer Proxy classes)
logoff_all_flag="-Droot.logLevel=OFF"

# Expectations path
expect_path="http://localhost:$MOCKPORT/expectation"

# Request parameters used by the curl tool
params="-H Content-Type:multipart/form-data"

with_backoff() {
  "$@" ||
  for backoff in `seq 1 $BACKOFFTRIES`
  do
    sleep $SLEEPSTEP
    "$@" && break
  done
}

# Function for creating expectations for all paths in dir ../tests as well as
# for the root path (http://localhost:$MOCKPORT/ opens folder view from the root)
CREATE_EXPECTATIONS()
{
    for FILENAME in `find * -type f` __IP__ ""
    do
        with_backoff curl --request PUT $expect_path $params --data \
"`cat <<EOF
{
  "httpRequest": {"method": "GET", "path": "/$FILENAME"},
  "httpForward": {"host": "localhost", "port":$WEBPORT, "scheme":"HTTP"},
  "times": {"unlimited": true}
}
EOF`" || cleanup 1
        with_backoff curl --request PUT $expect_path $params --data \
"`cat <<EOF
{
  "httpRequest": {"method": "POST", "path": "/$FILENAME"},
  "httpForward": {"host": "localhost", "port":$WEBPORT, "scheme":"HTTP"},
  "times": {"unlimited": true}
}
EOF`" || cleanup 1
    done
}

# Running the MockServer
java $logoff_flag $logoff_all_flag -jar mockserver-netty-3.10.4-jar-with-dependencies.jar -serverPort $MOCKPORT &
MOCKSRVPID=$!

# Waiting for a while to get the server started
with_backoff fuser -n tcp $MOCKPORT -s || {
    echo >&2 "Can not start MockServer, exiting..."
    cleanup 1
}

# Running server managing files starting from the root path to nested paths
./tests_server.py $WEBPORT $CUR_DIR/../tests &
WEBSRVPID=$!

with_backoff fuser -n tcp $WEBPORT -s || {
    echo >&2 "Can not start TestsServer, exiting..."
    cleanup 1
}

cd ../tests
CREATE_EXPECTATIONS

echo "MockServer (PID=$MOCKSRVPID) and WebServer (PID=$WEBSRVPID) has been started and configured. Type [q|Q] to terminate..."

grep -qLiw q

cleanup
