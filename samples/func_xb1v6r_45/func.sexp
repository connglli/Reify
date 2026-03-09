(fun func_xb1v6r_45 i32 ((par v0 i32) (par v1 i32[1][1]) (par v2 i32[1][2]) (par v3 i32) (par v4 i32) (par v5 i32[1][2][2]))
  (loc v6 #0 i32)
  (loc v7 #-1 i32)
  (bbl BB0 
    (asn v4 (esub (mul #-1 v7) (sub #2 v5[#0][#1][#1]) (cst #-1)))
    (asn v1[#0][#0] (eadd (mul #-1 v3) (mul #-1 v2[#0][#1]) (cst #-1)))
    (goto BB2)
  )
  (bbl BB1 
    (asn v3 (eadd (div #0 v3) (mul #0 v5[#0][#1][#1]) (cst #0)))
    (asn v0 (eadd (sub #0 v5[#0][#1][#1]) (rem #0 v6) (cst #0)))
    (brh BB13 BB14 (ltz (esub (rem #0 v3) (sub #0 v0) (rem #0 v4) (cst #0))))
  )
  (bbl BB2 
    (asn v6 (eadd (rem #-1 v0) (sub #-1 v4) (cst #-1)))
    (asn v2[#0][#1] (eadd (add #-1 v5[#0][#1][#1]) (div #-1 v0) (cst #-1)))
    (brh BB3 BB12 (gtz (esub (div #-1 v6) (div #-1 v2[#0][#1]) (sub #-1 v4) (cst #-1))))
  )
  (bbl BB3 
    (asn v7 (eadd (sub #-1 v4) (rem #-1 v0) (cst #-1)))
    (asn v6 (eadd (mul #-1 v6) (sub #-1 v7) (cst #-2)))
    (brh BB8 BB9 (gtz (esub (add #-1 v7) (div #-1 v6) (div #-1 v0) (cst #-1))))
  )
  (bbl BB4 
    (asn v4 (esub (div #-1 v5[#0][#1][#1]) (sub #3 v1[#0][#0]) (cst #-1)))
    (asn v7 (eadd (rem #-1 v4) (sub #-1 v2[#0][#1]) (cst #-1)))
    (brh BB5 BB6 (gtz (eadd (div #-1 v4) (div #-1 v7) (sub #-1 v5[#0][#1][#1]) (cst #-1))))
  )
  (bbl BB5 
    (asn v2[#0][#1] (eadd (add #-1 v7) (mul #-1 v2[#0][#1]) (cst #-1)))
    (asn v0 (eadd (div #-1 v1[#0][#0]) (mul #-1 v3) (cst #-1)))
    (brh BB4 BB14 (ltz (esub (sub #-1 v2[#0][#1]) (mul #-1 v0) (add #-1 v5[#0][#1][#1]) (cst #-1))))
  )
  (bbl BB6 
    (asn v0 (esub (add #0 v3) (sub #0 v5[#0][#1][#1]) (cst #0)))
    (asn v2[#0][#1] (esub (mul #0 v1[#0][#0]) (mul #0 v2[#0][#1]) (cst #0)))
    (brh BB5 BB7 (ltz (esub (div #0 v0) (div #0 v2[#0][#1]) (mul #0 v3) (cst #0))))
  )
  (bbl BB7 
    (asn v6 (esub (add #0 v5[#0][#1][#1]) (div #0 v6) (cst #0)))
    (asn v0 (eadd (rem #0 v0) (add #0 v2[#0][#1]) (cst #0)))
    (brh BB10 BB13 (gtz (esub (mul #0 v6) (rem #0 v0) (div #0 v5[#0][#1][#1]) (cst #0))))
  )
  (bbl BB8 
    (asn v5[#0][#1][#1] (eadd (rem #-1 v3) (rem #-1 v2[#0][#1]) (cst #-1)))
    (asn v2[#0][#1] (eadd (sub #-1 v7) (sub #-1 v0) (cst #-1)))
    (goto BB4)
  )
  (bbl BB9 
    (asn v3 (esub (mul #-1 v3) (div #-1 v7) (cst #1)))
    (asn v6 (eadd (add #-1 v3) (sub #-1 v7) (cst #-1)))
    (brh BB4 BB11 (eqz (esub (div #-1 v3) (sub #-1 v6) (div #-1 v5[#0][#1][#1]) (cst #-1))))
  )
  (bbl BB10 
    (asn v2[#0][#1] (eadd (sub #0 v3) (add #0 v5[#0][#1][#1]) (cst #0)))
    (asn v5[#0][#1][#1] (esub (add #0 v5[#0][#1][#1]) (sub #0 v7) (cst #0)))
    (goto BB4)
  )
  (bbl BB11 
    (asn v7 (eadd (add #-1 v1[#0][#0]) (mul #-1 v2[#0][#1]) (cst #-1)))
    (asn v0 (esub (rem #-1 v1[#0][#0]) (rem #-1 v6) (cst #2)))
    (goto BB12)
  )
  (bbl BB12 
    (asn v7 (esub (div #-1 v0) (add #-1 v6) (cst #6)))
    (asn v5[#0][#1][#1] (esub (mul #-1 v7) (rem #-1 v0) (cst #2)))
    (goto BB8)
  )
  (bbl BB13 
    (asn v5[#0][#1][#1] (esub (add #0 v1[#0][#0]) (rem #0 v6) (cst #0)))
    (asn v0 (eadd (sub #0 v5[#0][#1][#1]) (add #0 v7) (cst #0)))
    (goto BB10)
  )
  (bbl BB14 
    (asn v2[#0][#1] (esub (add #-1 v1[#0][#0]) (div #-1 v0) (cst #0)))
    (asn v6 (esub (div #-1 v3) (rem #-1 v1[#0][#0]) (cst #2)))
    (ret)
  )
)

