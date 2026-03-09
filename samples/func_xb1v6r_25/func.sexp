(struct func_xb1v6r_25_S0 ((f0 i32) (f1 i32[3]) (f2 i32)))
(fun func_xb1v6r_25 i32 ((par v0 i32[3][1]) (par v1 i32) (par v2 i32) (par v3 func_xb1v6r_25_S0) (par v4 i32[3][3]) (par v5 i32[2][3]))
  (loc v6 #-1 i32)
  (loc v7 #0 i32)
  (bbl BB0 
    (asn v6 (esub (add #-1 v3[f2]) (div #-1 v6) (cst #-1)))
    (asn v5[#1][#2] (eadd (div #-1 v6) (rem #-1 v3[f2]) (cst #-1)))
    (goto BB7)
  )
  (bbl BB1 
    (asn v6 (esub (mul #0 v5[#1][#2]) (add #0 v0[#2][#0]) (cst #0)))
    (asn v0[#2][#0] (eadd (rem #0 v3[f2]) (rem #0 v2) (cst #0)))
    (brh BB14 BB18 (eqz (eadd (sub #0 v6) (rem #0 v0[#2][#0]) (rem #0 v3[f0]) (cst #0))))
  )
  (bbl BB2 
    (asn v3[f1][#2] (esub (sub #0 v6) (mul #0 v3[f2]) (cst #0)))
    (asn v6 (eadd (mul #0 v4[#2][#2]) (div #0 v5[#1][#2]) (cst #0)))
    (brh BB3 BB6 (ltz (esub (div #0 v3[f2]) (sub #0 v6) (div #0 v5[#1][#2]) (cst #0))))
  )
  (bbl BB3 
    (asn v1 (eadd (sub #0 v7) (div #0 v2) (cst #0)))
    (asn v7 (esub (div #0 v7) (rem #0 v4[#2][#2]) (cst #0)))
    (goto BB15)
  )
  (bbl BB4 
    (asn v1 (eadd (add #0 v3[f2]) (div #0 v5[#1][#2]) (cst #0)))
    (asn v2 (eadd (rem #0 v7) (mul #0 v6) (cst #0)))
    (goto BB5)
  )
  (bbl BB5 
    (asn v1 (esub (div #0 v6) (rem #0 v3[f1][#2]) (cst #0)))
    (asn v6 (eadd (sub #0 v3[f0]) (add #0 v4[#2][#2]) (cst #0)))
    (goto BB13)
  )
  (bbl BB6 
    (asn v2 (esub (rem #0 v4[#2][#2]) (mul #0 v7) (cst #0)))
    (asn v0[#2][#0] (eadd (rem #0 v1) (rem #0 v2) (cst #0)))
    (brh BB1 BB4 (eqz (eadd (rem #0 v2) (sub #0 v0[#2][#0]) (mul #0 v6) (cst #0))))
  )
  (bbl BB7 
    (asn v7 (esub (rem #-1 v3[f2]) (mul #-1 v6) (cst #-1)))
    (asn v4[#2][#2] (eadd (sub #-1 v4[#2][#2]) (div #-1 v0[#2][#0]) (cst #-2)))
    (brh BB9 BB10 (eqz (eadd (rem #-1 v7) (rem #-1 v4[#2][#2]) (rem #-1 v6) (cst #-1))))
  )
  (bbl BB8 
    (asn v5[#1][#2] (esub (div #0 v5[#1][#2]) (add #0 v7) (cst #0)))
    (asn v4[#2][#2] (esub (mul #0 v0[#2][#0]) (mul #0 v2) (cst #0)))
    (goto BB10)
  )
  (bbl BB9 
    (asn v6 (eadd (mul #0 v2) (mul #0 v1) (cst #0)))
    (asn v3[f1][#2] (esub (mul #0 v4[#2][#2]) (add #0 v0[#2][#0]) (cst #0)))
    (goto BB8)
  )
  (bbl BB10 
    (asn v2 (esub (sub #-3 v5[#1][#2]) (sub #-1 v1) (cst #-1)))
    (asn v7 (esub (mul #-1 v0[#2][#0]) (rem #-1 v1) (cst #2)))
    (brh BB7 BB11 (ltz (esub (sub #-1 v2) (div #-1 v7) (add #-1 v4[#2][#2]) (cst #-1))))
  )
  (bbl BB11 
    (asn v3[f2] (esub (div #-1 v5[#1][#2]) (sub #1 v6) (cst #-1)))
    (asn v6 (esub (div #-1 v3[f0]) (mul #-1 v7) (cst #1)))
    (brh BB15 BB17 (ltz (esub (rem #-1 v3[f0]) (add #-1 v6) (sub #-1 v0[#2][#0]) (cst #-1))))
  )
  (bbl BB12 
    (asn v5[#1][#2] (eadd (rem #0 v7) (mul #0 v3[f0]) (cst #0)))
    (asn v6 (esub (sub #0 v7) (rem #0 v4[#2][#2]) (cst #0)))
    (goto BB17)
  )
  (bbl BB13 
    (asn v7 (esub (rem #-1 v7) (mul #-1 v4[#2][#2]) (cst #0)))
    (asn v2 (eadd (rem #-1 v2) (add #-1 v7) (cst #-1)))
    (brh BB12 BB18 (ltz (esub (sub #-1 v7) (div #-1 v2) (div #-1 v5[#1][#2]) (cst #-1))))
  )
  (bbl BB14 
    (asn v6 (esub (rem #0 v1) (div #0 v5[#1][#2]) (cst #0)))
    (asn v0[#2][#0] (eadd (add #0 v6) (rem #0 v2) (cst #0)))
    (goto BB16)
  )
  (bbl BB15 
    (asn v7 (eadd (mul #0 v7) (cst #2)))
    (asn v6 (eadd (mul #0 v6) (cst #-2147483648)))
    (asn v1 (eadd (mul #0 v1) (cst #1)))
    (asn v6 (eadd (div #0 v7) (sub #0 v6) (cst #0)))
    (asn v7 (eadd (rem #0 v1) (rem #0 v6) (cst #0)))
    (brh BB2 BB18 (gtz (eadd (sub #0 v6) (add #0 v7) (rem #0 v0[#2][#0]) (cst #0))))
  )
  (bbl BB16 
    (asn v1 (esub (mul #0 v3[f1][#2]) (rem #0 v1) (cst #0)))
    (asn v5[#1][#2] (esub (add #0 v0[#2][#0]) (sub #0 v1) (cst #0)))
    (goto BB13)
  )
  (bbl BB17 
    (asn v1 (eadd (add #-1 v4[#2][#2]) (div #-1 v1) (cst #-1)))
    (asn v3[f1][#2] (esub (sub #-5 v5[#1][#2]) (add #-1 v4[#2][#2]) (cst #-1)))
    (goto BB13)
  )
  (bbl BB18 
    (asn v1 (esub (mul #-1 v2) (add #-1 v1) (cst #7)))
    (asn v5[#1][#2] (esub (div #-1 v6) (sub #2 v3[f1][#2]) (cst #-1)))
    (ret)
  )
)

