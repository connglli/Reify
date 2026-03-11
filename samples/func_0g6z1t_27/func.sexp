(fun func_0g6z1t_27 i32 ((par v0 i32[3][1]) (par v1 i32) (par v2 i32) (par v3 i32) (par v4 i32) (par v5 i32))
  (loc v6 #-2 i32)
  (loc v7 #0 i32)
  (bbl BB0 
    (asn v7 (eadd (sub #-1 v6) (div #-1 v1) (cst #-1)))
    (asn v6 (esub (rem #-1 v4) (rem #-1 v7) (cst #1)))
    (brh BB2 BB11 (ltz (eadd (mul #-1 v7) (div #-1 v6) (add #-1 v0[#2][#0]) (cst #-1))))
  )
  (bbl BB1 
    (asn v5 (eadd (rem #1188222901 v4) (mul #-2099364576 v0[#2][#0]) (cst #-2115293317)))
    (asn v3 (eadd (rem #-2091400526 v1) (rem #-1761356206 v6) (cst #1692972703)))
    (brh BB5 BB6 (gtz (eadd (div #-1564930813 v5) (add #1129633470 v3) (add #-214471023 v0[#2][#0]) (cst #1647272685))))
  )
  (bbl BB2 
    (asn v4 (eadd (div #-1 v4) (div #-1 v1) (cst #-1)))
    (asn v0[#2][#0] (eadd (mul #-1 v1) (mul #-1 v4) (cst #-1)))
    (brh BB3 BB6 (gtz (esub (mul #-1 v4) (mul #-1 v0[#2][#0]) (rem #-1 v5) (cst #-1))))
  )
  (bbl BB3 
    (asn v4 (eadd (div #-567742294 v0[#2][#0]) (mul #-1552156737 v7) (cst #1897485736)))
    (asn v0[#2][#0] (esub (mul #875424913 v2) (sub #1245139139 v3) (cst #181500727)))
    (brh BB2 BB12 (eqz (esub (rem #368925950 v4) (add #-145652306 v0[#2][#0]) (rem #1359689855 v6) (cst #-71549568))))
  )
  (bbl BB4 
    (asn v4 (esub (sub #-1899147967 v0[#2][#0]) (mul #-1748944377 v2) (cst #740243720)))
    (asn v1 (eadd (rem #-1052315536 v1) (rem #125030059 v6) (cst #-1054168375)))
    (brh BB6 BB11 (eqz (esub (sub #-2055222740 v4) (rem #-1228884224 v1) (mul #-1024130708 v6) (cst #1705633468))))
  )
  (bbl BB5 
    (asn v0[#2][#0] (eadd (mul #0 v0[#2][#0]) (cst #1)))
    (asn v7 (eadd (mul #0 v7) (cst #1)))
    (asn v1 (eadd (mul #0 v1) (cst #1)))
    (asn v3 (esub (div #1 v0[#2][#0]) (mul #2147483647 v7) (cst #0)))
    (asn v0[#2][#0] (eadd (div #1 v1) (div #1073741824 v3) (cst #0)))
    (brh BB6 BB7 (gtz (esub (rem #1 v3) (rem #-1 v0[#2][#0]) (rem #1 v7) (cst #-2147483648))))
  )
  (bbl BB6 
    (asn v5 (eadd (mul #-1 v1) (sub #-1 v6) (cst #-1)))
    (asn v2 (esub (sub #-1 v7) (rem #-1 v6) (cst #-1)))
    (brh BB8 BB14 (ltz (eadd (mul #-1 v5) (add #-1 v2) (rem #-1 v7) (cst #2147483647))))
  )
  (bbl BB7 
    (asn v2 (esub (mul #-1116904293 v7) (rem #-1753924002 v3) (cst #1666720477)))
    (asn v1 (eadd (sub #-797733778 v6) (add #-1084660852 v4) (cst #-1355539828)))
    (brh BB12 BB13 (ltz (esub (sub #1238906917 v2) (rem #-601333354 v1) (mul #1181370267 v3) (cst #-1396298147))))
  )
  (bbl BB8 
    (asn v0[#2][#0] (esub (add #-1420443367 v2) (mul #1357307224 v3) (cst #-2008294264)))
    (asn v3 (esub (sub #216077463 v4) (rem #586130013 v1) (cst #1914265201)))
    (brh BB1 BB11 (ltz (esub (mul #135528181 v0[#2][#0]) (rem #1876581058 v3) (mul #-1110037038 v7) (cst #411585598))))
  )
  (bbl BB9 
    (asn v1 (esub (add #-1437297992 v6) (add #577277296 v2) (cst #-1321568005)))
    (asn v6 (esub (rem #-2019723522 v6) (mul #1573996594 v3) (cst #-1301778180)))
    (brh BB1 BB14 (ltz (esub (mul #545475651 v1) (mul #-1096557589 v6) (div #224415953 v2) (cst #-1532653634))))
  )
  (bbl BB10 
    (asn v2 (esub (sub #546447859 v1) (sub #1681119096 v7) (cst #2141616005)))
    (asn v0[#2][#0] (esub (div #1801415037 v1) (sub #1043567760 v4) (cst #-1533650987)))
    (brh BB9 BB14 (ltz (eadd (div #-172044147 v2) (div #246147991 v0[#2][#0]) (div #-1270865713 v6) (cst #-1458452012))))
  )
  (bbl BB11 
    (asn v1 (esub (div #262713768 v0[#2][#0]) (sub #-1654288226 v7) (cst #1473506070)))
    (asn v2 (eadd (rem #-1200048967 v0[#2][#0]) (mul #1236048429 v7) (cst #1832450442)))
    (brh BB4 BB10 (ltz (esub (rem #-1303658306 v1) (sub #-1532052759 v2) (add #-214803266 v0[#2][#0]) (cst #-875963392))))
  )
  (bbl BB12 
    (asn v3 (esub (mul #768073103 v7) (sub #180357944 v4) (cst #-1941058865)))
    (asn v0[#2][#0] (eadd (rem #-610303743 v3) (sub #-575127662 v7) (cst #528391515)))
    (brh BB5 BB14 (eqz (eadd (sub #-173046054 v3) (div #-1616430509 v0[#2][#0]) (mul #231763404 v6) (cst #-1420149449))))
  )
  (bbl BB13 
    (asn v1 (eadd (add #-1278163252 v3) (rem #-1817299148 v7) (cst #722924887)))
    (asn v6 (esub (div #-763640227 v1) (add #43122086 v6) (cst #-600790264)))
    (brh BB12 BB14 (gtz (eadd (mul #803598341 v1) (sub #1851819412 v6) (add #-1233440425 v4) (cst #31877927))))
  )
  (bbl BB14 
    (asn v0[#2][#0] (esub (mul #-1 v1) (sub #3 v5) (cst #-1)))
    (asn v3 (esub (sub #-2 v0[#2][#0]) (div #-1 v2) (cst #-1)))
    (ret)
  )
)

