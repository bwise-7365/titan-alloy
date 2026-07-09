% =============================================================================
% damped Newton (Levenberg-Marquardt) method.
% Given a vector function, F:R^n -> R^m, seek a vector x s.t. F(x) = 0.
% The Newton part is well-known: given 
% 0 = F + J*d
% Where J is invertible, it is simple.
% More generally, the LLSE estimate is
% -J^T * F = J^T * J * d
%
% The gradient search part defines f = 1/2 F dot F
% so grad(f) = F^T * J as a [1,n] vector
% d is the transpose of that, so 
% -J^T * F = alpha * d, for scalar alpha.
% we choose alpha so that
% 0 = f + grad(f)*d
%   = f + alpha * g dot g
% -f / (g dot g) = alpha
%
% Using theta as (my favorite) smoothing parameter, they combine to make this:
% -J^T * F = [theta * J^T * J  +  (1-theta) * alpha * I] * d
% -----------------------------------------------------------------------------
function [x0, mag0, iter] = dNewton(xStart, Fn, f2Tol, maxIter, iterFreq)
  x0 = xStart;
  F0 = Fn(x0);
  mag0 = dot(F0, F0);
  iter = 0;
  numXDim = mustBeColVec(x0) ;


  % A value >> 1.0 (e.g. 2.8) is just to exercise Armijo search
  % A value << 1.0 (e.g. 0.6) is just to exercise the Newton steps
  % Some problems do much better with lambda slightly below 1
  lambdaBase = 0.75;  %0.618034 ;

  rho = 0.80 ;
  ident = eye(numXDim);
  notDone = ((f2Tol < mag0)&(iter < maxIter));
  maxArmijo = 70; % 0.8^70 is about 1.65e-7
  
  theta = 0.618034;

  %printf("Start point has mag %.4e \n",  mag0);
  %fprintf("%9.4f ", transpose(x0));
  %printf("\n\n");

  while (notDone)
    % raise errors if we have somehow gotten NAN values
    mustBeNonNan(x0);
    mustBeNonNan(F0);
    mustBeNonNan(mag0);

    JF = jacobs (x0, Fn);
    JT = transpose(JF);
    JTJ = JT * JF ;
    gr = JT*F0;
    littleF = dot(F0, F0)/2.0;
    alpha = littleF / dot(gr, gr);
    d0 = -linsolve( theta*JTJ + (1-theta)*alpha*ident, gr);

    lambda = lambdaBase ;
    x1 = x0 + (lambda * d0);
    F1 = Fn(x1);
    mag1 = dot(F1,F1);
    mmRatio = mag1/mag0;
    % printf("Before Armijo, mag0: %.4e  vs mag1 %.4e for ratio %.3f \n", mag0, mag1, mmRatio);

    numArmijo = 1;
    while ((numArmijo < maxArmijo) & (mag0 < mag1))
      mmRatio = mag1/mag0;
      % printf("Armijo %2d, lambda %.6f , mag0: %.4e  vs mag1 %.4e, ratio %.3f \n",
      %   numArmijo, lambda, mag0, mag1, mmRatio);
      lambda = rho * lambda;
      x1 = x0 + (lambda * d0);
      F1 = Fn(x1);
      mag1 = dot(F1,F1);
      mmRatio = mag1/mag0;
      numArmijo = 1 + numArmijo;
    endwhile
    mag0 = mag1;
    x0 = x1;
    F0 = F1;

    if (maxArmijo <= numArmijo)
      printf("FAILED Armijo search \n");
      mustBeLessThan(numArmijo, maxArmijo); % raise an error if not so
    endif

    if (0 < iterFreq) && (0 == mod(iter, iterFreq)) % show progress?
      printf("dNewton iteration %4d/%4d, Armijo %2d/%2d, %.5e/%.2e \n",
	     iter, maxIter, numArmijo, maxArmijo, mag1, f2Tol);
    endif

    % printf("In iteration %3d, %3d Armijo steps to get mag %.4e with lambda %.4f, ratio %.3e \n",
    %    iter, numArmijo, mag0, lambda, mmRatio);
    % fprintf("%10.5f ", transpose(x0));
    % printf("\n");
    iter = iter + 1;
    notDone = ((f2Tol < mag0)&(iter < maxIter));

    % printf("\n");
  endwhile

  if (maxIter < iter)
    printf("FAILED damped Newton search \n");
    mustBeLessThan(iter, maxIter); % raise an error if not so
  endif


%printf("At end of dNewton, iter %d, mag0 %.3e \n", iter, mag0);
endfunction

% =============================================================================
