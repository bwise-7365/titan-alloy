* ====================================================
* deploy_v09.gms
*
* PPD family, (node, route) = (RATIO, SOFT-SALVO): interdiction
* with SOFTPLUS-SMOOTHED salvo combat along the routes (author's
* proposal, in the spirit of Fischer-Burmeister smoothing):
*     max(0, x)  ->  sp(x) = log(1 + exp(aSm*x))/aSm .
* Route combat, Red route m->k under (r,b):
*     A2s = sp(BAt - REs)              surviving Blue attackers
*     S   = f - A2s + sp(A2s - f)      surviving Red movers
* As aSm -> inf this approaches the exact salvo
* min(max(BAt-REs,0), f); the value error is bounded by
* 2*log(2)/aSm (~0.35 units at aSm = 2).  Everything is smooth,
* so NO rate variables are needed: each side differentiates the
* payoff directly (full Nash), exactly as in deploy_v07, and the
* softplus chain rule delivers regime-correct credits
* automatically (Maxima-verified, v09_check.mac):
*   movers   1 - sig(A2s - f)            (~1 partial, ~0 annihilated)
*   escorts  sig(BAt-REs)*(1-sig(A2s-f)) (~0 absorbed, ~1 partial)
*   attackers -sig(BAt-REs)*(1-sig(A2s-f)) (~0 absorbed AND annihilating)
* This resolves by smoothing the equilibrium-existence failure of
* the corner-sharp (ratio, salvo) model v08 (route hider-and-
* seeker; see salvo-ppd notes Sections 15-15b): smoothness makes
* best responses continuous, as in v07.  aSm dials v07-like
* smoothness (small a) toward exact salvo corners (large a);
* existence may fail again if aSm is pushed too high.
*
* Node combat: plain force ratio among arriving survivors, with
* its own regularizer epsNode (salvo-like routes can drive a
* node's survivors near zero).
*
* Each side's plan under a strategy has three parts:
*   (1) mover flows base -> conflict location (ton-mile limits,
*       as in the base PPD);
*   (2) defenders (escorts) allocated to each friendly flow;
*   (3) attackers allocated to each opposing route.
* Escorts and attackers are highly mobile: no ton-mile cost,
* only per-strategy pool limits.
*
* SMOOTHING THRESHOLD (found by local solves, 2026-07-06, at
* full size -- 5 strategies, 12 routes/side):
*   aSm = 0.25          -> PATH solves (Optimal)
*   aSm = 0.35 and up   -> locally infeasible (existence fails
*                          as the corners sharpen)
* So the default below is aSm = 0.25: smoothing width 2ln2/aSm
* ~ 5.5 force units, comparable to per-route flows.  Read
* physically: the targeting uncertainty needed to stabilize
* deterministic planning is of the order of the convoy sizes.
*
* For NEOS, use vgc6979@vincp.mozmail.com
* ====================================================

$ONSYMLIST

* ----------------------------------------------------
* DATA and PARAMETERS Section
* ----------------------------------------------------

Set m RedLoc  / RL1 , RL2 , RL3 /;
Set n BlueLoc / BL1 , BL2 , BL3 /;
Set k CLoc    / CL1 , CL2 , CL3 , CL4 /;

Set r RedStrat  / RS1 , RS2 , RS3 , RS4 , RS5 /;
Set b BlueStrat / BS1 , BS2 , BS3 , BS4 , BS5 /;

* aliases for sums inside equations already indexed by m or n
Alias (m, mp);
Alias (n, np);


Parameters
  TotalR
  TotalB
  LogR    "Maximum ton-miles for Red movers"
  LogB    "Maximum ton-miles for Blue movers"
  myEps
  rho     "Maximum probability of any given strategy"
;

TotalR =  35.0;
TotalB =  37.0;

LogR   = 421.0;
LogB   = 410.0;

myEps  =   0.01;

rho = 0.6;

* ------------------
* Escort and attacker pools (per strategy; highly mobile, so no
* ton-mile cost).  Sized to matter against per-route flows of
* roughly 2 to 13 units.

Scalars
  TotREsc "Red defenders available to escort own flows"   / 10.0 /
  TotRAtt "Red attackers available against Blue routes"   / 12.0 /
  TotBEsc "Blue defenders available to escort own flows"  / 12.0 /
  TotBAtt "Blue attackers available against Red routes"   / 11.0 /

  aSm     "softplus smoothing sharpness (value error <= 2ln2/aSm)" / 0.25 /
  epsNode "node-contest regularizer (routes can empty a node)"     / 0.5 /
;

* VR, VB: value of location k under the given strategy (as in
* the base PPD).

 Table Vr(r,k)
       CL1    CL2    CL3    CL4
 RS1  31.8   24.2   15.4   28.4
 RS2  22.7   15.3   26.6   35.3
 RS3  10.8   30.1   22.5   36.6
 RS4  15.5   12.6   33.2   38.7
 RS5  32.9   34.7   24.1    8.3
 ;

 Table DistR(m,k)
       CL1    CL2    CL3   CL4
 RL1  11.0   13.0   17.0  20.0
 RL2  17.0   13.0   14.0  16.0
 RL3  18.0   16.0   13.0  10.0
 ;


 Table Vb(b,k)
       CL1   CL2    CL3   CL4
 BS1   25.2  20.1   13.0  41.7
 BS2   33.4  33.1    8.3  25.2
 BS3   28.1  16.8   25.9  29.2
 BS4   10.1  43.6   14.5  31.8
 BS5   39.8  14.4   28.0  17.8
 ;

 Table DistB(n,k)
       CL1    CL2    CL3   CL4
 BL1  10.0   13.0   15.0  17.0
 BL2  14.0   12.0   13.0  15.0
 BL3  20.0   18.0   15.0  11.0
 ;


* ----------------------------------------------------
* MODEL Definition
* ----------------------------------------------------

* ------------------
* DECISION VARIABLES

Positive Variables
    RD(m)  "Red movers pre-positioned at location m"
    pr(r)
    flowR(r, m, k) "Red mover flow m->k under strategy r"

    BD(n)   "Blue movers pre-positioned at location n"
    pb(b)
    flowB(b, n, k)  "Blue mover flow n->k under strategy b"

    REs(r,m,k) "Red defenders escorting the flow m->k under r"
    RAt(r,n,k) "Red attackers against Blue route n->k under r"
    BEs(b,n,k) "Blue defenders escorting the flow n->k under b"
    BAt(b,m,k) "Blue attackers against Red route m->k under b"
    ;

* ------------------
* Softplus, sigmoid, survivors, and exact gradients (macros
* expand in place; Maxima-verified in v09_check.mac).

$macro spx(x) (log(1.0 + exp(aSm*(x)))/aSm)
$macro sgx(x) (1.0/(1.0 + exp(-aSm*(x))))

$macro BA2x(r,b,m,k)   (spx(BAt(b,m,k) - REs(r,m,k)))
$macro fRsurv(r,b,m,k) (flowR(r,m,k) - BA2x(r,b,m,k) + spx(BA2x(r,b,m,k) - flowR(r,m,k)))
$macro FRnode(r,b,k)   (sum(mp, fRsurv(r,b,mp,k)))

$macro RA2x(r,b,n,k)   (spx(RAt(r,n,k) - BEs(b,n,k)))
$macro fBsurv(r,b,n,k) (flowB(b,n,k) - RA2x(r,b,n,k) + spx(RA2x(r,b,n,k) - flowB(b,n,k)))
$macro FBnode(r,b,k)   (sum(np, fBsurv(r,b,np,k)))

$macro denx(r,b,k)     (FRnode(r,b,k) + FBnode(r,b,k) + epsNode)

* d(fhatR)/d(flowR); d(fhatR)/d(REs) = -d(fhatR)/d(BAt):
$macro dfR_df(r,b,m,k) (1.0 - sgx(BA2x(r,b,m,k) - flowR(r,m,k)))
$macro dfR_dE(r,b,m,k) (sgx(BAt(b,m,k) - REs(r,m,k))*(1.0 - sgx(BA2x(r,b,m,k) - flowR(r,m,k))))
$macro dfR_dA(r,b,m,k) (dfR_dE(r,b,m,k))

* Blue mirrors:
$macro dfB_dg(r,b,n,k) (1.0 - sgx(RA2x(r,b,n,k) - flowB(b,n,k)))
$macro dfB_dE(r,b,n,k) (sgx(RAt(r,n,k) - BEs(b,n,k))*(1.0 - sgx(RA2x(r,b,n,k) - flowB(b,n,k))))
$macro dfB_dA(r,b,n,k) (dfB_dE(r,b,n,k))

* ------------------
* Upper bounds (valid: pool/total rows imply them at solutions;
* they also keep exp(aSm*x) safely bounded at every iterate).

flowR.UP(r,m,k) = TotalR;
flowB.UP(b,n,k) = TotalB;
REs.UP(r,m,k) = TotREsc;
RAt.UP(r,n,k) = TotRAtt;
BEs.UP(b,n,k) = TotBEsc;
BAt.UP(b,m,k) = TotBAtt;

* ------------------
* Not-insane initial values (interior helps PATH's crash phase).

RD.L(m)  = TotalR / (card(m) + 1.0);
pr.L(r)   = 1.0 /( card(r) + 1.0 );
BD.L(n)  = TotalB / (card(n) + 1.0);
pb.L(b)   = 1.0 / (card(b) + 1.0);

flowR.L(r,m,k) = TotalR / (card(m)*card(k));
flowB.L(b,n,k) = TotalB / (card(n)*card(k));

REs.L(r,m,k) = TotREsc / (card(m)*card(k) + 1.0);
RAt.L(r,n,k) = TotRAtt / (card(n)*card(k) + 1.0);
BEs.L(b,n,k) = TotBEsc / (card(n)*card(k) + 1.0);
BAt.L(b,m,k) = TotBAtt / (card(m)*card(k) + 1.0);


* ------------------
* SHADOW PRICES / OPPORTUNITY COST
*
* Lagrange Multipliers of =e= constraints can have any sign.
* L. M. of =g= constraints must be positive.

Variables
 alphaR
 alphaB

 lambdaR
 lambdaB
 ;

Positive Variables

 etaR(r)
 etaB(b)

 betaR(r)
 betaB(b)

 gammaR(r,m)
 gammaB(b,n)

 muER(r) "shadow price of the Red escort pool under r"
 muAR(r) "shadow price of the Red attacker pool under r"
 muEB(b) "shadow price of the Blue escort pool under b"
 muAB(b) "shadow price of the Blue attacker pool under b"
 ;

* ------------------
* MODEL EQUATIONS
*
* Cost-Benefit comparisons for every decision variable, with the
* exact smooth gradients; constraint slackness for every
* multiplier.

Equations

  C_B_Red_Prob
  C_B_Blue_Prob

  C_B_Red_flow
  C_B_Blue_Flow

  C_B_Red_Esc
  C_B_Blue_Esc

  C_B_Red_Att
  C_B_Blue_Att

  C_B_Red_Dep
  C_B_Blue_Dep

  L_M_Red_Dep     'Red pre-positioning must exactly equal total available'
  L_M_Blue_Dep    'Blue pre-positioning must exactly equal total available'

  L_M_Red_Prob    'Red probabilities add to exactly one'
  L_M_Blue_Prob   'Blue probabilities add to exactly one'

  L_M_Red_Rho      'Red probability cannot exceed maximum'
  L_M_Blue_Rho     'Blue probability cannot exceed maximum'

  L_M_Red_Log      'Red movement plans cannot exceed logistical capacity'
  L_M_Blue_Log     'Blue movement plans cannot exceed logistical capacity'

  L_M_Red_Flow     'Red amount moved from a location cannot exceed what was pre-positioned there'
  L_M_Blue_Flow    'Blue amount moved from a location cannot exceed what was pre-positioned there'

  L_M_Red_Esc      'Red escorts allocated under r cannot exceed the pool'
  L_M_Blue_Esc     'Blue escorts allocated under b cannot exceed the pool'

  L_M_Red_Att      'Red attackers allocated under r cannot exceed the pool'
  L_M_Blue_Att     'Blue attackers allocated under b cannot exceed the pool'

;

* Stationarity: marginal cost vs exact marginal benefit.

C_B_Red_Prob(r)..   alphaR + betaR(r)
   - sum((b,k), pb(b)*VR(r,k)*FRnode(r,b,k)/denx(r,b,k)) =g= 0;

C_B_Blue_Prob(b)..  alphaB + betaB(b)
   - sum((r,k), pr(r)*VB(b,k)*FBnode(r,b,k)/denx(r,b,k)) =g= 0;

C_B_Red_Flow(r,m,k)..   etaR(r)*DistR(m,k) + gammaR(r,m)
   - pr(r)*sum(b, pb(b)*VR(r,k)
       *(FBnode(r,b,k) + epsNode)/sqr(denx(r,b,k))*dfR_df(r,b,m,k)) =g= 0;

C_B_Blue_Flow(b,n,k)..  etaB(b)*DistB(n,k) + gammaB(b,n)
   - pb(b)*sum(r, pr(r)*VB(b,k)
       *(FRnode(r,b,k) + epsNode)/sqr(denx(r,b,k))*dfB_dg(r,b,n,k)) =g= 0;

C_B_Red_Esc(r,m,k)..   muER(r)
   - pr(r)*sum(b, pb(b)*VR(r,k)
       *(FBnode(r,b,k) + epsNode)/sqr(denx(r,b,k))*dfR_dE(r,b,m,k)) =g= 0;

C_B_Blue_Esc(b,n,k)..  muEB(b)
   - pb(b)*sum(r, pr(r)*VB(b,k)
       *(FRnode(r,b,k) + epsNode)/sqr(denx(r,b,k))*dfB_dE(r,b,n,k)) =g= 0;

C_B_Red_Att(r,n,k)..   muAR(r)
   - pr(r)*sum(b, pb(b)*VR(r,k)
       *FRnode(r,b,k)/sqr(denx(r,b,k))*dfB_dA(r,b,n,k)) =g= 0;

C_B_Blue_Att(b,m,k)..  muAB(b)
   - pb(b)*sum(r, pr(r)*VB(b,k)
       *FBnode(r,b,k)/sqr(denx(r,b,k))*dfR_dA(r,b,m,k)) =g= 0;

C_B_Red_Dep(m)..   lambdaR - sum(r, gammaR(r,m)) =g= 0;
C_B_Blue_Dep(n)..  lambdaB - sum(b, gammaB(b,n)) =g= 0;

* Primal feasibility.

L_M_Red_Dep..   TotalR - sum(m, RD(m)) =e= 0;
L_M_Blue_Dep..  TotalB - sum(n, BD(n)) =e= 0;

L_M_Red_Prob..   1.0 - sum(r, pr(r))  =e=  0;
L_M_Blue_Prob..  1.0 - sum(b, pb(b))  =e=  0;

L_M_Red_Rho(r)..   rho - pr(r) =g= 0;
L_M_Blue_Rho(b)..  rho - pb(b) =g= 0;

L_M_Red_Log(r)..   LogR - sum((m,k), DistR(m,k)*flowR(r,m,k)) =g= 0;
L_M_Blue_Log(b)..  LogB - sum((n,k), DistB(n,k)*flowB(b,n,k)) =g= 0;

L_M_Red_Flow(r,m)..   RD(m) - sum(k, flowR(r,m,k)) =g= 0;
L_M_Blue_Flow(b,n)..  BD(n) - sum(k, flowB(b,n,k)) =g= 0;

L_M_Red_Esc(r)..   TotREsc - sum((m,k), REs(r,m,k)) =g= 0;
L_M_Blue_Esc(b)..  TotBEsc - sum((n,k), BEs(b,n,k)) =g= 0;

L_M_Red_Att(r)..   TotRAtt - sum((n,k), RAt(r,n,k)) =g= 0;
L_M_Blue_Att(b)..  TotBAtt - sum((m,k), BAt(b,m,k)) =g= 0;


Model interdict /
      C_B_Red_prob.pr
      C_B_Blue_Prob.pb

      C_B_Red_Flow.flowR
      C_B_Blue_Flow.flowB

      C_B_Red_Esc.REs
      C_B_Blue_Esc.BEs

      C_B_Red_Att.RAt
      C_B_Blue_Att.BAt

      C_B_Red_Dep.RD
      C_B_Blue_Dep.BD

      L_M_Red_Prob.alphaR
      L_M_Blue_Prob.alphaB

      L_M_Red_Rho.betaR
      L_M_Blue_Rho.betaB

      L_M_Red_Dep.lambdaR
      L_M_Blue_Dep.lambdaB

      L_M_Red_Log.etaR
      L_M_Blue_Log.etaB

      L_M_Red_Flow.gammaR
      L_M_Blue_Flow.gammaB

      L_M_Red_Esc.muER
      L_M_Blue_Esc.muEB

      L_M_Red_Att.muAR
      L_M_Blue_Att.muAB

      /;

* Multiple Nash equilibria are expected, as in v04c/v06; the
* cold-start property makes interior initial values essential.

Option MCP = PATH;

interdict.optfile = 0 ;

Solve interdict using MCP ;

* ------------------
* OUTPUT parameters

Parameter
 TotalRD
 TotalBD
 LogUsedR
 LogUsedB
 EscUsedR(r)
 AttUsedR(r)
 EscUsedB(b)
 AttUsedB(b)
 SentR(r)        "movers dispatched under r"
 SentB(b)
 ArriveR(r,b)    "Red movers arriving, given (r,b)"
 ArriveB(r,b)
 FhRL(r,b,k)
 FhBL(r,b,k)
 PayoffR_RB(r,b)
 PayoffB_BR(b,r)
 PayoffR_R(r)
 PayoffB_B(b)
 PayoffR
 PayoffB
 ;

TotalRD = sum(m, RD.L(m));
TotalBD = sum(n, BD.L(n));
LogUsedR(r) = sum((m,k), DistR(m,k)*flowR.L(r,m,k));
LogUsedB(b) = sum((n,k), DistB(n,k)*flowB.L(b,n,k));
EscUsedR(r) = sum((m,k), REs.L(r,m,k));
AttUsedR(r) = sum((n,k), RAt.L(r,n,k));
EscUsedB(b) = sum((n,k), BEs.L(b,n,k));
AttUsedB(b) = sum((m,k), BAt.L(b,m,k));
SentR(r) = sum((m,k), flowR.L(r,m,k));
SentB(b) = sum((n,k), flowB.L(b,n,k));

Parameter A2RL(r,b,m,k), A2BL(r,b,n,k);
A2RL(r,b,m,k) = log(1.0 + exp(aSm*(BAt.L(b,m,k) - REs.L(r,m,k))))/aSm;
A2BL(r,b,n,k) = log(1.0 + exp(aSm*(RAt.L(r,n,k) - BEs.L(b,n,k))))/aSm;

FhRL(r,b,k) = sum(m, flowR.L(r,m,k) - A2RL(r,b,m,k)
   + log(1.0 + exp(aSm*(A2RL(r,b,m,k) - flowR.L(r,m,k))))/aSm);
FhBL(r,b,k) = sum(n, flowB.L(b,n,k) - A2BL(r,b,n,k)
   + log(1.0 + exp(aSm*(A2BL(r,b,n,k) - flowB.L(b,n,k))))/aSm);

ArriveR(r,b) = sum(k, FhRL(r,b,k));
ArriveB(r,b) = sum(k, FhBL(r,b,k));

PayoffR_RB(r,b) = sum(k, VR(r,k)*FhRL(r,b,k)/(FhRL(r,b,k) + FhBL(r,b,k) + epsNode));
PayoffB_BR(b,r) = sum(k, VB(b,k)*FhBL(r,b,k)/(FhRL(r,b,k) + FhBL(r,b,k) + epsNode));

PayoffR_R(r) = sum(b, pb.L(b)*PayoffR_RB(r,b));
PayoffB_B(b) = sum(r, pr.L(r)*PayoffB_BR(b,r));

PayoffR = sum(r, pr.L(r) * PayoffR_R(r));
PayoffB = sum(b, pb.L(b) * PayoffB_B(b));

* ------------------
* GHOST ACCOUNTING (reporting only): the softplus overstates
* survivors by up to ln(2)/aSm per route ("phantom forces",
* largest on empty and unthreatened routes).  Recompute arrivals
* under the EXACT salvo at the same decisions, and report the
* difference, so every listing shows the fog the smoothing
* bought existence with.

Parameter
 FhRLx(r,b,k)  "exact-salvo Red arrivals at the same decisions"
 FhBLx(r,b,k)
 GhostR(r,b)   "phantom Red arrivals = smoothed - exact"
 GhostB(r,b)
 ;

FhRLx(r,b,k) = sum(m, max(0.0, flowR.L(r,m,k)
   - max(0.0, BAt.L(b,m,k) - REs.L(r,m,k))));
FhBLx(r,b,k) = sum(n, max(0.0, flowB.L(b,n,k)
   - max(0.0, RAt.L(r,n,k) - BEs.L(b,n,k))));

GhostR(r,b) = ArriveR(r,b) - sum(k, FhRLx(r,b,k));
GhostB(r,b) = ArriveB(r,b) - sum(k, FhBLx(r,b,k));

Display
 pr.L, pb.L,
 RD.L, BD.L,
 TotalRD, TotalBD,
 LogUsedR, LogUsedB,
 EscUsedR, AttUsedR, EscUsedB, AttUsedB,
 SentR, SentB,
 ArriveR, ArriveB,
 GhostR, GhostB,
 PayoffR_R, PayoffB_B,
 PayoffR, PayoffB
 ;

* ====================================================
