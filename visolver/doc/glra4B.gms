* ====================================================
* Global Logistics as Resource Allocation (GLRA)
* See the paper "Global Logistics as Resource Allocation"
* by Ben P Wise for explanation of goals, terms, logic, etc.
*
* Compared to glra3, this version introduces explicit penalty for using transporation assets.
* I also set rho=0 to ensure that there was no lossage on long routes,
* i.e. that the entire motive for efficient routing was the penalty in the objective.
*
* In this test problem, the total supply is only about 90% of the total required.
* There will always be shortfalls.
* There is no incentive to limit the usage of shipping.
* If facp = 500 so that node-node flow is nonbinding,
* then it seems to take at least MoveTotalMax = 26,114
* to move everything as best as possible. But if the MoveTotalMax is raised,
* it will use very inefficient routes. Introducing a little resource-usage to move
* resources, e.g. fractional loss rho = 3-5% over example routes, would probably fix this.
*
* Interesting overall parameters:
*
* facp = 500, MoveTotalMax = 26114.
* the maximum flow is 200 going from one supply to the biggest demand.
*
* facp: 145 forces some diversions, hence requires MoveTotalMax = 27527 to partially meet all demands.
*
* with facp = 145 and rho = 1/5000, it uses 27363.663 ton-miles even excess is available.
* As each link in the sample graph is 10 units long, this imposes 0.5% loss along each link,
* which hurts the objective function.
* ====================================================
*
* The default working directory is the location of the GAMS binary executable.
* That is usually not desirable.
* The solution in GAMS is to hard-code a specific directory,
* probably the one where this file is stored.
* I regard this as very bad practice, but it seems to be required.

$ONSYMLIST


$include glra4B.inc

* Derived parameters
Parameters
 tinyR        'small constant to avoid division by zero'
 Wght(nj)     'weight of requirement-fulfillment at a node'
 rho          'small loss per unit distance'
 MoveTotalMax 'total ton-milage movement possible'
 MoveTotalRef 'reference estimate of ton-milage from gravity model'
 mu           'normalizing weight for movement penalty'
 sigma(ni,nj) 'throughput combining tau and distance'
 ;

* tinyR avoids division by zero when Rqrt=0. It is roughly mean(Rqrt)/10000
tinyR = 0.015;

* simpliification to avoid tedious algebra; see the PDF
Wght(nj) = Value(nj)/((tinyR+Rqrt(nj)) * (tinyR+Rqrt(nj)));

* rho = 1/10000 means about 0.1% loss per 10-mile link traversed
rho = 0.0;

* Maximum ton-miles (or sqr-ft miles, etc) that can be moved
MoveTotalMax = 50000;

MoveTotalRef = 33560.0;

mu = 2.2097E-11;

sigma(ni,nj) = tau(ni,nj) * (2.71828183 ** (-rho*dist(ni,nj)));

Display tinyR, MoveTotalMax;

* ----------------------------------------------------
* MODEL Definition
* ----------------------------------------------------

* ------------------
* DECISION VARIABLES

* Initial-decision variables with not-insane initial values
Positive Variables
    Qnty(nj)      'Quantity extracted from each node'
    Dlvr(nj)      'Delivery to each node'
    flow(ni, nj)  'Flow from ni to nj'
    ;
Qnty.L(nj)     = SCap(nj) / 2.0;
Dlvr.L(nj)     = Rqrt(nj) / 2.0;
flow.L(ni, nj) = fcap(ni, nj) / 10.0;


* Lagrange multipliers, shadow prices, opportunity costs
Positive Variable
   alpha(ni)     'LM of supply limit at node'
   beta(ni, nj)  'LM of capacity limit of flow between nodes'
   lambda        'LM of total movement limit'
   phi(nj)       'LM of flow balance at a node'
   eta(nj)       'LM of stockpile limit at a node'
;


* ------------------
* MODEL EQUATIONS

Equations
    CompDlvr(nj)      'Delivery MC-MB comparison, perp to Dlvr'
    CompQnty(nj)      'Quantity MC-MB comparison, perp to Qnty'
    CompFlow(ni,nj)   'Flow MC-MB comparison, perp to flow'

    SlackQnty(nj)     'Slack in quantity removed from node j, perp to alpha'
    SlackFlow(ni,nj)  'Slack in flow limit from node i to node j, perp to beta'
    SlackMovement     'Slack in total movement, perp to lambda'
    SlackBalance(nj)  'Slack in flow balance at node j, perp to phi'
    SlackSP(nj)       'Slack in stock pile limit at node j, perp to eta'
;

CompDlvr(nj)..      phi(nj) -  (eta(nj) + 2.0 * Wght(nj)*(Rqrt(nj) - Dlvr(nj)))  =g= 0.0;
CompQnty(nj)..      alpha(nj) + eta(nj) - phi(nj) =g= 0.0;
CompFlow(ni,nj)..   (beta(ni,nj) + lambda*dist(ni,nj) + phi(ni) + eta(nj)*sigma(ni,nj) + 2*mu*sum((nn, nm), flow(nn, nm)*dist(nn,nm))) - (eta(ni) + phi(nj) * sigma(ni,nj)) =g= 0.0;

SlackQnty(nj)..     SCap(nj) - Qnty(nj) =g= 0.0;
SlackFlow(ni,nj)..  fcap(ni, nj) - flow(ni, nj) =g= 0.0;
SlackMovement..     MoveTotalMax - sum((ni,nj) , dist(ni,nj)*flow(ni,nj)) =g= 0.0;
SlackBalance(nj)..  (Qnty(nj) + sum(ni, sigma(ni,nj)*flow(ni,nj))) - (sum(nk, flow(nj, nk)) + Dlvr(nj)) =g= 0.0;
SlackSP(nj)..       SPMax(nj) +  (sum(nk, flow(nj, nk)) + Dlvr(nj)) - (Qnty(nj) + sum(ni, sigma(ni,nj)*flow(ni,nj))) =g= 0.0;


Model glra4B / CompDlvr.Dlvr
               CompQnty.Qnty
               CompFlow.flow

               SlackQnty.alpha
               SlackFlow.beta
               SlackMovement.lambda
               SlackBalance.phi
               SlackSP.eta
               /;


* Particularly when it comes to multiple Nash Equilibria,
* different well-vetted solvers can produce different answers.
*
* PATH seems to be fastest on this problem, using the NEOS server
* KNITRO is second, but slightly overfills most.
* MILES is third and has the same good solution quality as PATH and NLPEC
* NLPEC is slowest.
*
*Option MCP = KNITRO;
* Shortfall N000 13.978,   N007  3.256,   N019  2.440,    N024  4.374
* Shortfall N000 13.978,   N007  3.256,   N019  2.440     N024  4.374
* But all other shortfalls were small negatives: N025 -0.945,    N026 -0.909,    N027 -0.848,    N028 -0.903,    N029 -2.109
* EXECUTION TIME       =        1.510 SECONDS
* EXECUTION TIME       =        1.573 SECONDS

Option MCP = MILES;
* Shortfall N000 13.975,    N007  3.257,    N019  2.442,    N024  4.375
* EXECUTION TIME       =        6.724 SECONDS
* EXECUTION TIME       =        6.651 SECONDS

*Option MCP = NLPEC;
* Shortfall N000 13.977,    N007  3.256,    N019  2.440,    N024  4.373
* EXECUTION TIME       =       12.729 SECONDS
* EXECUTION TIME       =       12.750 SECONDS

*Option MCP = PATH;
* Shortfall N000 13.977,    N007  3.256,    N019  2.440,    N024  4.373
* EXECUTION TIME       =        0.734 SECONDS
* EXECUTION TIME       =        0.729 SECONDS

Solve glra4B using MCP ;

* KNITRO delivered 529.171 and had lots of small numbers where 0 was required
*  MILES delivered all 535, apparently violating the ton-mile limit
*  NLPEC delivered 527.556
*  PATH  delivered 527.556

* ------------------
* OUTPUT parameters

Parameter
  TotalUsed     'Total ton-miles used'
  TotalTaken    'Total tons taken from sources'
  TotalDlvrd    'Total tons delivered'
  TotalLossage  'Lossage between sources and sinks'
  Shortfall(nj) 'Shortfall fraction at node'
  ValAchieved   'Value-weighted shortfall'
  Stockage(nj)  'implied stockpile at a node'
  Outflow(nj)   'total flow out of a node'
  Inflow(nj)    'total flow into a node'
  ;

TotalUsed = sum((ni,nj), dist(ni,nj)*flow.L(ni,nj));
TotalTaken = sum(ni, Qnty.L(ni));
TotalDlvrd = sum(ni, Dlvr.L(ni));
TotalLossage = TotalTaken - TotalDlvrd;
Shortfall(nj) = 100.0 * (Rqrt(nj) - Dlvr.L(nj))/(Rqrt(nj) + tinyR);
ValAchieved  = 100000.0 * sum(nj, Wght(nj)*(Rqrt(nj) - Dlvr.L(nj))*(Rqrt(nj) - Dlvr.L(nj)));
Stockage(nj) = (Qnty.L(nj) + sum(ni, sigma(ni,nj)*flow.L(ni,nj))) - (sum(nk, flow.L(nj, nk)) + Dlvr.L(nj));
Inflow(nj)   =  sum(ni, flow.L(ni,nj));
Outflow(nj)  =  sum(nk, flow.L(nj, nk));

Display
  rho, MoveTotalMax , 
  TotalUsed , lambda.L ,
  TotalTaken, TotalDlvrd, TotalLossage ,
  Qnty.L, Dlvr.L , Shortfall , ValAchieved ,
  Stockage , eta.L ,
  Inflow, Outflow ,
  flow.L
  ;

* ====================================================
