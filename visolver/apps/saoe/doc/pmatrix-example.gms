* Copyright Ben Paul Wise. All Rights Reserved.
* ============================================================
* Example pmatrix input: an SAOE (Strategic Allocation Of Effort)
* instance in the limited GMS subset the pmatrix app accepts.
* It must define exactly these symbols: act, opt, weight, reward,
* raFrac. Comments (lines beginning with *) are allowed.
* ============================================================

Set act actors   / A0, A1, A2, A3, A4, A5 / ;
Set opt options  / P0, P1, P2, P3, P4, P5, P6, P7, P8, P9 / ;


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
    raFrac  'risk aversion fraction'
    ;

raFrac  = 0.2;

* Copyright Ben Paul Wise. All Rights Reserved.
