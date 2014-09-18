gfilter
======

GCode transformation library organized as a streaming pipeline of independently 
configurable filters that can be rearranged to suit a particular transformation task.

### Example `PointOffsetFilter`
The `PointOffsetFilter` scans for G0/G1 movement commands and applies offsets
to compensate for individual CNC machine variances from desired coordinates.
The offsets are configurable using JSON:

<pre>
{
	"offsets":[
		{"point":[0,0,0], "offset":[0,0,0]},
		{"point":[0,0,1], "offset":[0,0,0.01]},
		{"point":[0,1,0], "offset":[0,0,0.02]},
		{"point":[0,1,1], "offset":[0,0,0.02]}
	]
}
</pre>

With the above configuration, you can create a `PointOffsetFilter` and send it some GCode:

<pre>
PointOffsetFilter pof(outfilter, configJSON);
pof.writeln("G0X0Y0Z1");
</pre>

The `outFilter` is simply the next filter in the pipeline. A common filter is OStreamSink,
which sends the output to an `ostream`. In this example, we're sending the transformed
GCode to `cout`:

<pre>
#include "gfilter.hpp"
json_t *configJSON = ...; // JSON configuration for PointOffsetFilter
OStreamSink outFilter(cout);
PointOffsetFilter pof(outFilter, configJSON);
pof.writeln("G0X0Y0Z1");
</pre>


The output of this program will be:

<pre>
G0Z1.01
</pre>

For more examples, [see the test code](https://github.com/firepick1/gfilter/blob/master/test/test.cpp)
