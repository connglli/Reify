(fun func_xb1v6r_33 i32 ((par v0 i32[3]) (par v1 i32) (par v2 i32) (par v3 i32) (par v4 i32) (par v5 i32))
  (loc v6 #0 i32)
  (loc v7 #-1 i32)
  (bbl BB0 
    (asn v5 (esub (add #-1 v7) (div #-1 v2) (cst #-1)))
    (asn v2 (esub (sub #-2 v0[#2]) (mul #-1 v4) (cst #-1)))
    (brh BB16 BB17 (eqz (esub (sub #-1 v5) (rem #-1 v2) (rem #-1 v3) (cst #-1))))
  )
  (bbl BB1 
    (asn v5 (eadd (div #0 v1) (add #0 v2) (cst #0)))
    (asn v0[#2] (esub (rem #0 v6) (add #0 v3) (cst #0)))
    (goto BB3)
  )
  (bbl BB2 
    (asn v1 (eadd (sub #0 v1) (rem #0 v5) (cst #0)))
    (asn v3 (eadd (mul #0 v7) (sub #0 v1) (cst #0)))
    (goto BB4)
  )
  (bbl BB3 
    (asn v4 (esub (mul #0 v5) (mul #0 v7) (cst #0)))
    (asn v2 (esub (add #0 v5) (rem #0 v3) (cst #0)))
    (goto BB2)
  )
  (bbl BB4 
    (asn v5 (esub (mul #0 v5) (div #0 v7) (cst #0)))
    (asn v4 (eadd (add #0 v5) (div #0 v6) (cst #0)))
    (brh BB1 BB5 (gtz (esub (rem #0 v5) (sub #0 v4) (rem #0 v6) (cst #0))))
  )
  (bbl BB5 
    (asn v2 (eadd (mul #0 v1) (rem #0 v7) (cst #0)))
    (asn v4 (eadd (add #0 v2) (rem #0 v6) (cst #0)))
    (brh BB6 BB20 (eqz (eadd (div #0 v2) (sub #0 v4) (mul #0 v7) (cst #0))))
  )
  (bbl BB6 
    (asn v3 (eadd (rem #0 v4) (div #0 v3) (cst #0)))
    (asn v7 (esub (div #0 v6) (add #0 v1) (cst #0)))
    (brh BB8 BB9 (ltz (esub (div #0 v3) (rem #0 v7) (mul #0 v0[#2]) (cst #0))))
  )
  (bbl BB7 
    (asn v2 (esub (mul #0 v5) (mul #0 v1) (cst #0)))
    (asn v4 (esub (sub #0 v6) (rem #0 v4) (cst #0)))
    (goto BB9)
  )
  (bbl BB8 
    (asn v4 (eadd (sub #0 v7) (rem #0 v3) (cst #0)))
    (asn v5 (eadd (div #0 v6) (mul #0 v4) (cst #0)))
    (goto BB7)
  )
  (bbl BB9 
    (asn v4 (eadd (rem #0 v5) (div #0 v3) (cst #0)))
    (asn v1 (esub (add #0 v1) (rem #0 v6) (cst #0)))
    (brh BB6 BB10 (eqz (eadd (add #0 v4) (mul #0 v1) (sub #0 v7) (cst #0))))
  )
  (bbl BB10 
    (asn v3 (esub (add #0 v5) (add #0 v3) (cst #0)))
    (asn v1 (esub (div #0 v3) (sub #0 v0[#2]) (cst #0)))
    (brh BB1 BB19 (ltz (esub (div #0 v3) (rem #0 v1) (mul #0 v4) (cst #0))))
  )
  (bbl BB11 
    (asn v1 (esub (add #0 v6) (add #0 v4) (cst #0)))
    (asn v6 (esub (rem #0 v5) (rem #0 v6) (cst #0)))
    (brh BB13 BB17 (ltz (eadd (sub #0 v1) (mul #0 v6) (add #0 v7) (cst #0))))
  )
  (bbl BB12 
    (asn v2 (esub (div #0 v3) (div #0 v5) (cst #0)))
    (asn v7 (esub (add #0 v4) (add #0 v3) (cst #0)))
    (brh BB18 BB21 (eqz (eadd (rem #0 v2) (div #0 v7) (mul #0 v3) (cst #0))))
  )
  (bbl BB13 
    (asn v4 (esub (div #0 v7) (add #0 v0[#2]) (cst #0)))
    (asn v1 (esub (rem #0 v6) (div #0 v1) (cst #0)))
    (brh BB1 BB16 (ltz (eadd (mul #0 v4) (rem #0 v1) (rem #0 v0[#2]) (cst #0))))
  )
  (bbl BB14 
    (asn v0[#2] (eadd (mul #0 v5) (div #0 v0[#2]) (cst #0)))
    (asn v2 (esub (mul #0 v5) (div #0 v0[#2]) (cst #0)))
    (brh BB1 BB19 (ltz (eadd (sub #0 v0[#2]) (div #0 v2) (add #0 v6) (cst #0))))
  )
  (bbl BB15 
    (asn v3 (esub (div #0 v0[#2]) (div #0 v7) (cst #0)))
    (asn v0[#2] (eadd (div #0 v2) (rem #0 v0[#2]) (cst #0)))
    (brh BB18 BB21 (gtz (eadd (rem #0 v3) (mul #0 v0[#2]) (div #0 v7) (cst #0))))
  )
  (bbl BB16 
    (asn v5 (eadd (mul #0 v0[#2]) (rem #0 v7) (cst #0)))
    (asn v2 (eadd (add #0 v2) (div #0 v5) (cst #0)))
    (brh BB20 BB22 (gtz (esub (add #0 v5) (div #0 v2) (div #0 v1) (cst #0))))
  )
  (bbl BB17 
    (asn v5 (eadd (add #-1 v5) (sub #-1 v0[#2]) (cst #-1)))
    (asn v6 (eadd (mul #-1 v2) (sub #-1 v3) (cst #-2)))
    (brh BB12 BB22 (gtz (eadd (rem #-1 v5) (div #-1 v6) (add #-1 v2) (cst #-1))))
  )
  (bbl BB18 
    (asn v3 (eadd (sub #0 v3) (add #0 v5) (cst #0)))
    (asn v5 (esub (rem #0 v7) (add #0 v0[#2]) (cst #0)))
    (brh BB11 BB22 (eqz (eadd (mul #0 v3) (rem #0 v5) (mul #0 v7) (cst #0))))
  )
  (bbl BB19 
    (asn v3 (esub (sub #0 v2) (add #0 v3) (cst #0)))
    (asn v6 (eadd (sub #0 v5) (rem #0 v7) (cst #0)))
    (brh BB14 BB15 (gtz (eadd (sub #0 v3) (sub #0 v6) (div #0 v4) (cst #0))))
  )
  (bbl BB20 
    (asn v1 (esub (add #0 v0[#2]) (add #0 v1) (cst #0)))
    (asn v0[#2] (eadd (mul #0 v1) (mul #0 v6) (cst #0)))
    (brh BB1 BB21 (ltz (eadd (mul #0 v1) (mul #0 v0[#2]) (div #0 v4) (cst #0))))
  )
  (bbl BB21 
    (asn v7 (esub (sub #0 v1) (rem #0 v0[#2]) (cst #0)))
    (asn v4 (eadd (add #0 v0[#2]) (add #0 v2) (cst #0)))
    (brh BB17 BB22 (eqz (esub (div #0 v7) (add #0 v4) (sub #0 v1) (cst #0))))
  )
  (bbl BB22 
    (asn v2 (eadd (div #-1 v7) (add #-1 v3) (cst #-1)))
    (asn v5 (eadd (add #-1 v6) (add #-1 v5) (cst #-1)))
    (ret)
  )
)

