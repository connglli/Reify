(struct func_0g6z1t_87_S0 ((f0 i32[3][2]) (f1 i32)))
(fun func_0g6z1t_87 i32 ((par v0 i32) (par v1 i32) (par v2 i32) (par v3 i32) (par v4 i32[2]) (par v5 i32))
  (loc v6 #-1 #-1 #-1 #-1 #-1 #-1 #-1 func_0g6z1t_87_S0)
  (loc v7 #0 i32)
  (bbl BB0 
    (asn v7 (esub (sub #-3 v4[#1]) (sub #-1 v2) (cst #-1)))
    (asn v6[f1] (esub (div #-1 v6[f0][#2][#1]) (add #-1 v3) (cst #-1)))
    (goto BB10)
  )
  (bbl BB1 
    (asn v7 (esub (rem #-1 v6[f1]) (sub #1 v4[#1]) (cst #-1)))
    (asn v0 (esub (sub #-5 v0) (add #-1 v6[f0][#2][#1]) (cst #-1)))
    (brh BB3 BB11 (ltz (esub (sub #-1 v7) (add #-1 v0) (add #-1 v5) (cst #-1))))
  )
  (bbl BB2 
    (asn v7 (esub (rem #168261761 v6[f1]) (rem #1977920875 v3) (cst #270109990)))
    (asn v0 (esub (add #-454967513 v2) (mul #752442707 v4[#1]) (cst #507529732)))
    (brh BB3 BB9 (ltz (eadd (rem #-1740564294 v7) (mul #348195021 v0) (add #-42683830 v2) (cst #-370319477))))
  )
  (bbl BB3 
    (asn v6[f1] (esub (div #-1944202631 v0) (div #-1668660139 v4[#1]) (cst #307107536)))
    (asn v7 (eadd (mul #1811455412 v5) (add #-1092855740 v1) (cst #1162460490)))
    (brh BB6 BB14 (ltz (eadd (add #-1775355343 v6[f0][#2][#1]) (add #-203939595 v7) (add #-193405555 v3) (cst #2115216436))))
  )
  (bbl BB4 
    (asn v5 (esub (rem #-1 v7) (add #-1 v3) (cst #-1)))
    (asn v0 (esub (rem #-1 v4[#1]) (rem #-1 v3) (cst #2)))
    (brh BB7 BB13 (eqz (eadd (sub #-1 v5) (sub #-1 v0) (mul #-1 v6[f1]) (cst #-2))))
  )
  (bbl BB5 
    (asn v5 (esub (add #-1 v6[f1]) (rem #-1 v4[#1]) (cst #-1)))
    (asn v0 (esub (sub #-7 v5) (mul #-1 v3) (cst #-1)))
    (brh BB1 BB14 (gtz (eadd (add #-1 v5) (mul #-1 v0) (sub #-1 v1) (cst #-1))))
  )
  (bbl BB6 
    (asn v3 (eadd (add #-222333123 v3) (add #1951484560 v0) (cst #-1217043357)))
    (asn v0 (eadd (div #-646935281 v4[#1]) (div #1862744653 v1) (cst #700439766)))
    (brh BB8 BB14 (ltz (esub (div #-1243881971 v3) (sub #486018963 v0) (sub #1224335733 v7) (cst #-1895429946))))
  )
  (bbl BB7 
    (asn v3 (esub (div #-1969193157 v7) (div #-1832583936 v3) (cst #-1945693093)))
    (asn v6[f0][#2][#1] (esub (mul #-305120899 v6[f1]) (mul #875385828 v5) (cst #-491080121)))
    (brh BB12 BB13 (eqz (eadd (rem #-1588416593 v3) (sub #1373040679 v6[f1]) (mul #-1748574038 v2) (cst #1095764214))))
  )
  (bbl BB8 
    (asn v6[f1] (esub (div #211995027 v4[#1]) (rem #1879433058 v3) (cst #-33333838)))
    (asn v0 (eadd (add #-1041074276 v7) (div #-1185619341 v3) (cst #1629364321)))
    (brh BB2 BB14 (gtz (esub (rem #1442158410 v6[f0][#2][#1]) (add #1989678213 v0) (mul #1812331313 v5) (cst #-622864905))))
  )
  (bbl BB9 
    (asn v3 (esub (mul #766660325 v6[f0][#2][#1]) (mul #549765756 v3) (cst #1894171770)))
    (asn v0 (esub (div #-1900874743 v1) (add #-1331660024 v6[f1]) (cst #-1353427265)))
    (brh BB4 BB8 (gtz (esub (rem #-1025403515 v3) (rem #-556014788 v0) (sub #-1332814649 v2) (cst #-481158761))))
  )
  (bbl BB10 
    (asn v2 (eadd (add #-1 v6[f0][#2][#1]) (div #-1 v4[#1]) (cst #0)))
    (asn v7 (esub (div #-1 v5) (mul #-1 v7) (cst #1)))
    (brh BB2 BB5 (ltz (esub (add #-1 v2) (add #-1 v7) (rem #-1 v3) (cst #-1))))
  )
  (bbl BB11 
    (asn v3 (esub (rem #-1 v1) (sub #-3 v7) (cst #-1)))
    (asn v7 (eadd (div #-1 v4[#1]) (add #-1 v1) (cst #-1)))
    (brh BB4 BB14 (ltz (esub (rem #-1 v3) (rem #-1 v7) (add #-1 v2) (cst #2147483647))))
  )
  (bbl BB12 
    (asn v1 (esub (sub #-1406991487 v1) (mul #1331301698 v4[#1]) (cst #-1452783088)))
    (asn v3 (esub (div #-2098731177 v2) (sub #1934161037 v1) (cst #830359529)))
    (brh BB13 BB14 (eqz (eadd (sub #-1462555754 v1) (add #5578660 v3) (mul #1038527565 v6[f0][#2][#1]) (cst #1598159393))))
  )
  (bbl BB13 
    (asn v5 (esub (sub #-2 v2) (div #-1 v0) (cst #-1)))
    (asn v1 (esub (add #-1 v4[#1]) (sub #-1 v2) (cst #-1)))
    (brh BB5 BB14 (eqz (eadd (rem #-1 v5) (rem #-1 v1) (sub #-1 v2) (cst #-1))))
  )
  (bbl BB14 
    (asn v4[#1] (esub (rem #-1 v4[#1]) (mul #-1 v3) (cst #3)))
    (asn v3 (esub (rem #-1 v3) (rem #-1 v7) (cst #1)))
    (ret)
  )
)

