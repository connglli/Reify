(struct func_xb1v6r_78_S0 ((f0 i32)))
(fun func_xb1v6r_78 i32 ((par v0 i32) (par v1 i32[2]) (par v2 func_xb1v6r_78_S0) (par v3 i32) (par v4 i32) (par v5 i32[2]))
  (loc v6 #-1 i32)
  (loc v7 #0 i32)
  (bbl BB0 
    (asn v7 (esub (add #-1 v4) (sub #-1 v0) (cst #-1)))
    (asn v2[f0] (esub (rem #-1 v5[#1]) (rem #-1 v4) (cst #0)))
    (goto BB7)
  )
  (bbl BB1 
    (asn v0 (esub (add #-1 v0) (add #-1 v7) (cst #-1)))
    (asn v1[#1] (esub (mul #-1 v7) (add #-1 v5[#1]) (cst #-2)))
    (brh BB3 BB5 (eqz (esub (mul #-1 v0) (sub #-1 v1[#1]) (mul #-1 v4) (cst #-1))))
  )
  (bbl BB2 
    (asn v0 (eadd (div #-1 v6) (sub #-1 v0) (cst #-1)))
    (asn v7 (esub (mul #-1 v6) (rem #-1 v4) (cst #2)))
    (brh BB7 BB14 (gtz (eadd (mul #-1 v0) (div #-1 v7) (mul #-1 v6) (cst #-2))))
  )
  (bbl BB3 
    (asn v0 (esub (sub #0 v2[f0]) (sub #0 v3) (cst #0)))
    (asn v3 (esub (div #0 v7) (div #0 v4) (cst #0)))
    (brh BB11 BB14 (gtz (esub (rem #0 v0) (div #0 v3) (div #0 v6) (cst #0))))
  )
  (bbl BB4 
    (asn v0 (eadd (add #0 v2[f0]) (add #0 v6) (cst #0)))
    (asn v3 (esub (rem #0 v5[#1]) (div #0 v7) (cst #0)))
    (brh BB9 BB13 (ltz (esub (div #0 v0) (rem #0 v3) (add #0 v7) (cst #0))))
  )
  (bbl BB5 
    (asn v3 (esub (sub #2 v5[#1]) (div #-1 v1[#1]) (cst #-1)))
    (asn v5[#1] (eadd (add #-1 v6) (mul #-1 v7) (cst #-1)))
    (brh BB2 BB13 (eqz (esub (rem #-1 v3) (sub #-3 v5[#1]) (sub #-1 v2[f0]) (cst #-1))))
  )
  (bbl BB6 
    (asn v0 (eadd (add #0 v4) (rem #0 v3) (cst #0)))
    (asn v4 (eadd (rem #0 v7) (div #0 v4) (cst #0)))
    (brh BB11 BB12 (gtz (eadd (mul #0 v0) (div #0 v4) (rem #0 v2[f0]) (cst #0))))
  )
  (bbl BB7 
    (asn v1[#1] (esub (rem #-1 v0) (add #-1 v5[#1]) (cst #-1)))
    (asn v7 (eadd (mul #-1 v1[#1]) (mul #-1 v7) (cst #-1)))
    (brh BB1 BB8 (eqz (esub (mul #-1 v1[#1]) (sub #2 v7) (div #-1 v6) (cst #-1))))
  )
  (bbl BB8 
    (asn v3 (esub (rem #0 v4) (div #0 v3) (cst #0)))
    (asn v2[f0] (eadd (div #0 v3) (mul #0 v5[#1]) (cst #0)))
    (brh BB2 BB10 (gtz (eadd (mul #0 v3) (rem #0 v2[f0]) (sub #0 v4) (cst #0))))
  )
  (bbl BB9 
    (asn v4 (eadd (sub #0 v7) (rem #0 v4) (cst #0)))
    (asn v3 (eadd (sub #0 v2[f0]) (rem #0 v5[#1]) (cst #0)))
    (brh BB1 BB12 (ltz (eadd (div #0 v4) (mul #0 v3) (add #0 v5[#1]) (cst #0))))
  )
  (bbl BB10 
    (asn v7 (eadd (div #0 v1[#1]) (add #0 v2[f0]) (cst #0)))
    (asn v4 (esub (add #0 v3) (mul #0 v7) (cst #0)))
    (brh BB2 BB4 (ltz (esub (sub #0 v7) (sub #0 v4) (sub #0 v1[#1]) (cst #0))))
  )
  (bbl BB11 
    (asn v0 (eadd (rem #0 v6) (div #0 v2[f0]) (cst #0)))
    (asn v6 (eadd (add #0 v1[#1]) (sub #0 v2[f0]) (cst #0)))
    (goto BB4)
  )
  (bbl BB12 
    (asn v6 (esub (rem #0 v2[f0]) (div #0 v3) (cst #0)))
    (asn v2[f0] (esub (sub #0 v5[#1]) (div #0 v1[#1]) (cst #0)))
    (goto BB7)
  )
  (bbl BB13 
    (asn v3 (eadd (sub #0 v1[#1]) (mul #0 v0) (cst #0)))
    (asn v6 (eadd (mul #0 v0) (sub #0 v5[#1]) (cst #0)))
    (goto BB4)
  )
  (bbl BB14 
    (asn v1[#1] (eadd (sub #-1 v2[f0]) (sub #-1 v6) (cst #-1)))
    (asn v3 (eadd (sub #-1 v3) (div #-1 v1[#1]) (cst #-1)))
    (ret)
  )
)

