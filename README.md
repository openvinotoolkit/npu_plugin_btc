
# BitCompactor compression/decompression model (C/C++)

## How to build:

* Create the build/ directory and move into it.
* Enter:
```bash
cmake ..
make -j8
```

## Manifest:

<pre>
.
|-- README                    <- this readme file
|-- include
|   |-- utils
|   |   |-- utils.h           <- SafeMem functions for klocwork
|   |   |-- logger.h          <- Simple logger class declaration
|   |-- bitCompactor.h        <- BitCompactor model (C++ class BitCompactor)
|-- src
|   |-- utils
|   |   |-- logger.cpp        <- Simple logger class implementation
|  `-- bitCompactor.cpp       <- BitCompactor model (C++ class BitCompactor implementation)
`- CMakeLists.txt             <- Example of CMakeLists.txt to build a shared library
</pre>
