% ==============================================================================
% (Attempt to) solve linear variational inequality (LVI) using
% "A new method for a class of linear variational inequalities",
% Bingsheng He 1994
%
% It performs amazingly well for a two-line algorithm.
%
% As of 2023-09, emacs has octave-mode to indent scripts, but
% Octave itself does not formatting routines.
%
% x0 is an [n,1] starting vector.
% M is an [n,n] matrix
% q is an [n,1] vector
% Pr is a projector onto a convex set in n-space.
%
% B. S. He commented around equation 16 that
% "the search direction (6) may lead to slow convergence".
% My experiments on LVI with ellipsoid K suggest that
% equation 6 (bsHe94a) has 4-20 times more iterations as equation 16 (bsHe94b).
% ------------------------------------------------------------------------------
function [x, mag, iter] = bsHe94b (x0, M, q, Pr, magTol, iterMax, iterFreq)

  gamma = 1.6; % between 0 and 2, greater than 1 recommended
  nd = mustBeColVec(x0);
  invMI = inverse(M + eye(nd));
  x = x0;
  mag = 1 + magTol;
  iter = 0;
  doneP = false;
  initMag = -1.0;
  magFactor = 100.0;
  magLimit = 1;

  while (!doneP)
    e = x - Pr(x - (M*x+q));  % Line 1: eqn 4
    x = x - gamma*invMI*e;    % Line 2: eqn 16

    mag = dot(e,e);
    if (initMag < 0.0)
      initMag = mag;
      magLimit = 1.0 + magFactor * initMag;
    endif

    % raise errors if we have somehow gotten NAN values
    mustBeNonNan(mag);
    %printf("\n");
    %printf("limit on mag: %.4e \n", magLimit);
    %printf("current mag: %.4e \n", mag);
    mustBeGreaterThan(magLimit, mag); % catch out of control divergence

    if (0 < iterFreq) && (0 == mod(iter, iterFreq))
      printf("bsHe94b iteration %4d/%4d, %.3e/%.3e \n", iter, iterMax,  mag, magTol);
    endif

    iter++;
    doneP = (mag < magTol) || (iterMax < iter);
  endwhile


endfunction

% no sub-functions

% ==============================================================================
