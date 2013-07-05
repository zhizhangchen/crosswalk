CURRENT_DIR="$(realpath $(dirname $0))"
../../out/Release/xwalk --external-extensions-path=$CURRENT_DIR  --remote-debugging-port=9222 file://$CURRENT_DIR/test.html
