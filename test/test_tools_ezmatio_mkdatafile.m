% This file is part of the Tomographer project, which is distributed under the
% terms of the MIT license.
%
% The MIT License (MIT)
%
% Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
% Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
%
% Permission is hereby granted, free of charge, to any person obtaining a copy
% of this software and associated documentation files (the "Software"), to deal
% in the Software without restriction, including without limitation the rights
% to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
% copies of the Software, and to permit persons to whom the Software is
% furnished to do so, subject to the following conditions:
%
% The above copyright notice and this permission notice shall be included in
% all copies or substantial portions of the Software.
%
% THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
% IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
% FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
% AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
% LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
% OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
% SOFTWARE.


d = struct;

% double, simple value
d.d = double(3.14);
% single, simple value
d.f = single(2.718);
% int8, simple value
d.i8 = int8(42);
% int16, simple value
d.i16 = int16(-32768);
% int32, simple value
d.i32 = int32(2147483647);
% int64, simple value
d.i64 = int64(-9223372036854775808);
% uint8, simple value
d.u8 = uint8(42);
% uint16, simple value
d.u16 = uint16(65535);
% uint32, simple value
d.u32 = uint32(4294967295);
% uint64, simple value
d.u64 = uint64(18446744073709551615);

% double, 4x3 matrix
d.md_4x3 = double([1.0, 2.0, 3.0; 1.5, 3, 4.5; 100.0, 200.0, 300.0; 0.0 0.0 1.0]);
% double row vector
d.rvd_5 = double([1.0, 2.0, -3.0, 4.0, -193.223]);
% double column vector
d.vd_5 = double([1.0; 2.0; -3.0; 4.0; -193.223]);

% double, 4x3 matrix
d.mcd_4x3 = [1 0 0; 0 1 0; 0 0 1; 0 0 0] + 1i*double([1.0, 2.0, 3.0; 1.5, 3, 4.5; 100.0, 200.0, 300.0; 0.0 0.0 1.0]);
% double row vector
d.rvcd_5 = double([1.0+1i, 2.0+2.5i, -3.0, 4.0, -193.223]);
% double column vector
d.vcd_5 = double([1.0+1i; 2.0-2.5i; -3.0; 4.0; -193.223]);

% cplx double, 3x Pauli matrices
d.mcd_2x2x3 = zeros(2,2,3);
d.mcd_2x2x3(:,:,1) = [0 1; 1 0];
d.mcd_2x2x3(:,:,2) = [0 -1i; 1i 0];
d.mcd_2x2x3(:,:,3) = [1 0; 0 -1];

% cplx single, 3x Pauli matrices
d.mcf_2x2x3 = single(zeros(2,2,3));
d.mcf_2x2x3(:,:,1) = single([0 1; 1 0]);
d.mcf_2x2x3(:,:,2) = single([0 -1i; 1i 0]);
d.mcf_2x2x3(:,:,3) = single([1 0; 0 -1]);

% single, two 2x3 matrices
d.mf_2x3x2 = single(zeros(2,3,2));
d.mf_2x3x2(:,:,1) = single([ 1 4 -2.5; 1.0 1.5 -1e4 ]);
d.mf_2x3x2(:,:,2) = single([ 0 0 0; 1 -2 -3 ]);

% cplx double, four (2x2) 2x3 matrices
d.mcd_2x3x2x2 = (zeros(2,3,2,2));
d.mcd_2x3x2x2(:,:,1,1) = ([ 1 1i -1i; 1.0 1.5i -1e4+1e3i ]);
d.mcd_2x3x2x2(:,:,2,1) = ([ 0 0 0; 1i -2i -3i ]);
d.mcd_2x3x2x2(:,:,1,2) = ([ 1 0 0; 0 1 0 ]);
d.mcd_2x3x2x2(:,:,2,2) = ([ 0 0 0; 1i -2i -3i ]);

% single, 4x3 matrix
d.mf_4x3 = single([1.0, 2.0, 3.0; 1.5, 3, 4.5; 100.0, 200.0, 300.0; 0.0 0.0 1.0]);
% single row vector
d.rvf_5 = single([1.0, 2.0, -3.0, 4.0, -193.223]);
% single column vector
d.vf_5 = single([1.0; 2.0; -3.0; 4.0; -193.223]);

% int8, 3x3 matrix
d.mi8_3x3 = int8([1 1 1; 2 2 2; 127 0 -128]);
% int32, 3x3 matrix
d.mi32_3x3 = int32([1 1 1; 2 2 2; 2147483647 0 -2147483648]);
% uint32, 3x3 matrix
d.mu32_3x3 = uint32([1 1 1; 2 2 2; 4294967295 0 0]);

% single real, pos semidef matrix
d.psdf_3x3 = single([   1, 0.5, 0.2; ...
                      0.5,   1, 0.1; ...
                      0.2, 0.1,   1]);
% double real, pos semidef matrix
d.psdd_2x2 = [ 1/sqrt(2.0), 1/sqrt(2.0); ...
               1/sqrt(2.0), 1/sqrt(2.0) ];
% complex single, pos semidef matrix
d.psdcf_2x2 = single([ 0.5, 0.5*1i;
                      -0.5*1i, 0.5 ]);
% complex double, pos semidef matrix
d.psdcd_2x2 = [ 1/sqrt(2.0),   1i/sqrt(2.0); ...
                -1i/sqrt(2.0), 1/sqrt(2.0) ];


% char, N row vector
d.rvc = 'A character string.';

% cell array, 3x2 with various entries
d.c_3x2 = { ...
    'One', 'Two'; ...
    1.0, 2.0; ...
    eye(2)/2, [0.5, 0.5; 0.5, 0.5] ...
          };

% struct with the definitions above.
d.s = d;



% save all of that to our data file.
save('data/test_tools_ezmatio/test_tools_ezmatio_data.mat', '-struct', 'd', '-v6');
