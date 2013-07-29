# Copyright (c) 2013 Intel Corporation. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

SOURCES = extension.cc
OBJECTS = extension.o
NODE_PATH = node
CFLAGS = -Wall -Wextra -fPIC -I$(NODE_PATH)/src -I$(NODE_PATH)/deps/uv/include -I$(NODE_PATH)/deps/v8/include -I../..
LDFLAGS = -L$(NODE_PATH)/out/Release/lib.target -lnode
EXTENSION_BASE = node
EXTENSION = $(EXTENSION_BASE).so
JSAPI = $(EXTENSION_BASE)_api.h
JSAPISRC = $(EXTENSION_BASE)_api.js
NODE_MAIN = node_main.h
NODE_MAIN_SRC = node_main.js

all: $(SOURCES) $(EXTENSION)

$(EXTENSION): $(OBJECTS)
	$(CXX) $(CFLAGS) $(OBJECTS) -shared -o $(EXTENSION) $(LDFLAGS)

.cc.o:
	$(CXX) $(CFLAGS) $< -c -o $@

extension.o: $(JSAPI) $(NODE_MAIN)

$(JSAPI): $(JSAPISRC)
	hexdump -v -e '10/1 "0x%02x, "' -e '"\n"' $(JSAPISRC) | sed 's/0x  ,//g' > $(JSAPI).tmp
	echo "static const char kGeneratedSource[] = {" > $(JSAPI)
	cat $(JSAPI).tmp >> $(JSAPI)
	rm -f $(JSAPI).tmp
	echo "};" >> $(JSAPI)

$(NODE_MAIN): $(NODE_MAIN_SRC)
	hexdump -v -e '10/1 "0x%02x, "' -e '"\n"' $(NODE_MAIN_SRC) | sed 's/0x  ,//g' > $(NODE_MAIN).tmp
	echo "static const char kNodeMainSource[] = {" > $(NODE_MAIN)
	cat $(NODE_MAIN).tmp >> $(NODE_MAIN)
	rm -f $(NODE_MAIN).tmp
	echo "};" >> $(NODE_MAIN)
clean:
	rm -f $(EXTENSION) $(OBJECTS) $(JSAPI) $(NODE_MAIN) *~
