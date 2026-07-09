* Copyright Ben Paul Wise. All Rights Reserved.
* ============================================================
* Example pform input: a PFORM (parliament-formation) instance
* in the limited GMS subset the pform app accepts. It must
* define exactly these symbols: act, iss, weight, position,
* salience, unselectedProb. Comments (lines beginning with *)
* are allowed.
*   act = parties, iss = issues. position and salience are
*   indexed (iss, act). Each party's salience column must sum
*   to at least 1. K = |act|^|iss| = 3^4 = 81 parliaments.
* ============================================================

Set act parties / P0, P1, P2 / ;
Set iss issues  / I0, I1, I2, I3 / ;

Parameter weight(act) 'relative weight of each party'
/
P0   5.0
P1   3.0
P2   7.0
/ ;

Table position(iss, act) 'preferred position of each party on each issue (0..1)'
        P0      P1      P2
I0     0.20    0.80    0.50
I1     0.60    0.10    0.90
I2     0.40    0.50    0.30
I3     0.90    0.20    0.60
;

Table salience(iss, act) 'salience of each issue to each party (column sum >= 1)'
        P0      P1      P2
I0     0.40    0.10    0.30
I1     0.30    0.50    0.20
I2     0.20    0.20    0.40
I3     0.30    0.40    0.30
;

Parameters
    unselectedProb  'unselected probability q, in (0, (K-1)/K)'
    ;

unselectedProb = 0.05 ;

* Copyright Ben Paul Wise. All Rights Reserved.
