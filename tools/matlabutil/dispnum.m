function s = dispnum(N)
  
  n = N;
  
  if (numel(n) == 0)
    s = '[]';
  elseif (numel(n) == 1)
    s = num2str(n);
  elseif (ndims(n) <= 2 && numel(n) <= 8)
    suffix = '';
    % if it is a column vector, write it as a transposed row vector
    if (size(n,2) == 1)
      n = n.';
      suffix = '.''';
    end
    s = '[ ';
    for i=1:size(n,1)
      for j=1:size(n,2)
        if (j > 1)
          s = [s '  '];
        end
        s = [s  num2str(n(i,j)) ];
      end
      if (i < size(n,1))
        s = [s ' ; '];
      end
    end
    s = [s ' ]' suffix];
  elseif (ndims(n) == 2 && size(n,2) == 1)
    s = ['<' num2str(numel(n)) '>'];
  else
    s = ['<' num2str(size(n,1)) ];
    for i=2:ndims(n)
      s = [ s 'x' num2str(size(n,i)) ];
    end
    s = [s '>'];
  end
  
end
