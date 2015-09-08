
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
d.u8 = int8(42);
% uint16, simple value
d.u16 = int16(65536);
% uint32, simple value
d.u32 = int32(2147483647);
% uint64, simple value
d.u64 = int64(9223372036854775807);

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
d.vcd_5 = double([1.0+1i; 2.0; -3.0; 4.0; -193.223]);

% single, 4x3 matrix
d.mf_4x3 = single([1.0, 2.0, 3.0; 1.5, 3, 4.5; 100.0, 200.0, 300.0]);
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
