# PROJECT NOT UNDER ACTIVE MANAGEMENT #  
This project will no longer be maintained by Intel.  
Intel has ceased development and contributions including, but not limited to, maintenance, bug fixes, new releases, or updates, to this project.  
Intel no longer accepts patches to this project.  
 If you have an ongoing need to use this project, are interested in independently developing it, or would like to maintain patches for the open source software community, please create your own fork of this project.  
  

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
