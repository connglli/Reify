(fun func_0g6z1t_114 i32 ((par v0 i32) (par v1 i32) (par v2 i32) (par v3 i32) (par v4 i32) (par v5 i32))
  (loc v6 #-1 i32)
  (loc v7 #-1 i32)
  (bbl BB0 
    (asn v1 (eadd (div #-1 v6) (mul #-1 v5) (cst #-1)))
    (asn v4 (eadd (rem #-1 v0) (div #-1 v6) (cst #-1)))
    (goto BB7)
  )
  (bbl BB1 
    (asn v2 (eadd (mul #1046152153 v2) (sub #-770019189 v4) (cst #-1204220604)))
    (asn v7 (esub (add #-439377471 v2) (rem #-1747273890 v0) (cst #-260432471)))
    (brh BB3 BB4 (ltz (esub (mul #1517848135 v2) (add #-443430608 v7) (rem #-49314837 v1) (cst #1169009735))))
  )
  (bbl BB2 
    (asn v2 (eadd (add #1651605909 v4) (sub #-794462097 v5) (cst #82350861)))
    (asn v3 (esub (sub #1792952631 v2) (sub #606117129 v6) (cst #815669779)))
    (brh BB3 BB13 (eqz (esub (div #88914345 v2) (rem #1421975236 v3) (add #-711792060 v1) (cst #-1700309355))))
  )
  (bbl BB3 
    (asn v6 (esub (add #-1554127980 v1) (sub #-1856302098 v7) (cst #-1508040067)))
    (asn v0 (esub (sub #1295777445 v2) (div #-20478190 v3) (cst #211644074)))
    (brh BB4 BB8 (gtz (eadd (mul #1193041166 v6) (mul #1004162963 v0) (mul #875543758 v7) (cst #1918457413))))
  )
  (bbl BB4 
    (asn v0 (esub (add #-1 v5) (sub #-1 v4) (cst #-1)))
    (asn v7 (eadd (add #-1 v5) (div #-1 v6) (cst #-1)))
    (brh BB9 BB10 (ltz (esub (div #-1 v0) (add #-1 v7) (add #-1 v2) (cst #-1))))
  )
  (bbl BB5 
    (asn v4 (eadd (sub #-1 v0) (add #-1 v3) (cst #-1)))
    (asn v6 (esub (div #-1 v5) (div #-1 v2) (cst #3)))
    (brh BB6 BB13 (ltz (eadd (sub #-1 v4) (add #-1 v6) (mul #-1 v3) (cst #-1))))
  )
  (bbl BB6 
    (asn v4 (eadd (rem #-1 v1) (div #-1 v3) (cst #-1)))
    (asn v7 (eadd (rem #-1 v3) (sub #-1 v5) (cst #-1)))
    (brh BB12 BB13 (gtz (esub (rem #-1 v4) (rem #-1 v7) (sub #-1 v3) (cst #2147483647))))
  )
  (bbl BB7 
    (asn v4 (eadd (rem #-1 v5) (add #-1 v4) (cst #-1)))
    (asn v1 (esub (sub #-2 v7) (sub #-1 v4) (cst #-1)))
    (brh BB8 BB11 (eqz (eadd (mul #-1 v4) (add #-1 v1) (mul #-1 v2) (cst #-1))))
  )
  (bbl BB8 
    (asn v6 (esub (sub #-2 v6) (div #-1 v0) (cst #-1)))
    (asn v2 (eadd (mul #-1 v3) (add #-1 v7) (cst #-1)))
    (brh BB10 BB11 (gtz (esub (add #-1 v6) (mul #-1 v2) (mul #-1 v3) (cst #-1))))
  )
  (bbl BB9 
    (asn v2 (eadd (mul #-1 v7) (rem #-1 v0) (cst #-1)))
    (asn v7 (esub (add #-1 v1) (add #-1 v6) (cst #7)))
    (brh BB5 BB8 (eqz (eadd (mul #-1 v2) (sub #-1 v7) (sub #-1 v3) (cst #1))))
  )
  (bbl BB10 
    (asn v1 (eadd (mul #-1 v7) (rem #-1 v5) (cst #-1)))
    (asn v6 (eadd (rem #-1 v1) (add #-1 v5) (cst #-1)))
    (brh BB9 BB12 (eqz (eadd (mul #-1 v1) (add #-1 v6) (mul #-1 v5) (cst #6))))
  )
  (bbl BB11 
    (asn v6 (eadd (sub #-1 v7) (rem #-1 v4) (cst #-1)))
    (asn v2 (eadd (rem #-1 v2) (sub #-1 v0) (cst #-1)))
    (brh BB3 BB4 (gtz (eadd (mul #-1 v6) (rem #-1 v2) (rem #-1 v4) (cst #-1))))
  )
  (bbl BB12 
    (asn v3 (esub (mul #1393721948 v5) (div #-221316132 v4) (cst #-1675770556)))
    (asn v4 (esub (rem #-858853515 v0) (add #-625319482 v4) (cst #830832638)))
    (brh BB2 BB14 (gtz (eadd (add #-1147558105 v3) (mul #1665538221 v4) (sub #-110583786 v0) (cst #-957518854))))
  )
  (bbl BB13 
    (asn v7 (eadd (sub #-1 v2) (rem #-1 v5) (cst #-1)))
    (asn v6 (esub (mul #-1 v0) (div #-1 v4) (cst #2)))
    (brh BB10 BB14 (ltz (esub (add #-1 v7) (mul #-1 v6) (sub #-1 v0) (cst #-6))))
  )
  (bbl BB14 
    (asn v0 (esub (add #-1 v2) (mul #-1 v7) (cst #-2)))
    (asn v6 (esub (add #-1 v5) (add #-1 v3) (cst #-1)))
    (ret)
  )
)

