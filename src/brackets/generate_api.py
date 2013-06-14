import os
import sys

sys.path.append(os.path.abspath("../../../v8/tools/js2c.py"))

import js2c

TEMPLATE = """\
static const char kGeneratedSource[] = { %s, 0 };
"""

js_code = sys.argv[1]
lines = js2c.ReadFile(js_code)
c_code = js2c.ToCAsciiArray(lines);

output = open(sys.argv[2], "w")
output.write(TEMPLATE % c_code);
output.close()
