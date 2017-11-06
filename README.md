# Overview

Simple library which exports regions of memory into HDF5 datasets for analysis
in external tools.  Intended to be used as part of the development and debugging
process for scientific computing as a way to get data into an environment where
higher-level investigation and comparison against known-good models can rapidly
occur (MATLAB, Python, etc).  The library is built as a shared object which may
be injected into an existing process during debugging (e.g. from within GDB) so
variables can be selectively exported and analyzed.

This library was originally written as a means to quickly export data from a
C/C++ application implementing an algorithm developed in MATLAB.  When
differences between the two implementations were detected, data were exported
onto disk where it could be read into a MATLAB session stopped at the
corresponding location in the reference implementation.  There, discrepancies
between the two could be analyzed at a high-level and a fix applied to the
appropriate implementation.

# Installation

Requires the following dependencies:

 * HDF5 1.8, or newer, to export data via the library.
 * [`hdf5oct`](https://github.com/gthomsen/hdf5oct) for reading data in
   Octave 4.x, or newer.
 * MATLAB or Octave for using `h5_read.m`.  Optional if data are analyzed
   in other tools.

Instructions:

 1. Build the library and install it somewhere:

    $ make
    $ cp hdf5_shim.so /path/to/installation

 2. Optionally install the MATLAB reader:

    $ cp matlab/h5_read.m /path/to/matlab/installation

# Usage

The library simplifies the process of writing data into an HDF5 file and
wraps the following steps into a single function that can be easily called
from a GDB prompt:

 1. Open an existing or create a new HDF5 file.
 2. Optionally remove an existing dataset to ensure the current data exported
    are represented properly on disk.
 3. Create a new dataset within the HDF5 with the requested
    dataspace/dimensionality.
 4. Write the data to the dataset.
 5. Close the file and sync the contents to disk.

This allows interactive exploration of an application as each export is flushed
to disk and is quickly visible to external applications.  The following shows
all that is needed to export a 3D array of 32-bit floats to `test.h5/3d_array`:

``` c
(gdb) call h5_export_volume_f32( "test.h5", "3d_array", float_buffer, 10, 10, 10 )
```

See below for injecting the library into an application so the above works.

Exported data matches the data type found in memory though the dimensionality of
data can be controlled so multiplexed/flattened arrays can have their sizes
properly expressed during analysis.  One-, two-, and three dimensional arrays
can be represented directly while higher-dimensional arrays can be exported as a
lower dimensional array and then further reshaped in external tools.  Note
that the decision to limit exports to three dimensional arrays was driven by
maintenance (read: laziness) rather than a technical limitation.

Each dimensionality has a function for export:

```c
h5_export_vector_*( "test.h5", "1d_array", buffer, 1000 )
h5_export_matrix_*( "test.h5", "2d_array", buffer, 10, 100 )
h5_export_volume_*( "test.h5", "3d_array", buffer, 10, 10, 10 )
```

Where `buffer` contains at least 1000 elements.

There is one group of export functions per data type and currently the following
data types are supported:

 * 32-bit signed integer (`i32`)
 * 32-bit floating point (`f32`)
 * 64-bit floating point (`f64`)

Additional functions are trivial to add and have not been due to a lack of need.
See `hdf5_export.h` for a complete list of functions and their signatures.

## Library Injection

The HDF5 Export library can be injected into an application in a number of ways
though some of the more common ones are outlined below.

### Into executables linked against `libdl.so`

From within GDB:
```
(gdb) break file.c:1234
(gdb) run
(gdb) call dlopen( "./hdf5_export.so", 2 )
```

### Into executables not linked against `libdl.so`

From within GDB:
```
# change the path to libdl.so as needed.
(gdb) set environment LD_PRELOAD /usr/lib64/libdl.so
(gdb) break file.c:1234
(gdb) run
(gdb) call __dlopen( "./hdf5_export.so", 2 )
```

From outside of GDB:
```
# change the path to libdl.so as needed.
LD_PRELOAD=/usr/lib64/libdl.so gdb /path/to/executable
(gdb) break file.c:1234
(gdb) run
(gdb) call __dlopen( "./hdf5_export.so", 2 )
```
