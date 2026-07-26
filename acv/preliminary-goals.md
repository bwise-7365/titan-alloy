# Goals

This directory holds work relating to a hierarchical Bayesian inference problem,
and how to write it up in a LaTeX paper.

Read and follow the instructions in C:\repos\ghub-per\CPVI\style-instructions.md

For LaTeX documents, 'include' the C:\repos\ghub-per\visolver\doc\report\prolog.tex
to define style, macros, colors, etc.

All algebra should be explicitly verified by MAXIMA. Keep the MAC files.

I want you to do the hierarchical Bayesian inference problem which is an
extension of the AUTOCLASS formalism. See the papers and source code
in C:\Library\AutoClass

Do not write any C++ code to compute anything until after the theory is fully
worked out and codified in a LaTeX document.

This 'preliminary goals' document covers ONLY the first, short LaTeX document
which I will use to make sure we have worked out a clear and tractable problem
statement. It will be superseded by later guidance for later documents.
Nothing in this document is to be taken as a final and non-negotiable requirement.
Its purpose is to start the dialog toward a clear problem statement,
not to provide one up-front.

## Document Structure.

Write for the "educated layman". Do not use highly compressed, jargon-heavy terminology that requires
the reader to consult several other papers in order to read this one. Do not use corporate or software developer
jargon. Write as if writing to a professional audience in the pre-computer era of the 1950's.

State Bayes's theorem in a numbered LaTeX formula:
P(A|B) = P(B|A) * P(A) / P(B), where P(B) = sum/integral over a of P(B|a)P(a)

Point out that we can choose A to maximize P(A|B) without calculating P(B).
That is maximum posterior likelihood estimation (MPLE) and that is what this paper will do.

Clearly state, with a minimum of jargon, what is our space of A: the space of square
matrices with positive or negative real values. State clearly, with a minimum of jargon,
what is the prior over those matrices. Suppose the count of non-zero entries is M and
we assume the scale-free improper distribution 1/a for the terms A_ij in the matrix.
Given 'p', the prior probability of a transition matrix is thus 

P(A|p) = p^M * product over non-zero terms 1/|A_ij|.

The hyper-prior over p is U[0,1]. Use Bayesian inference.

Our data set, B,  is T pairs (Y,X) of N dimensional real vectors.

Y_it = sum_j A_ij * X_jt

We assume, for now, that the errors in Y_it are all independent, identically distributed
with mean 0 and standard deviation 'sigma'. To be scale-free, the hyper-prior over sigma is the improper
scale-free distribution 1/sigma.

Thus, given p and sigma, 
P(B|A) is the probability of those conditionally independent errors (conditioned on a given 'A' matrix)

Z_it is the expected value of Y_it, given 'A': Z_it = sum_j A_ij * X_jt

P(B|A, sigma) = product over i and t of exp(-(Z_it - Y_it)^2 / 2*sigma^2) / (sigma * sqrt(2*pi))

Point out to the reader that the sqrt(2*pi) terms do not affect the MPLE and can be ignored.

Now combine the above formulae to get the hierarchical Bayesian term for the posterior
probability of (A, p, sigma) given (Y,X).

Show the integral over p and sigma to get the posterior probability of a matrix A.

Find the maximum likelihood estimate of A.


Verify the above algebra with MAXIMA.