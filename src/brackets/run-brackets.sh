#!/bin/bash

cd "$(dirname "$0")"

if [ ! -d "$1" ]; then
    echo "Run Brackets using Cameo"
    echo "Usage: $0 <brackets-path>"
    exit 1
fi

BRACKETS_PATH="$1"
CAMEO_EXECUTABLE=../../../out/Release/cameo

$CAMEO_EXECUTABLE --disable-web-security --allow-file-access-from-files $BRACKETS_PATH/src/index.html
