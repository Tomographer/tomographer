function tf = ischarstring(x)
%  
% ISCHARSTRING Tests if argument is a character string.
%
%   tf = ischarstring(x)
%
% This is equivalent to testing that ischar(x), that ndims(x) <= 2 and that
% size(x,1) == 1.
%
  
  tf = ischar(x) && ndims(x) <= 2 && size(x,1) == 1;
  
end
