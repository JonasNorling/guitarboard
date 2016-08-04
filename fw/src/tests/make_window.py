#!/usr/bin/python

import math

N=2048

# Hann window
with open("window.h", "w") as f:
	f.write("// %u point Hann window\n" % N)
	f.write("static const float hannWindow[%u] = {\n" % N)
	for w in [ 1-math.cos((2*math.pi*n)/(N-1)) for n in range(0, N) ]:
		f.write("    %8f,\n" % w)
	f.write("};\n")
