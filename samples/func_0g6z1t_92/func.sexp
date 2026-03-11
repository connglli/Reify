(fun func_0g6z1t_92 i32 ((par v0 i32) (par v1 i32[1]) (par v2 i32) (par v3 i32) (par v4 i32) (par v5 i32))
  (loc v6 #-1 i32)
  (loc v7 #-1 #-1 #-1 #-1 #-1 #-1 #-1 #-1 #-1 #-1 #-1 #-1 i32[2][3][2])
  (bbl BB0 
    (asn v7[#1][#2][#1] (eadd (rem #-1 v1[#0]) (mul #-1 v6) (cst #-1)))
    (asn v2 (eadd (div #-1 v0) (add #-1 v7[#1][#2][#1]) (cst #-1)))
    (goto BB14)
  )
  (bbl BB1 
    (asn v4 (eadd (add #-1088642789 v1[#0]) (div #384330398 v6) (cst #1056155313)))
    (asn v2 (esub (div #2064745724 v2) (add #-1729800741 v3) (cst #1697127505)))
    (goto BB11)
  )
  (bbl BB2 
    (asn v1[#0] (eadd (mul #-1354251087 v7[#1][#2][#1]) (sub #1134729229 v0) (cst #207732635)))
    (asn v4 (esub (div #-2006276197 v0) (mul #-1372642142 v3) (cst #950906677)))
    (goto BB1)
  )
  (bbl BB3 
    (asn v0 (esub (add #-1327785096 v3) (add #-545036081 v4) (cst #-502251552)))
    (asn v1[#0] (esub (div #1313445365 v3) (mul #-503833094 v7[#1][#2][#1]) (cst #-847282395)))
    (brh BB12 BB14 (ltz (eadd (mul #1555820619 v0) (add #233576841 v1[#0]) (mul #-1955072082 v3) (cst #1986212559))))
  )
  (bbl BB4 
    (asn v0 (esub (add #432548518 v2) (add #1892946331 v1[#0]) (cst #-894536815)))
    (asn v7[#1][#2][#1] (esub (sub #-1945700147 v0) (div #-1411751450 v5) (cst #1775332389)))
    (brh BB2 BB6 (gtz (eadd (add #1200096460 v0) (rem #-758295444 v7[#1][#2][#1]) (sub #696990470 v4) (cst #95416961))))
  )
  (bbl BB5 
    (asn v2 (esub (rem #-539787683 v7[#1][#2][#1]) (rem #-1426750476 v3) (cst #572147562)))
    (asn v1[#0] (eadd (mul #-647928353 v5) (div #-1056773228 v0) (cst #-1904134873)))
    (goto BB6)
  )
  (bbl BB6 
    (asn v2 (eadd (mul #-2131015241 v7[#1][#2][#1]) (mul #638591107 v3) (cst #-1728853773)))
    (asn v5 (eadd (mul #230863080 v3) (sub #544474131 v7[#1][#2][#1]) (cst #1507216578)))
    (goto BB2)
  )
  (bbl BB7 
    (asn v1[#0] (eadd (mul #0 v1[#0]) (cst #1)))
    (asn v0 (eadd (mul #0 v0) (cst #-1)))
    (asn v2 (eadd (mul #0 v2) (cst #1)))
    (asn v0 (eadd (mul #0 v0) (cst #-1)))
    (asn v4 (esub (div #1 v1[#0]) (div #1 v0) (cst #-1073741826)))
    (asn v1[#0] (esub (rem #1073741825 v2) (sub #-2147483648 v0) (cst #-2147483648)))
    (brh BB3 BB10 (ltz (esub (sub #2147483646 v4) (mul #1 v1[#0]) (mul #0 v6) (cst #0))))
  )
  (bbl BB8 
    (asn v1[#0] (esub (mul #-996091823 v6) (div #-1490271115 v2) (cst #-835851389)))
    (asn v4 (eadd (mul #343032034 v4) (rem #-285688200 v5) (cst #299000396)))
    (goto BB10)
  )
  (bbl BB9 
    (asn v7[#1][#2][#1] (eadd (rem #697705875 v0) (mul #1957842939 v7[#1][#2][#1]) (cst #1064237425)))
    (asn v2 (eadd (sub #533153263 v7[#1][#2][#1]) (sub #808543272 v1[#0]) (cst #-2095546271)))
    (brh BB5 BB14 (eqz (eadd (div #1593424729 v7[#1][#2][#1]) (add #143090246 v2) (div #1799224305 v4) (cst #1692800501))))
  )
  (bbl BB10 
    (asn v6 (eadd (rem #-938460525 v1[#0]) (rem #-1084203576 v5) (cst #-237765414)))
    (asn v3 (eadd (div #-575969648 v4) (add #1837767007 v7[#1][#2][#1]) (cst #-1587925618)))
    (goto BB13)
  )
  (bbl BB11 
    (asn v4 (eadd (div #2102851548 v5) (add #-1101285555 v3) (cst #1184245996)))
    (asn v2 (eadd (add #1048648581 v0) (sub #846230897 v3) (cst #-579773893)))
    (goto BB9)
  )
  (bbl BB12 
    (asn v5 (esub (add #61605786 v1[#0]) (rem #2030128696 v2) (cst #1054100634)))
    (asn v7[#1][#2][#1] (eadd (div #434193331 v0) (rem #1924752953 v1[#0]) (cst #199559563)))
    (goto BB11)
  )
  (bbl BB13 
    (asn v6 (eadd (add #999494315 v3) (add #-516366117 v1[#0]) (cst #-2083861033)))
    (asn v2 (esub (sub #-637843264 v3) (div #-611613113 v7[#1][#2][#1]) (cst #1043311356)))
    (goto BB5)
  )
  (bbl BB14 
    (asn v0 (eadd (rem #-1 v3) (rem #-1 v5) (cst #-1)))
    (asn v5 (eadd (sub #-1 v3) (div #-1 v6) (cst #-1)))
    (ret)
  )
)

