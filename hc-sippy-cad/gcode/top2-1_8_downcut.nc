%
(top2-1_8-downcut)
(T1  D=2.812 CR=0 - ZMIN=-7.5 - flat end mill)
G90 G94
G17
G21

(Contour-1_8)
M9
T1 M6
S16000 M3
G54
G0 X81.501 Y26.17
Z15
Z5
G1 Z1 F333.3
Z-7.219
G18 G2 X81.782 Z-7.5 I0.281
G1 X82.063 F1000
G17 G3 X82.344 Y26.451 J0.281
G1 Y37.296
X13.656
Y15.607
X82.344
Y26.451
G3 X82.063 Y26.733 I-0.281
G1 X81.782
G18 G3 X81.501 Z-7.219 K0.281
G0 Z5
X102.326 Y18.552
G1 Z1 F333.3
Z-7.219
G2 X102.607 Z-7.5 I0.281
G1 X102.888 F1000
G17 G3 X103.169 Y18.833 J0.281
X93.831 I-4.669
X103.169 I4.669
X102.888 Y19.114 I-0.281
G1 X102.607
G18 G3 X102.326 Z-7.219 K0.281
G0 Z5
Y37.386
G1 Z1 F333.3
Z-7.219
G2 X102.607 Z-7.5 I0.281
G1 X102.888 F1000
G17 G3 X103.169 Y37.667 J0.281
X93.831 I-4.669
X103.169 I4.669
X102.888 Y37.948 I-0.281
G1 X102.607
G18 G3 X102.326 Z-7.219 K0.281
G0 Z15
G17
M30
%
