#include <stdio.h>

int var_0 = 411; // Initial value from Z3 model
int var_1 = 104; // Initial value from Z3 model
int var_2 = -985; // Initial value from Z3 model
int var_3 = 15; // Initial value from Z3 model
int var_4 = 512; // Initial value from Z3 model
int var_5 = -613; // Initial value from Z3 model
int var_6 = -305; // Initial value from Z3 model
int var_7 = 774; // Initial value from Z3 model
int var_8 = 925; // Initial value from Z3 model
int var_9 = -855; // Initial value from Z3 model
int var_10 = -86; // Initial value from Z3 model
int var_11 = -521; // Initial value from Z3 model
int var_12 = 440; // Initial value from Z3 model
int var_13 = 850; // Initial value from Z3 model
int var_14 = 190; // Initial value from Z3 model

int pass_counter = 0;
int main() {
BB0:
    var_13 = -1 * var_11 + -1 * var_7 + -1;
    goto BB28;

BB1:
    var_8 = 1787034797 * var_6 + -2061688837 * var_8 + 902734500;
    goto BB27;

BB2:
    var_8 = -2006147462 * var_13 + -28102373 * var_2 + 1827482044;
    if (2115402983 * var_8 + -2063908990 * var_10 + 1344553397 * var_7 + 546422525 * var_1 + 2069524973 >= 0) goto BB3;
    goto BB4;

BB3:
    var_1 = -1 * var_1 + -1 * var_2 + -1;
    if (-1 * var_1 + -1 * var_12 + -1 * var_9 + -1 * var_4 + 976 >= 0) goto BB2;
    goto BB29;

BB4:
    var_7 = 1286672080 * var_12 + -1833045501 * var_10 + 524495529;
    if (1484594768 * var_7 + 1298010217 * var_8 + -621187695 * var_9 + -495561937 * var_5 + 1114684965 >= 0) goto BB20;
    goto BB7;

BB5:
    var_3 = -1118972290 * var_8 + 1009243961 * var_7 + 937038665;
    if (-261274652 * var_3 + 1430841431 * var_0 + 1741301327 * var_10 + -265049139 * var_4 + 1729237363 >= 0) goto BB20;
    goto BB17;

BB6:
    var_6 = -868949071 * var_8 + -1127028089 * var_7 + 248022204;
    if (-79598476 * var_6 + 704111016 * var_12 + -2145646263 * var_2 + 1002516721 * var_11 + -1670149360 >= 0) goto BB14;
    goto BB13;

BB7:
    var_6 = -413580163 * var_2 + -1895735027 * var_9 + 2067195220;
    if (-1791619152 * var_6 + 1718997141 * var_3 + 1660539433 * var_12 + 718782355 * var_0 + -21585479 >= 0) goto BB19;
    goto BB8;

BB8:
    var_6 = -1847319340 * var_14 + 1049448344 * var_10 + 285695752;
    if (-1281657505 * var_6 + 16800571 * var_12 + 1629337733 * var_5 + -1554508007 * var_10 + 612676147 >= 0) goto BB10;
    goto BB13;

BB9:
    var_6 = -1 * var_2 + -1 * var_3 + -1;
    if (-1 * var_6 + -1 * var_13 + -1 * var_5 + -1 * var_2 + 0 >= 0) goto BB3;
    goto BB15;

BB10:
    var_14 = 208098016 * var_11 + -1694228134 * var_0 + -1816107560;
    if (252957136 * var_14 + -2079493663 * var_4 + -1551900686 * var_8 + -1826298578 * var_6 + -1669113962 >= 0) goto BB17;
    goto BB11;

BB11:
    var_3 = 1653402445 * var_1 + 493460774 * var_7 + 816948592;
    if (-1315080634 * var_3 + -2080804398 * var_2 + -233214720 * var_6 + -1175527620 * var_8 + -1425554618 >= 0) goto BB20;
    goto BB7;

BB12:
    var_12 = 585267359 * var_3 + -182015248 * var_9 + 1144924291;
    if (388211357 * var_12 + 1079332846 * var_1 + 35861097 * var_7 + 1572492290 * var_9 + 1215469528 >= 0) goto BB9;
    goto BB13;

BB13:
    var_8 = -299896754 * var_12 + -855263845 * var_8 + 208610690;
    if (-1696643602 * var_8 + 1234078571 * var_1 + 98789751 * var_9 + -1959818812 * var_3 + 1684586930 >= 0) goto BB6;
    goto BB4;

BB14:
    var_11 = 1801530938 * var_2 + 518297878 * var_1 + 613441367;
    if (-1297622421 * var_11 + 4747536 * var_13 + -764784423 * var_10 + -1956198747 * var_5 + 1314182873 >= 0) goto BB8;
    goto BB16;

BB15:
    var_2 = -1124041844 * var_14 + 1469319309 * var_4 + -1555620916;
    if (-1103046751 * var_2 + 1436904233 * var_14 + 1183039397 * var_7 + -281137660 * var_5 + 1529025092 >= 0) goto BB20;
    goto BB25;

BB16:
    var_11 = 910734699 * var_11 + 135222792 * var_12 + -139030599;
    if (449926780 * var_11 + 1362728340 * var_2 + 1428644787 * var_12 + 1068804223 * var_7 + -1976497476 >= 0) goto BB5;
    goto BB18;

BB17:
    var_13 = 1443773294 * var_2 + -1922380517 * var_1 + -447414859;
    if (-1652589651 * var_13 + -875009593 * var_1 + 344331186 * var_7 + 1831125916 * var_5 + 620755372 >= 0) goto BB29;
    goto BB15;

BB18:
    var_3 = -1000711784 * var_10 + -1866531689 * var_11 + 2143074606;
    if (796321441 * var_3 + -738649778 * var_13 + -1426286893 * var_1 + 480196584 * var_9 + 1490287763 >= 0) goto BB19;
    goto BB22;

BB19:
    var_11 = -2063278143 * var_11 + -795721067 * var_0 + -477666400;
    if (-1151291823 * var_11 + 1070019000 * var_6 + -1373026588 * var_5 + 1635037337 * var_8 + 361246698 >= 0) goto BB10;
    goto BB12;

BB20:
    var_11 = -756987765 * var_5 + 2138345286 * var_12 + 1139241094;
    if (2103907454 * var_11 + -2053895996 * var_0 + -602773371 * var_4 + -1839115175 * var_8 + -1047150154 >= 0) goto BB23;
    goto BB7;

BB21:
    var_8 = -2010897204 * var_6 + 447939676 * var_10 + 646173297;
    if (923186678 * var_8 + 644119906 * var_11 + 303293874 * var_6 + 522773436 * var_3 + -461386620 >= 0) goto BB22;
    goto BB28;

BB22:
    var_7 = -469714732 * var_4 + -324518647 * var_8 + -1644151160;
    if (109961346 * var_7 + 931912676 * var_5 + 336882136 * var_14 + -572689893 * var_12 + -339550651 >= 0) goto BB8;
    goto BB9;

BB23:
    var_7 = -346782327 * var_13 + 1034926615 * var_7 + -1934258895;
    if (-689579533 * var_7 + -1219619260 * var_0 + 1631942200 * var_5 + -298294740 * var_14 + -395734736 >= 0) goto BB15;
    goto BB21;

BB24:
    var_10 = -1151211284 * var_7 + 926222611 * var_0 + 1269451873;
    if (-960671271 * var_10 + -2114628708 * var_12 + 2026290518 * var_14 + -837090689 * var_13 + -618444208 >= 0) goto BB8;
    goto BB7;

BB25:
    var_8 = -309716739 * var_8 + -155860437 * var_10 + 1773017440;
    if (1215153099 * var_8 + -287747360 * var_4 + -1896473847 * var_5 + -387728102 * var_0 + 874153714 >= 0) goto BB15;
    goto BB26;

BB26:
    var_12 = 959546529 * var_1 + -1575976346 * var_7 + -582435402;
    if (-1813028061 * var_12 + -2112979449 * var_2 + -588543302 * var_0 + -202348492 * var_9 + -2097298266 >= 0) goto BB29;
    goto BB10;

BB27:
    var_0 = -1484791838 * var_9 + -24050408 * var_10 + 2138964460;
    if (1670836296 * var_0 + -512576131 * var_2 + -1777801880 * var_12 + -510236282 * var_7 + -1212568897 >= 0) goto BB24;
    goto BB15;

BB28:
    var_6 = -1 * var_3 + -1 * var_8 + -1;
    goto BB9;

BB29:
    var_6 = -1 * var_3 + -1 * var_12 + -1;
    printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10, var_11, var_12, var_13, var_14);
    return 0;

}
#include <stdio.h>

int var_0 = -406; // Initial value from Z3 model
int var_1 = -6; // Initial value from Z3 model
int var_2 = 375; // Initial value from Z3 model
int var_3 = 894; // Initial value from Z3 model
int var_4 = 997; // Initial value from Z3 model
int var_5 = 729; // Initial value from Z3 model
int var_6 = 170; // Initial value from Z3 model
int var_7 = 851; // Initial value from Z3 model
int var_8 = 871; // Initial value from Z3 model
int var_9 = 158; // Initial value from Z3 model
int var_10 = 400; // Initial value from Z3 model
int var_11 = 178; // Initial value from Z3 model
int var_12 = -53; // Initial value from Z3 model
int var_13 = 452; // Initial value from Z3 model
int var_14 = -437; // Initial value from Z3 model

int pass_counter = 0;
int main() {
BB0:
    var_8 = -1 * var_12 + -1024 * var_9 + -2147321909;
    goto BB28;

BB1:
    var_5 = 1015225199 * var_7 + 1910302408 * var_4 + 821596223;
    goto BB27;

BB2:
    var_0 = -633114621 * var_13 + 2053735035 * var_12 + -772167240;
    if (1136763655 * var_0 + -1062391708 * var_9 + 559529910 * var_1 + 652219042 * var_12 + -1946324888 >= 0) goto BB4;
    goto BB3;

BB3:
    var_0 = -6 * var_5 + -3 * var_12 + -2147483281;
    if (-1 * var_0 + -1 * var_13 + -1024 * var_6 + -1 * var_12 + 173196 >= 0) goto BB29;
    goto BB2;

BB4:
    var_12 = -64745372 * var_2 + 883627712 * var_14 + -1570633690;
    if (-1406952983 * var_12 + -1788902307 * var_6 + 1288552820 * var_1 + -546659174 * var_0 + 1026928721 >= 0) goto BB20;
    goto BB7;

BB5:
    var_13 = -1606510590 * var_11 + 139567630 * var_12 + -1007946375;
    if (-632000161 * var_13 + -1921928002 * var_10 + -2073921785 * var_2 + -939870389 * var_14 + -1680277254 >= 0) goto BB17;
    goto BB20;

BB6:
    var_8 = -822414097 * var_8 + -413358517 * var_14 + 1059924202;
    if (-1759565641 * var_8 + -442661515 * var_13 + -105554917 * var_1 + -2143956447 * var_5 + -49548699 >= 0) goto BB13;
    goto BB14;

BB7:
    var_3 = 1462767936 * var_8 + -1908980512 * var_10 + -449390659;
    if (1384500141 * var_3 + -2006801761 * var_6 + 864227951 * var_8 + 489970704 * var_4 + -502116215 >= 0) goto BB8;
    goto BB19;

BB8:
    var_8 = 870775588 * var_13 + -787339658 * var_2 + -1200183831;
    if (-1959936530 * var_8 + 70781296 * var_3 + -397988306 * var_7 + -528974613 * var_11 + -1812635783 >= 0) goto BB10;
    goto BB13;

BB9:
    var_12 = -1 * var_13 + -1 * var_7 + -33;
    if (-1 * var_12 + -1024 * var_4 + -1024 * var_13 + -1 * var_1 + 1482434 >= 0) goto BB3;
    goto BB15;

BB10:
    var_9 = 1161626162 * var_12 + -590376310 * var_6 + 1850349025;
    if (-1803883725 * var_9 + -2101057250 * var_3 + 479911202 * var_5 + -9270616 * var_14 + 300660465 >= 0) goto BB17;
    goto BB11;

BB11:
    var_5 = -1977587124 * var_8 + 25795222 * var_5 + -998189341;
    if (1522114066 * var_5 + 549396609 * var_12 + -420999252 * var_11 + -1286970662 * var_8 + -1551331010 >= 0) goto BB7;
    goto BB20;

BB12:
    var_1 = 541844384 * var_14 + 270294195 * var_11 + -1787436581;
    if (751780570 * var_1 + -1460977803 * var_13 + 1993746662 * var_9 + 2035236380 * var_14 + -465815768 >= 0) goto BB13;
    goto BB9;

BB13:
    var_5 = 706221935 * var_9 + -586411430 * var_7 + 915530725;
    if (1165507559 * var_5 + 233269498 * var_12 + -797960895 * var_6 + 1489714807 * var_13 + 1955744047 >= 0) goto BB4;
    goto BB6;

BB14:
    var_13 = -2110285754 * var_7 + 1672449126 * var_10 + -1318324120;
    if (504126537 * var_13 + -2041090440 * var_11 + -732283798 * var_12 + 863569904 * var_8 + -628952851 >= 0) goto BB8;
    goto BB16;

BB15:
    var_13 = 1188701549 * var_4 + 2011505628 * var_5 + 2108568127;
    if (1159325155 * var_13 + -27793042 * var_11 + 772003910 * var_6 + -624189976 * var_2 + -27580175 >= 0) goto BB20;
    goto BB25;

BB16:
    var_6 = -1654615342 * var_4 + 2027803219 * var_12 + -1801822057;
    if (670852470 * var_6 + -130679319 * var_12 + -1200070762 * var_13 + 1960890227 * var_0 + 694634828 >= 0) goto BB5;
    goto BB18;

BB17:
    var_7 = -302045374 * var_9 + 176768875 * var_11 + -1173046118;
    if (363625342 * var_7 + -1223642790 * var_13 + -938631717 * var_9 + -185657943 * var_11 + 1215298985 >= 0) goto BB15;
    goto BB29;

BB18:
    var_3 = -1121773830 * var_6 + 1212783215 * var_10 + -894499184;
    if (2134042826 * var_3 + 1254935415 * var_11 + -1774223615 * var_7 + 1677033473 * var_13 + 1945236411 >= 0) goto BB19;
    goto BB22;

BB19:
    var_7 = -1117803213 * var_4 + 1704578579 * var_5 + 1165381366;
    if (288320609 * var_7 + 2143940615 * var_6 + 1505226350 * var_4 + 2102653377 * var_3 + -920644050 >= 0) goto BB10;
    goto BB12;

BB20:
    var_5 = 1164515413 * var_2 + 325652449 * var_1 + -1265769948;
    if (-2011895403 * var_5 + 1266280 * var_1 + 1900013064 * var_12 + 1099714619 * var_9 + -494566716 >= 0) goto BB7;
    goto BB23;

BB21:
    var_11 = -1748146714 * var_6 + -352799910 * var_11 + -304679300;
    if (-702798097 * var_11 + -1966307808 * var_2 + -66660193 * var_3 + -1484429470 * var_6 + 1559806956 >= 0) goto BB22;
    goto BB28;

BB22:
    var_2 = 44659632 * var_8 + 1698465468 * var_1 + 1136225215;
    if (1927388929 * var_2 + -760523131 * var_0 + 953737560 * var_8 + 1606911898 * var_4 + 978107558 >= 0) goto BB8;
    goto BB9;

BB23:
    var_0 = -1810706957 * var_7 + -793456512 * var_13 + 2094877801;
    if (-1316633987 * var_0 + 75539932 * var_11 + 288787724 * var_12 + 1715732159 * var_4 + -1547604518 >= 0) goto BB21;
    goto BB15;

BB24:
    var_2 = 1577838550 * var_5 + -379285090 * var_3 + 1301798532;
    if (-112660375 * var_2 + 1696967173 * var_1 + 408281435 * var_9 + 753930207 * var_7 + -1196215424 >= 0) goto BB8;
    goto BB7;

BB25:
    var_13 = 209981761 * var_12 + -1644121798 * var_6 + 1209070462;
    if (837968354 * var_13 + 385504665 * var_3 + -288883150 * var_6 + 907732917 * var_5 + 1641891830 >= 0) goto BB26;
    goto BB15;

BB26:
    var_0 = -1777567267 * var_7 + 84921324 * var_4 + -625188081;
    if (-1909885975 * var_0 + 1021408976 * var_8 + -1880628979 * var_14 + 2065918614 * var_4 + -401535223 >= 0) goto BB29;
    goto BB10;

BB27:
    var_7 = 1456248236 * var_4 + 1669732169 * var_1 + 909333178;
    if (-960201757 * var_7 + 2053174632 * var_2 + 2013943811 * var_10 + 1144299665 * var_0 + 1527639369 >= 0) goto BB15;
    goto BB24;

BB28:
    var_2 = -1 * var_2 + -1024 * var_12 + -1;
    goto BB9;

BB29:
    var_12 = -1024 * var_5 + -2 * var_2 + -2146629360;
    printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10, var_11, var_12, var_13, var_14);
    return 0;

}
