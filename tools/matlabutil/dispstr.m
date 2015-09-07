function s = dispstr(x, maxitems)
  
  if (nargin < 2)
    maxitems = 8;
  end
  
  if (ischar(x))
    s = dispchararray(x, round(maxitems/2), maxitems*3);
  elseif (islogical(x) && numel(x) == 1)
    if (x)
      s = 'true';
    else
      s = 'false';
    end
  elseif (isnumeric(x) || islogical(x))
    s = disparray(x, @num2str, '[', ']', maxitems, '');
  elseif (iscell(x))
    if (numel(x) == 1)
      s = ['{' dispstr(x{1}) '}'];
    else
      s = disparray(x, @dispcellitem, '{', '}', round(maxitems/2), 'cell');
    end
  elseif (isstruct(x))
    s = dispdimsspec(x, 'struct');
  else
    s = dispdimsspec(x, class(x));
  end
  
end

function s = dispcellitem(x)
  s = dispstr(x{1});
end

function s = fmtstrabbrev(x, limit)
  
  if (nargin < 2)
    limit = 12;
  end
  
  s = [ x ];
  if (numel(s) > limit)
    s = [s(1:(limit-3)) '...'];
  end
  s = [ '''' s '''' ];
  %display(s);
  
end


function s = disparray(N, fmtfunc, beginsym, endsym, maxnumel, spec)
  
  n = N;
  
  if (numel(n) == 0)
    s = [ beginsym endsym ];
  elseif (numel(n) == 1)
    s = fmtfunc(n);
  elseif (ndims(n) <= 2 && numel(n) <= maxnumel)
    suffix = '';
    % if it is a column vector, write it as a transposed row vector
    if (size(n,2) == 1)
      n = n.';
      suffix = '.''';
    end
    s = [beginsym ' '];
    for i=1:size(n,1)
      for j=1:size(n,2)
        if (j > 1)
          s = [s '  '];
        end
        s = [s  fmtfunc(n(i,j)) ];
      end
      if (i < size(n,1))
        s = [s ' ; '];
      end
    end
    s = [s ' ' endsym suffix];
  else
    s = dispdimsspec(n, spec);
  end
  
end

function s = dispchararray(N, maxstrings, strlenlimit)
  
  n = N;
  
  if (numel(n) == 0)
    s = '''''';
  elseif (ndims(n) <= 2 && size(n, 1) == 1)
    s = fmtstrabbrev(N, strlenlimit);
  elseif (ndims(n) <= 2 && size(n,2) <= maxstrings)
    suffix = '';
    % % if it is a column vector, write it as a transposed row vector
    % if (size(n,2) == 1)
    %   n = n.';
    %   suffix = '.''';
    % end
    s = '[ ';
    for i=1:size(n,1)
      s = [s  fmtstrabbrev(n(i,:), strlenlimit) ];
      if (i < size(n,1))
        s = [s ' ; '];
      end
    end
    s = [s ' ]' suffix];
  else
    s = dispdimsspec(n, 'char');
  end
  
end

function s = dispdimsspec(n, spec)
  if (~strcmp(spec, ''))
    spec = [' ' spec];
  end
  
  if (ndims(n) == 2 && size(n,2) == 1)
    s = ['<' num2str(numel(n)) spec '>'];
  else
    s = ['<' num2str(size(n,1)) ];
    for i=2:ndims(n)
      s = [ s 'x' num2str(size(n,i)) ];
    end
    s = [s spec '>'];
  end
end
