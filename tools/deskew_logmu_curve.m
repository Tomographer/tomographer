function [a x0 y0] = deskew_logmu_curve(a2, a1, m, c)
% deskew_logmu_curve -- de-skew the fit model to obtain a second order approximation.
%
  
  x0 = (sqrt(a1.^2 + 8*a2.*m) - a1) ./ (4*a2);
  y0 = -a2*x0.^2 - a1*x0 + m*log(x0) + c;
  a = a2 + m ./ (2 * x0.^2);
  
end
