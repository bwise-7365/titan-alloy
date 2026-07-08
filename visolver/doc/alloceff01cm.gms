* ====================================================
* Nash equilibrium for exertion of influence
* ====================================================
* This uses a mixed linear-quadratic model to avoid concavity.
* alpha = 1 for purely linear, 0 for purely quadratic.
* ====================================================
*
* NOTE WELL: With alpha = 1.0, the answer from NPLEC exactly matches
* that from my saoeJNrn. I could not find any way to make MILES or PATH
* match either NPLEC or Josephy-Newton. As I understand my own JN code
* best, and the only agreement between the four is NPLEC and JN, I
* suspect that PATH and MILES are less reliable than NPLEC for this
* kind of problem. Of course, there are multiple Nash Equilibria.
* They can be found by varying the random seed in saoeJNrn, but none
* agree with MILES or PATH.
*
* For NEOS, use vgc6979@vincp.mozmail.com
*
* ====================================================

$ONSYMLIST

* ------------------
* BASIC DATA and PARAMETERS

Set act actors   / A0, A1, A2, A3, A4, A5 / ;
Set opt options  / P0, P1, P2, P3, P4, P5, P6, P7, P8, P9 / ;

alias(act, ai) ;
alias(opt, pj, pk) ;

Parameter weight(act) 'relative weight of each actor'
/
A0         68.0
A1         66.0
A2        125.0
A3        101.0
A4        127.0
A5         96.0
/ ;

Table reward(act, opt) 'reward to each actor from each option'
     P0       P1       P2       P3       P4        P5        P6       P7        P8       P9
A0  0.00   181.42   -50.43   -26.32    256.02   -21.27   -132.68   -65.12    131.40    14.54
A1  0.00    31.24   -46.53   122.90     39.47   -12.94     50.32    70.03      8.34  -109.78
A2  0.00   -30.11   -48.64   -56.84    -51.50   -80.42   -130.30   -54.20     -8.75    51.97
A3  0.00    29.12   160.04   -27.68     91.49    80.93    117.95    27.88     33.62    72.34
A4  0.00  -199.26  -234.61    67.80   -319.57   270.83    234.85   -14.91   -236.86  -103.50
A5  0.00    78.66   -22.12    14.25    109.23   -17.30    -31.16   -42.61     31.46   -46.13
;


Parameters
    numAct  'number of actors'
    numOpt  'number of options'
    epsilon 'lowest effort on an alternative'
    totalW  'total of all weights'
    alpha   'linear fraction'
    effR    'reference level of net effort'
    ;
numAct  = sum(act, 1.0) ;
numOpt  = sum(opt, 1.0) ;
epsilon = sqrt(sum(act, weight(act)*weight(act))/ numAct) / 1000.0 ;
totalW  = sum(act, weight(act)) ;
alpha   = 1.000 ;
effR    = sum(act, weight(act))/(1.0 + numOpt) ;

* phi-1 <= alpha <= 1.0 , where phi-1 = 0.618034...


* ------------------
* DECISION VARIABLES

* The real decision variables and plausible initial values.
* The initial eff(act,opt) is the analytic center of the constaints
Positive Variables
    beta(act)     'shadow price of influence'
    eff(act, opt) 'effort by an actor for an option'
    ;
beta.L(act)  =  0.50 ;
eff.L(act, opt) = weight(act)/(1.0 + numOpt) ;
eff.up(act,opt) = weight(act);


* Intermediate variables, as decision variables with plausible initial values
Positive Variables
  nfv(opt)   'net effort for an option, as variable'
  sigma(opt) 'total strength for an option, as variable'
  gamma      'sum of all strengths, as variable'
  ;
nfv.L(opt)   = epsilon + sum(act, eff.L(act,opt))  ;
sigma.L(opt) = (alpha*nfv.L(opt)*effR)  +  ((1-alpha)*nfv.L(opt)*nfv.L(opt)) ;
gamma.L      = sum(opt, sigma.L(opt))  ;

Equations
    nfDef(opt)    'definition of net effort for an option'
    sigmaDef(opt) 'definition of total strength for an option'
    gammaDef        'definition of sum of all strength'
    ;
nfDef(opt) ..     nfv(opt) =e= epsilon + sum(act, eff(act,opt)) ;
sigmaDef(opt) ..  sigma(opt) =e= (alpha*nfv(opt)*effR)  +  ((1-alpha)*nfv(opt)*nfv(opt)) ;
gammaDef ..         gamma =e= sum(opt, sigma(opt))  ;

* NOTE WELL: partial(sigma)/partial(nfv) = alpha*effR + 2*(1-alpha)*nfv
* This appears later in MVInf

options decimals=3

* Show initial values
Display
    numAct, numOpt, epsilon, weight, totalW, alpha, effR,
    eff.L,
    nfv.L, sigma.L, gamma.L
    ;


Equations
    EInf(ai)       extra influence left over for an actor
    MVInf(ai,pj)  marginal value of influence for an option
    ;
EInf(ai) ..        weight(ai)  =g= sum(pj, eff(ai,pj)) ;
MVInf(ai,pj)  ..   beta(ai) - (alpha*effR + 2*(1-alpha)*nfv(pj))*sum(pk, sigma(pk)*(reward(ai,pj)-reward(ai,pk)))/(gamma*gamma)  =g= 0;


* ------------------
* DEFINE and SOLVE (we hope)
Model neInf / nfDef.nfv, sigmaDef.sigma, gammaDef.gamma, EInf.beta, MVInf.eff /;

* Options file has to be somewhere like this:
*  "C:\Users\bwise\Documents\gamsdir\projdir\miles.opt"

* As noted above, NLPEC seems to perform the best on this problem.
*Option MCP = KNITRO;
*Option MCP = MILES;
Option MCP = NLPEC;
*Option MCP = PATH;

* Specify options. value '1' means a file suffix of 'opt',
* i.e. use the file named 'miles.opt' with MILES.
neInf.optfile = 1;

Solve neInf using MCP;

* ------------------
* OUTPUT parameters
Parameter
   prob(opt)    'probability an option will occur'
   expVal(act)  'expected value for actor'
   ;

prob(opt) = round(sigma.L(opt) / gamma.L , 3) ;
expVal(act) = sum(opt, prob(opt)*reward(act,opt));

* This shows the parameter nfp as unchanged since definition, as expected.

Display
    eff.L,
    nfv.L, sigma.L, gamma.L
    sigma.L, gamma.L,
    prob, expVal
    ;

* ====================================================
