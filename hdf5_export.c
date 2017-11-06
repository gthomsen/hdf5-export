#include <hdf5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hdf5_export.h"

#define CONSTRUCTOR __attribute__((constructor))
#define DESTRUCTOR  __attribute__((destructor))

/* initialize the HDF5 library if it hasn't already.  while this should
   be automatically done from the C interface, it is recommended to
   explicitly do so just in case. */
CONSTRUCTOR static int h5_export_init()
{
    herr_t status = H5open();

    /* map negative errors to positive errors. */
    return (status < 0);
}

/* shutdown the HDF5 library if it hasn't been done already.  this is provided
   for more advanced uses where the shared object is unloaded from memory. */
DESTRUCTOR static int h5_export_shutdown()
{
    herr_t status = H5close();

    /* map negative errors to positive errors. */
    return (status < 0);
}

/* returns an HDF5 file handle for writing.  if an HDF5 file already exists
   at the requested path it is opened, otherwise it is created. */
static hid_t open_file( const char *file_path )
{
    /* check whether the file exists for reading and writing.  we have to
       explicitly open or create the file depending on whether it does. */
    int file_exists_flag = (access( file_path, (R_OK | W_OK) ) == 0);

    hid_t file_id = -1;

    if( file_exists_flag )
        file_id = H5Fopen( file_path, H5F_ACC_RDWR, H5P_DEFAULT );
    else
        file_id = H5Fcreate( file_path, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );

    if( 0 > file_id )
        fprintf( stderr,
                 "Failed to %s \"%s\" (%d)!\n",
                 (file_exists_flag ? "open" : "create"),
                 file_path,
                 file_id );

    return file_id;
}

/* convenience wrapper for H5Lexists() since it does not validate each of the
   components in the requested data path, but forces users to implement that on
   their own.  returns the same values as H5Lexits() does. */
static hid_t H5Lexists_wrapper( hid_t location_id, const char *name, hid_t lapl_id )
{
    /* result of our validation via H5Lexists(). */
    hid_t  status           = -1;

    /* copy of the data path we're validating. */
    char *temp_name      = NULL;

    /* position of the current component's trailing slash. */
    char *slash_position = NULL;

    /* length of the current component's full path. */
    size_t name_index       = 0;

    /* length of the full data path and the current component being examined. */
    size_t total_length     = strlen( name );
    size_t component_length = 0;

    /* allocate space for the working buffer.  assuming the name requested
       exists, this will hold an ever-growing subset of it as we validate
       each component. */
    if( NULL == (temp_name = malloc( total_length + 1 )) )
    {
        fprintf( stderr,
                 "Failed to allocate %ld byte%s for \"%s\" to check if the path exists.\n",
                 total_length + 1,
                 (total_length == 0) ? "" : "s",
                 name );

        return -1;
    }

    /* walk through the components until we've found the last one. */
    do
    {
        /* find the next slash separating path components and compute it's
           length.  if we don't find a slash, then we're at the last one. */
        if( NULL == (slash_position = strchr( name + name_index, '/' )) )
            component_length = total_length - name_index;
        else
            /* each component includes the trailing slash (+1). */
            component_length = slash_position - name - name_index + 1;

        /* append the current component to our temporary buffer and
           NUL terminate it. */
        memcpy( temp_name + name_index,
                name + name_index,
                component_length );
        temp_name[name_index + component_length] = 0;

        /* jump past this component for the next search. */
        name_index += component_length;

        /* check this component's existence.  if we encounter a problem we
           bail. */
        if( 0 > (status = H5Lexists( location_id, temp_name, lapl_id )) )
        {
            fprintf( stderr,
                     "Failed to check \"%s\" for existence (%d)!\n",
                     temp_name, status );
            break;
        }
    }
    while( NULL != slash_position );

    free( temp_name );

    return status;
}

/* opens a dataset at the specified path for writing.  if a dataset already
   exists with said path, it is unlinked first so the dataspace (data type and
   dimensionality) matches the current request. */
static hid_t open_dataset( hid_t file_id,
                           const char *data_path, hid_t data_type,
                           int number_dimensions, const hsize_t *dimensions )
{
    htri_t status = H5Lexists_wrapper( file_id, data_path, H5P_DEFAULT );

    hid_t  dataspace_id = -1;
    hid_t  dataset_id   = -1;
    hid_t  lcpl_list    = -1;

    /* did we fail to query the dataset? */
    if( status < 0 )
    {
        fprintf( stderr,
                 "Failed to determine if \"%s\" exists! (%d)\n",
                 data_path, status );

        return -1;
    }

    /* we need to remove existing data sets so we can recreate a dataspace that
       matches the current one.  otherwise we run into issues when we attempt to
       write data of one type/dimensionality into data of another
       type/dimensionality (N-many floats in an M-many integer buffer). */
    if( status > 0 )
    {
        if( 0 > (status = H5Ldelete( file_id, data_path, H5P_DEFAULT )) )
        {
            fprintf( stderr,
                     "Failed to delete the existing dataset \"%s\" (%d)!\n",
                     data_path, status );

            return -1;
        }
    }

    /* create intermediate groups that don't exist.  this let's us write a
       dataset called "/foo/bar/qux" in a newly created file. */
    lcpl_list = H5Pcreate( H5P_LINK_CREATE );
    H5Pset_create_intermediate_group( lcpl_list, 1 );

    /* create a dataset that matches the requested data type and
       dimensionality. */
    if( 0 > (dataspace_id = H5Screate_simple( number_dimensions,
                                              dimensions,
                                              NULL )) )
    {
        /* XXX: factor this code into a separate function to print out the dataspace */
        fprintf( stderr,
                 "Failed to create a simple dataspace of %d dimension%s! (%d)\n",
                 number_dimensions,
                 (number_dimensions == 1) ? "" : "s",
                 dataspace_id );

        return -1;
    }

    if( 0 > (dataset_id = H5Dcreate2( file_id, data_path, data_type, dataspace_id,
                                      lcpl_list, H5P_DEFAULT, H5P_DEFAULT )) )
    {
        fprintf( stderr,
                 "Failed to create \"%s\"! (%d)\n",
                 data_path, dataset_id );

        return -1;
    }

    H5Sclose( dataspace_id );
    H5Pclose( lcpl_list );

    return dataset_id;
}

/* maps an HDF5 data type to its corresponding native type. */
static hid_t lookup_native_data_type( hid_t data_type )
{
    /* NOTE: we have a giant if/elseif/.../else construct since the H5T_STD_*
             "constants" aren't really constants and that prevents us from
             using a case statement. */

    /* 8-bit integers. */
    if( data_type == H5T_STD_I8BE ||
        data_type == H5T_STD_I8LE )
        return H5T_NATIVE_INT8;
    else if( data_type == H5T_STD_U8BE ||
             data_type ==  H5T_STD_U8LE )
        return H5T_NATIVE_UINT8;
    /* 16-bit integers. */
    else if( data_type == H5T_STD_I16BE ||
             data_type ==  H5T_STD_I16LE )
        return H5T_NATIVE_INT16;
    else if( data_type == H5T_STD_U16BE ||
             data_type == H5T_STD_U16LE )
        return H5T_NATIVE_UINT16;
    /* 32-bit integers. */
    else if( data_type == H5T_STD_I32BE ||
             data_type == H5T_STD_I32LE )
        return H5T_NATIVE_INT32;
    else if( data_type == H5T_STD_U32BE ||
             data_type == H5T_STD_U32LE )
        return H5T_NATIVE_UINT32;
    /* 64-bit integers. */
    else if( data_type == H5T_STD_I64BE ||
             data_type == H5T_STD_I64LE )
        return H5T_NATIVE_INT64;
    else if( data_type == H5T_STD_U64BE ||
             data_type == H5T_STD_U64LE )
        return H5T_NATIVE_UINT64;
    /* 32-bit floating point. */
    else if( data_type == H5T_IEEE_F32BE ||
             data_type == H5T_IEEE_F32LE )
        return H5T_NATIVE_FLOAT;
    /* 64-bit floating point. */
    else if( data_type == H5T_IEEE_F64BE ||
             data_type == H5T_IEEE_F64LE )
        return H5T_NATIVE_DOUBLE;
    /* assume we were already handed a native data type. */
    else
        return data_type;
}

/* writes the supplied data to a dataset.  it is assumed the extent of the
   supplied buffer is no larger than the in-memory dataspace associated with the
   requested dataset. */
static int write_dataset( hid_t dataset_id, hid_t data_type, const void *data )
{
    hid_t status = -1;

    /* data_type is the requested representation on disk, so we need to map it
       to the native variant used in memory.

       NOTE: to be truly flexible/correct, we should be accepting both the
             in-memory and on-disk data types. */
    hid_t native_data_type = lookup_native_data_type( data_type );

    if( 0 > (status = H5Dwrite( dataset_id, native_data_type, H5S_ALL, H5S_ALL,
                                H5P_DEFAULT, data )) )
    {
        fprintf( stderr,
                 "Failed to write data! (%d)\n",
                 status );
        return -1;
    }

    return 0;
}

/* main entry point for the library.  exports a buffer of data to a specific
   dataset within an HDF5 file.  care is taken to create a new HDF5 file (to
   work around HDF5's technical limitations) and replace existing datasets
   (to ensure the data written is what was intended). */
static int export_data( const char *file_path, const char *data_path, const void *data,
                        hid_t data_type,
                        int number_dimensions, const hsize_t *dimensions )
{
    hid_t file_id;
    hid_t dataset_id;

    /* open or create the file. */
    if( 0 > (file_id = open_file( file_path )) )
        return -1;

    /* open (or create) the dataset for writing. */
    if( 0 > (dataset_id = open_dataset( file_id, data_path, data_type,
                                        number_dimensions, dimensions )) )
        return -1;

    /* write the data. */
    if( 0 > (write_dataset( dataset_id, data_type, data )) )
        return -1;

    /* release the HDF5 resources and call it a day. */
    H5Dclose( dataset_id );
    H5Fclose( file_id );

    return 0;
}

/* ---------------------- public interface  ---------------------- */

/* 32-bit integers. */
int h5_export_vector_i32( const char *file_path, const char *data_path, const int *data,
                          int dim1_length )
{
    hsize_t dimensions[1] = { dim1_length };

    return export_data( file_path, data_path, data, H5T_STD_I32LE, 1, dimensions );
}

int h5_export_matrix_i32( const char *file_path, const char *data_path, const int *data,
                          int dim1_length, int dim2_length )
{
    hsize_t dimensions[2] = { dim1_length, dim2_length };

    return export_data( file_path, data_path, data, H5T_STD_I32LE, 2, dimensions );
}

int h5_export_volume_i32( const char *file_path, const char *data_path, const int *data,
                          int dim1_length, int dim2_length, int dim3_length )
{
    hsize_t dimensions[3] = { dim1_length, dim2_length, dim3_length };

    return export_data( file_path, data_path, data, H5T_STD_I32LE, 3, dimensions );
}

/* 32-bit floating point. */
int h5_export_vector_f32( const char *file_path, const char *data_path, const float *data,
                          int dim1_length )
{
    hsize_t dimensions[1] = { dim1_length };

    return export_data( file_path, data_path, data, H5T_IEEE_F32LE, 1, dimensions );
}

int h5_export_matrix_f32( const char *file_path, const char *data_path, const float *data,
                          int dim1_length, int dim2_length )
{
    hsize_t dimensions[2] = { dim1_length, dim2_length };

    return export_data( file_path, data_path, data, H5T_IEEE_F32LE, 2, dimensions );
}

int h5_export_volume_f32( const char *file_path, const char *data_path, const float *data,
                          int dim1_length, int dim2_length, int dim3_length )
{
    hsize_t dimensions[3] = { dim1_length, dim2_length, dim3_length };

    return export_data( file_path, data_path, data, H5T_IEEE_F32LE, 3, dimensions );
}

/* 64-bit floating point. */
int h5_export_vector_f64( const char *file_path, const char *data_path, const double *data,
                          int dim1_length )
{
    hsize_t dimensions[1] = { dim1_length };

    return export_data( file_path, data_path, data, H5T_IEEE_F64LE, 1, dimensions );
}

int h5_export_matrix_f64( const char *file_path, const char *data_path, const double *data,
                          int dim1_length, int dim2_length )
{
    hsize_t dimensions[2] = { dim1_length, dim2_length };

    return export_data( file_path, data_path, data, H5T_IEEE_F64LE, 2, dimensions );
}

int h5_export_volume_f64( const char *file_path, const char *data_path, const double *data,
                          int dim1_length, int dim2_length, int dim3_length )
{
    hsize_t dimensions[3] = { dim1_length, dim2_length, dim3_length };

    return export_data( file_path, data_path, data, H5T_IEEE_F64LE, 3, dimensions );
}
