function h5_data = h5_read( file_path, data_path, dimensions, complex_flag )
% h5_data = h5_read( file_path, data_path, dimensions, complex_flag )
%
% Reads a dataset from an HDF5 file, optionally reshaping it and promoting it
% to a complex data type.
%
% This is intended to work with the HDF5 Export library and provides an
% interface for reshaping exported data so it can be easily interacted with.
%
% Takes 4 arguments:
%
%   file_path    - Path to the HDF5 file.
%   data_path    - Path to the dataset within file_path.
%   dimensions   - Optional vector of dimensions the returned data should
%                  have.  If specified, one of the entries may be 'inf'
%                  to have said dimension's size be computed from the
%                  data read.  If omitted, defaults to the dimensions
%                  found within the dataset.
%   complex_flag - Optional flag indicating the data read are complex
%                  and should be promoted to MATLAB's complex data type.
%                  If omitted, defaults to false.  If specified as true
%                  any dimensions specified are in complex units rather
%                  than the original data type.
%
% Returns 1 value:
%
%   h5_data - Data read from data_path.

if nargin < 4
    complex_flag = [];
end
if nargin < 3
    dimensions = [];
end

if isempty( complex_flag )
    complex_flag = false;
end

h5_data = h5read( file_path, data_path );

% promote our interleaved complex values into a proper variable.
if complex_flag
    h5_data = complex( read_data(1:2:end), read_data(2:2:end) );
end

% reshape the data if the user want's something different than what was written
% to disk.
%
% NOTE: we assume the specified dimensions represent the complex data rather
%       than what was written to disk.
%
if ~isempty( dimensions )
    dimensions_cell = cell( length( dimensions ), 1 );

    % detect how many "compute it for me" dimensions are present and
    % complain if there are too many.  this turns a cryptic warning
    % in reshape() into something easier to understand.
    inf_count = length( find( isinf( dimensions ) ) );
    if inf_count > 1
        error( '%d inf''s were specified as the dimensions.  Must have 0 or 1.', ...
               inf_count );
    end

    % map the entries from the requested dimensionality into our cell, taking
    % care to map inf to an empty matrix that reshape() understands.
    for dimension_index = 1:length( dimensions_cell )
        dimensions_cell{dimension_index} = dimensions(dimension_index);

        if isinf( dimensions(dimension_index) )
            dimensions_cell{dimension_index} = [];
        end
    end

    h5_data = reshape( h5_data, dimensions_cell{:} );
end

return
