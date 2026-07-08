* ====================================================
* Very simple partial equilibrium model of energy markets
* ----------------------------------------------------
* This is a very limited model, basically of natural gas alone.
* It is NOT intended for serious policy analysis.
* Its only purpose is to show something vaguely economic
* and clearly not combat.
* No consideration of substitutes for NG, greatly simplified
* topology of pipelines, no consideration of rest of economy,
* 'rest of world' is lumped into one, despite the huge variability,
* shipping rates are not proportional to route-length,
* and so on.
*
* NOTE: the numeric values are very approximate.



$ONSYMLIST

* ----------------------------------------------------
* DATA SECTION
* ----------------------------------------------------
*
* This very simple model uses world wide shipping capacity of 600.
* IGU 2021 World LNG Report, section 'Message from the President
* of the International Gas Union', cites 572 ships, which is just
* a numeric coincidence. Only 531 are actual transport vessels;
* 37 are regassification and 4 are storage. A real model would inclued
* capacity and speed of ships, length of routes, cost per day
* operating (according the IGU 2021 report, $20,000 to $177,000 per
* day depending on type of vessel, type of contract, and market
* conditions at the date each contract was signed).
* For this very simple model, the '600' number is capacity to carry
* the equivalent of 600 Bcm of NG from various producing areas
* to various consuming areas per year.
*
* IEA said US LNG exports to EU exceeded
* RF pipeline in June 2022 at about 5Bcm/month
* or about 60 Bcm/year for both USA-shipping and RF-pipline.
* Estimated from IEA image:
*           RF  USA
* 2021-01  150    6
* 2021-12  126   24
* 2022-01   90   48
* 2022-03  120   66
* 2022-06   60   60
* ----------------------------------------------------

Set is 'producers who ship' / usap, rowp /;
Set ip 'producers who pipe' / nrwy, nthr, eup, rf /;
Set jm 'markets' / eum, rowm /;

Scalar
 wwship   'world wide shipping capacity' / 600 / ;

Parameter
 mcps(is) 'marginal cost for producers who ship'
 / usap 29,
   rowp 30 /
 mcs(is) 'marginal cost of shipping'
 / usap 2,
   rowp 2 /
 mct(jm) 'marginal cost of terminal services'
 / eum  1,
   rowm 1 /
 cps(is) 'capacity producers who ship'
 / usap  50,
   rowp 475 /
 ct(jm) 'capacity of terminals'
 / eum  250,
   rowm 450 /
 mcpp(ip) 'marginal cost for producers who pipe'
 / nrwy  32,
   nthr 32,
   eup   31,
   rf    28 /
 cpp(ip) 'capacity producers who pipe'
 / nrwy   80,
   nthr  35,
   eup    40,
   rf    220 /
;

Parameter
 dbase(jm) 'base year demand'
 / eum  380,
   rowm 500 /
 pbase(jm) 'base year price'
 / eum  33,
   rowm 33 /
 elas(jm) 'elasticity'
 / eum  0.128535,
   rowm 0.128535 /
;

Parameter cpl(ip,jm) 'capacity of pipelines'
/ nrwy  .eum   = 100
  nrwy  .rowm  =   0
  nthr  .eum   =  40
  nthr  .rowm  =   0
  eup   .eum   =  80
  eup   .rowm  =   0
  rf    .eum   = 150
  rf    .rowm  =  50
/ ;

* new RF policy
* were 150 and 50, respectively
*cpl('rf', 'eum')  = 0; 
*cpl('rf', 'rowm') = 80;

* USA response
* were 50 and 2, respectively
*cps('usap') = 150;
*mcs('usap') = 1.0;


* ----------------------------------------------------
* MODEL SECTION
* ----------------------------------------------------

Positive Variable
* prices
  p(jm)          'price in market'
  srate(is,jm)   'shipping rate from producer to market'
  tau(jm)        'terminal fees for market'
* quantities
  dmnd(jm)     'level of demand in market'
  qps(is, jm)  'production from producer via ship to market'
  ship(is, jm) 'shipping offered to producer for market'
  vol(jm)      'volume processed at terminal for market'
  qpp(ip, jm)  'production from producer via pipe to market'
* rents
  rps(is)    'rent at producer who ships'
  rs         'rent in shipping'
  rt(jm)     'rent at terminal'
  rpp(ip)    'rent at producer who pipes'
  rpl(ip,jm) 'pipeline rent'
;

* explicit initial values are needed so that
* the parameters can be initialized
p.l(jm)     = pbase(jm);
*p.l('rowm')    = pbase('rowm');

* Of course, 'parameters' are never re-evaluated.
* They remain forever at the initial values.

* --------------------------
* Constrained profit maximization
* 0 <= q  \perp mc + R - p  => 0
*
* Capacity limits and rents
* 0 <=  R  \per cap - q  => 0
*
Equations
ProdSProfit(is, jm) 'producer profit on route'
ProdPProfit(ip, jm) 'producer profit on pipline'
ShipProfit(is, jm)  'shipper profit on route'
TermProfit(jm)      'terminal profit'
*
ProdSRent(is)       'producer rent'
ProdPRent(ip)       'producer rent'
PipeRent(ip, jm)    'pipeline rent'
ShipRent            'shipper rent'
TermRent(jm)        'terminal rent'
;

ProdSProfit(is, jm).. mcps(is)+srate(is,jm)+tau(jm)+rps(is)-p(jm) =g= 0;
ProdPProfit(ip, jm).. mcpp(ip) + rpp(ip) + rpl(ip,jm) - p(jm)     =g= 0;
ShipProfit(is, jm)..  mcs(is) + rs - srate(is,jm)                 =g= 0;
TermProfit(jm)..      mct(jm) + rt(jm) - tau(jm)                  =g= 0;
*
ProdSRent(is)..       cps(is) - sum(jm, qps(is,jm))               =g= 0;
ShipRent..            wwship - sum((is,jm), ship(is,jm))          =g= 0;
ProdPRent(ip)..       cpp(ip) - sum(jm, qpp(ip,jm))               =g= 0;
PipeRent(ip, jm)..    cpl(ip,jm)-qpp(ip,jm)                       =g= 0;
TermRent(jm)..        ct(jm) - vol(jm)                            =g= 0;

* --------------------------
* Market clearing
* 0 <=  price  \perp  Supply - Demand  => 0
* CES demand curve in each market
Equations
XSupplyShip(is, jm) 'excess ships along a route'
XSupplyTerm(jm)     'excess terminal services'
XSupplyNG(jm)       'excess NG in a market'
;

XSupplyShip(is, jm)..  ship(is,jm) - qps(is,jm)       =g= 0;
XSupplyTerm(jm)..      vol(jm) - sum(is, qps(is,jm))  =g= 0;
XSupplyNG(jm)..        sum(is, qps(is,jm)) + sum(ip, qpp(ip,jm)) - ( dbase(jm) * ((pbase(jm)/p(jm))**elas(jm)) ) =g= 0;


Model pewem / ProdSProfit.qps , ProdSRent.rps ,
              ShipProfit.ship, ShipRent.rs ,
              TermProfit.vol , TermRent.rt ,
              ProdPProfit.qpp,
              ProdPRent.rpp ,
              PipeRent.rpl ,
              XSupplyNG.p , XSupplyTerm.tau , XSupplyShip.srate 
              /;

Option MCP = MILES;

Solve pewem using MCP;

* --------------------------
* Estract useful data from solution
* --------------------------

Parameter
psNetProfit(is)  'total profit of shipping producers'
ppNetProfit(ip)  'total profit of pipeline producers';

psNetProfit(is)  = sum(jm, qps.L(is,jm)*rps.L(is));
ppNetProfit(ip) = sum(jm, qpp.L(ip,jm)*(rpp.L(ip) + rpl.L(ip, jm)));


Display p.L,
        qps.L,  rps.L,
        qpp.L, rpl.L, 
        ship.L , srate.L , rs.L , 
        tau.L, rt.L,
        psNetProfit, ppNetProfit ;
        
* ====================================================

