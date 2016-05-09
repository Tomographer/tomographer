function [a2 a1 c] = reskew_logmu_curve(a, x0, y0, m)
% reskew_logmu_curve -- apply skew to parabola via m*log(x) term
%
% Implements the inverse function of deskew_logmu_curve.
%

  a2 = a - m ./ (2*x0.^2);
  a1 = m ./ x0 - 2*a2.*x0;
  c = y0 + a2.*x0.^2 + a1.*x0 - m.*log(x0);
  
end
