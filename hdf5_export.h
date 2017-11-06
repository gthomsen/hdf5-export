#ifndef __HDF5_SHIM_H__
#define __HDF5_SHIM_H__

/* 32-bit integers. */
int h5_export_vector_i32( const char *file_path, const char *data_path, const int *data,
                          int dim1_length );
int h5_export_matrix_i32( const char *file_path, const char *data_path, const int *data,
                          int dim1_length, int dim2_length );
int h5_export_volume_i32( const char *file_path, const char *data_path, const int *data,
                          int dim1_length, int dim2_length, int dim3_length );

/* 32-bit floating point. */
int h5_export_vector_f32( const char *file_path, const char *data_path, const float *data,
                          int dim1_length );
int h5_export_matrix_f32( const char *file_path, const char *data_path, const float *data,
                          int dim1_length, int dim2_length );
int h5_export_volume_f32( const char *file_path, const char *data_path, const float *data,
                          int dim1_length, int dim2_length, int dim3_length );

/* 64-bit floating point. */
int h5_export_vector_f64( const char *file_path, const char *data_path, const double *data,
                          int dim1_length );
int h5_export_matrix_f64( const char *file_path, const char *data_path, const double *data,
                          int dim1_length, int dim2_length );
int h5_export_volume_f64( const char *file_path, const char *data_path, const double *data,
                          int dim1_length, int dim2_length, int dim3_length );

#endif  /* __HDF5_SHIM_H__ */
