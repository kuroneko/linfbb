# Installation

This version of linfbb is built using cmake.

This is not an exhaustive guide, but a simple example is provided below.

The old installation guide is in `INSTALL.OLD` and provides instructions beyond compilation and install.

## Configuration and Build

```shell
  mkdir build
  cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/opt/fbb
  cmake --build build -j
```

## Installation

```shell
  cmake --install build
  # if you need to install a new configuration too
  cmake --install build --component NewConfig
```

# Build time configuration

You can specify `-DFBB_USE_SYSTEM_PATHS=OFF` to cmake if you're NOT installing into
shared prefixes (/usr, etc) and want a flatter var/lib/etc heirachy.

This does not automatically fix the DOCDIR paths and those should be adjusted in addition.
