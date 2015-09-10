% This file is part of the Tomographer project, which is distributed under the
% terms of the MIT license.
% 
% The MIT License (MIT)
% 
% Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe
% Faist
% 
% Permission is hereby granted, free of charge, to any person obtaining a copy
% of this software and associated documentation files (the "Software"), to deal
% in the Software without restriction, including without limitation the rights
% to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
% copies of the Software, and to permit persons to whom the Software is
% furnished to do so, subject to the following conditions:
% 
% The above copyright notice and this permission notice shall be included in all
% copies or substantial portions of the Software.
% 
% THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
% IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
% FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
% AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
% LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
% OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
% SOFTWARE.


%
% save some particular tomography data to a .mat file
%

Cdata = struct()

Cdata.dim = 2;

Cdata.Emn = zeros(2,2,2);
Cdata.Emn(:,:,1) = [0.5, -0.5i; 0.5i, 0.5]; % +1 eigenspace of sigma_Y
Cdata.Emn(:,:,2) = [0.5, 0.5i; -0.5i, 0.5]; % -1 eigenspace of sigma_Y

% tomography data: 500x counts +1, 0x counts -1; when measuring sigma_Y 500 times in total
Cdata.Nm = [ 500; 0 ];

Cdata.rho_MLE = [0.5, -0.5i; 0.5i, 0.5];
Cdata.rho_ref = [0.5, -0.5i; 0.5i, 0.5];


save('test_tomorun_1qubit_analytic_solution.mat', '-struct', 'Cdata', '-v6');


%
% this dataset is very handy because we know its analytical form (up to a constant):
%
%     func = @(f) (1/c) * f.^2 .* (1+f) .* (1-f) .* exp(2*n*log(f));
%
% The normalizing constant [cf Mathematica file] is:
%
%     c = 2 / (15+16*n+4*n^2)
%
%
% Mathematica code:
% -----------------
%
% mu[f_] := (f)^2 (1 + f) (1 - f) Exp[2 n Log[f]]
% Integrate[mu[f], {f, 0, 1}, Assumptions -> {n > 0}]
%
%[  Out[2]= 2/(15 + 16 n + 4 n^2)  ]
%
