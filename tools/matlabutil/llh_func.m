function [llh] = llh_func(Nm, Emn, rho)
% Calculate the (-2)-log-likelihood function for the given data, at the given point
%
% Nm and Emn should be the frequency counts, and the POVM effects, respectively, in the
% same format as accepted by `tomorun`.  I.e. Nm(k) should be the number of times the
% POVM effect Emn(:,:,k) was observed.
%
% rho is a quantum state at which the log-likelihood is to be evaluated.
  
  llh = 0;

  % needed for when we use this function with CVX
  iscvx = false;
  if (strcmp(class(sigma), 'cvx'))
    iscvx = true;
  end

  for k=1:numel(N)

    if (N(k) > 0  &&  any(any(En(:,:,k))))
      trprojsigma = real(trace(En(:,:,k) * sigma));
      if (iscvx || abs(trprojsigma) > 1e-6)
        llh = llh  -  2 * N(k) * log(trprojsigma);
      end
    end

  end
  
end
