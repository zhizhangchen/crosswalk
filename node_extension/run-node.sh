#!/bin/bash
# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [ ! -x "$1" -o ! -f "$2" ]; then
    echo "Run Crosswalk with node extension"
    echo "Usage: $0 /path/to/xwalk/executable /path/to/htmlfile"
    exit 1
fi

CURRENT_DIRECTORY=$(readlink -f $(dirname $0))
XWALK_EXECUTABLE="$1"

if [ ! -x "$CURRENT_DIRECTORY/node.so" ]; then
    echo "Node extension not found, trying to build."
    if ! make; then
        echo "Couldn't build extension, please file a bug report at"
        echo "  https://github.com/otcshare/crosswalk"
        exit 1
    fi
fi

exec $XWALK_EXECUTABLE \
	--remote-debugging-port=9234 \
	--disable-web-security \
	--allow-file-access-from-files \
	--external-extensions-path=$CURRENT_DIRECTORY \
	$2
