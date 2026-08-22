# licenses

Licenses directory contains license, notice and patent copies for each library that is used or may be used (depending on the build configuration) in the project.

They may need or may not need to be included in the releases depending on how to release was build. Since we're producing statically-built releases for Windows, all of those must be included in our official build - but not all of them in yours.

Include the license for whatever you actually enabled at configure time, and also depending on whether you linked it statically or dynamically. Consult the licenses and terms of the libraries themselves to decide.
