* ====================================================
* Selection of a force package:
* Chooses one vector of friendly forces.
* Minimizes cost, subject to achieving a minimum
* force ratio across a suite of 'scenarios',
* using a linear measure of strength.
* Win-probability depends on the ratio of
* the squares of linear-strength.
* The 'friendly' side is whichever side is the planner.
* ----------------------------------------------------

$ONSYMLIST

* ----------------------------------------------------
* DATA SECTION
* ----------------------------------------------------

Set ei 'opp, enemy systems'   / 1, 2, 3, 4 /;
Set fj 'own, friendly systems'   / 1, 2, 3, 4, 5 /;
Set sk 'Planning scenarios'  / 1, 2, 3, 4, 5, 6 /;

Scalar
min_pw  'lowest acceptable probability of win' / 0.8 /
;

* Every system is weak in some scenario, so we probably
* cannot count too much on any single system
Table us(fj, sk) 'effectiveness of each own in each scenario'
     1        2      3        4       5       6  
1    1.324   0.531   2.602   2.427   1.093   1.598
2    2.490   1.790   0.129   2.830   2.165   1.827
3    2.166   2.127   1.459   0.669   1.539   2.594
4    2.818   2.955   1.733   2.609   0.369   1.878
5    1.853   2.273   1.958   1.951   1.920   0.620
;

Table vs(ei, sk) 'effectiveness of each opp in each scenario'
     1        2      3        4       5       6  
1    1.312   1.138   2.415   2.877   1.080   2.866
2    2.833   1.708   1.384   2.222   1.253   1.446
3    1.336   1.371   2.743   2.379   1.551   2.003
4    2.989   2.629   1.602   1.751   1.928   2.683
;

Table et(ei, sk) 'Threat level of each opp system in each scenario'
       1       2       3       4       5       6  
1   109.76  113.72  327.87  220.05  198.91  495.88
2   151.35  396.44  444.72  205.67  404.46  100.50
3   103.75  366.95  113.23  458.49  215.25  372.55
4   376.22  179.11  237.90  120.58  246.83  477.60
;

Parameter
cs(fj) 'own cost per system'
/
1  2.970 
2  1.378 
3  2.280 
4  2.480 
5  2.100 
/
;


* ----------------------------------------------------
* MODEL SECTION
* ----------------------------------------------------

* minimum acceptable win probability is mr^2 / (mr^2 + 1)
Parameters min_ratio  'lowest acceptable force ratio' ;
min_ratio = sqrt(min_pw / (1.0 - min_pw)) ;

Parameter  es(sk) 'strength of opp in each scenario' ;
es(sk) = sum(ei, vs(ei,sk)*et(ei, sk));

* to have a win-probability of 0.8 or more, we need
* R^2 / (R^2 + B^2) > 4/5, or
* R^2 > 2 B^2, or
* R > 2 B, which is much more tractable

Positive Variable
fs(fj) 'level of own system'
beta(sk) 'Lagrange multiplier for slack in advantage'
;


Equations
NetCost(fj)      'Net cost of own system'
Advantage(sk)    'Advantage in force ratio'
;

NetCost(fj)..     cs(fj) - sum(sk, beta(sk)*us(fj,sk)) =g= 0;
Advantage(sk)..   sum(fj, us(fj,sk)*fs(fj)) - min_ratio*es(sk)  =g= 0;

Model rp /  NetCost.fs , Advantage.beta /;

Option MCP = MILES;

Solve rp using MCP;

Parameter  finalFS(sk) 'final strength of own in each scenario' ;
finalFS(sk) = sum(fj, us(fj,sk)*fs.L(fj)) ;

Parameter  pwin(sk) 'Probability of own win in each scenario' ;
pwin(sk) = (finalFS(sk)*finalFS(sk)) / ( (finalFS(sk)*finalFS(sk)) + (es(sk)*es(sk)) );


* ----------------------------------------------------
* Extract useful data
* ----------------------------------------------------

Display fs.L, beta.L , es, finalFS, pwin;


* ====================================================
