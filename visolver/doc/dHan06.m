% ==============================================================================
% (Attempt to) solve linear variational inequality (LVI) using
% "Solving linear variational inequality problems by a self-adaptive projection method",
% Deren Han 2006

% x0 is an [n,1] starting vector.
% M is an [n,n] matrix
% q is an [n,1] vector
% Pr is a projector onto a convex set in n-space.
%
% Seems faster (e.g. 1/2 to 1/3 as many iterations) than bsHe94b on difficult,
% many-iteration steps of SAOE, and slightly slower (e.g. 10% more iterations)
% of easy, few-iteration steps of SAOE.
% ------------------------------------------------------------------------------
function [x, mag, iter] = dHan06 (x0, M, q, Pr, magTol, iterMax, iterFreq)

% 0 < gamma < 2
gamma = 1.6;

% unexplained constant in Han's paper, with value 1.0
mu = 1.05;

% Beta must be positive
beta0 = 0.5 ;
bk = beta0;

nd = mustBeColVec(x0);
myEps = 1.0e-8;

% All tau_k positive, with finite sum
tau0 = 0.5;
tk = tau0;

I = eye(nd);
x = x0;


% the tau(k) sequence must all be positive with finite sum
% e.g. tau(k) = 25^2 / (25^2 * k^2)
% or 0.5 for 50 iterations and zero thereafter

mag = 1 + magTol;
iter = 0;
doneP = false;
initMag = -1.0;

while (!doneP)
  tk = tau(tau0, 10, iter);

  % Definition of 'e', just before equation (3)
  p = Pr(x - bk * (M*x+q));
  e = x - p;
  mag = dot(e,e);


  if (initMag < 0.0)
    initMag = mag;
  endif

  % raise errors if we have somehow gotten NAN values
  mustBeNonNan(mag);
  mustBeGreaterThan(100*initMag, mag); % catch out of control divergence

  if (0 < iterFreq) && (0 == mod(iter, iterFreq))
    printf("dHan06 iteration %4d/%4d, %.3e/%.3e \n", iter, iterMax,  mag, magTol);
  endif


  % Equation (10) of the paper
  omegaNum = bk * M * e;
  omegaNum = sqrt (dot (omegaNum, omegaNum)); % i.e. |βk * M * e|
  omegaDnm = sqrt(mag); % i.e. |e| = sqrt (dot(e,e));
  omega = omegaNum/omegaDnm;

  % Equation (11) of the paper
  if (omega < 1/(1+mu))
    bk = bk * (1+tk);
  endif
  if (omega > 1 + mu)
    bk = bk / (1+tk);
  endif
    %% leave beta unchanged otherwise


  % Equation (4) of the paper
  y = linsolve(I +  bk*M, e);
  x = x - gamma*y;

  iter++;
  doneP = (mag < magTol) || (iterMax < iter);
endwhile
% end of main loop

endfunction

% sub-functions

function tk = tau(t0, n, k)
  tk = t0;
  if (n < k)
    tk = (2.0 * t0 * n * n) / ((n*n) + (k*k));
  endif
endfunction


% ==============================================================================
