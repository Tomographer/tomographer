function P = ProjPauli(n, s, v)
% Get the Projector to an eigenspace of the nth Pauli operator.
%
%   P = ProjPauli(n,s)
%   P = ProjPauli(n,s,'v')
%
%   n:  1,2,3 the Pauli operator
%   s:  +1/-1 the eigenspace on which to project
%
% The form ProjPauli(n,s) returns a projector as a 2x2 matrix (or a tensor product of
% 2x2 matrices, if n and s are arrays).
%
% The form ProjPauli(n,s,'v') returns a normalized vector in the direction the
% corresponding projector would project on. Note that all projectors returned by
% ProjPauli(n,s) are rank-1.
%
% Notes.
%   * if s is zero or +2, it is considered as -1. (Rationale for +2: index
%     the projectors as ProjPauli(n,1) and ProjPauli(n,2) in for loops for
%     example. In fact, any other value to s than +1 is the same as s=-1.
%   * if n=0 or n=4 (conventionally the identity), s=1 projects onto the
%     first basis vector and otherwise projects onto the second basis
%     vector, like for n=3. In fact, n=0 or n=4 is treated like n=3.
%
% If n & s are 1-D arrays of the same length, then the tensor product of all the
% specified pairs of pauli projectors is returned. n & s can be cell arrays of
% integers, too.
%
  
  assert(numel(n) == numel(s), 'n & s must have same sizes!');

  if (nargin < 3)
    v = [];
  end

  if (iscell(n))
    n = cell2mat(n);
  end
  if (iscell(s))
    s = cell2mat(s);
  end
  
  if(numel(n) == 1)
    P = get_pauli_proj(n,s,v);
    return
  end
  
  P = 1;
  for ind=1:numel(n)
    P = kron(P, get_pauli_proj(n(ind), s(ind), v));
  end
  
end
  
function P = get_pauli_proj(n, s, v)
  if (n == 1) %Sigma_X
    if (s == 1) % "+"
      P = get_proj([ 1; 1 ]/sqrt(2), v);
      return;
    else % "-"
      P = get_proj([ 1; -1 ]/sqrt(2), v);
      return;
    end
  elseif (n == 2) %Sigma_Y
    if (s == 1) % "0 +i 1"
      P = get_proj([ 1; 1i ]/sqrt(2), v);
      return;
    else % "0 -i 1"
      P = get_proj([ 1; -1i ]/sqrt(2), v);
      return;
    end
  elseif (n == 3 || n == 0 || n == 4) % Sigma_Z, like Identity
    if (s == 1) % "0"
      P = get_proj([ 1; 0], v);
      return;
    else % "1"
      P = get_proj([ 0; 1], v);
      return;
    end
  else
    display(n);
    error('ProjPauli:Badarg', sprintf('Bad Argument: %s', dispstr(n)));
  end
end


function P = get_proj(Vect, v)
  if (ischar(v) && strcmp(v, 'v'))
    P = Vect;
  else
    P = Vect*Vect';
  end
end
