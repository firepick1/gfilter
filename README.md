gfilter
======

GCode transformation library organized as a streaming pipeline of independently 
configurable filters that can be rearranged to suit a particular transformation task.

### Example `MappedPointFilter`
The `MappedPointFilter` scans for G0/G1 movement commands and maps the incoming _domain_ of points
to the output _range_ of points that compensate for individual CNC machine variances 
from ideal coordinates. The point mappings are configurable using JSON. Intermediate points
will be interpolated:

<pre>
{
	"map":[
		{"domain":[0,0,0], "range":[0,0,0]},
		{"domain":[0,0,1], "range":[0,0,1.01]},
		{"domain":[0,1,0], "range":[0,1,0.02]},
		{"domain":[0,1,1], "range":[0,1,1.02]}
	]
}
</pre>

With the above configuration, you can create a `MappedPointFilter` and send it some GCode:

<pre>
MappedPointFilter pof(outfilter, configJSON);
pof.writeln("G0X0Y0Z1");
</pre>

The `outFilter` is simply the next filter in the pipeline. A common filter is OStreamSink,
which sends the output to an `ostream`. In this example, we're sending the transformed
GCode to `cout`:

<pre>
#include "gfilter.hpp"
json_t *configJSON = ...; // JSON configuration for MappedPointFilter
OStreamSink outFilter(cout);
MappedPointFilter pof(outFilter, configJSON);
pof.writeln("G0X0Y0Z1");
</pre>


The output of this program will be:

<pre>
G0X0Y0Z1.01
</pre>

For more examples, [see the test code](https://github.com/firepick1/gfilter/blob/master/test/test.cpp)
