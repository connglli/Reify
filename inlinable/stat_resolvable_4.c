#include <stdio.h>

int var_0 = 599; // Initial value from Z3 model
int var_1 = 797; // Initial value from Z3 model
int var_2 = 40; // Initial value from Z3 model
int var_3 = -238; // Initial value from Z3 model
int var_4 = 360; // Initial value from Z3 model
int var_5 = 180; // Initial value from Z3 model
int var_6 = -997; // Initial value from Z3 model
int var_7 = -566; // Initial value from Z3 model
int var_8 = 843; // Initial value from Z3 model
int var_9 = 74; // Initial value from Z3 model
int var_10 = -349; // Initial value from Z3 model
int var_11 = -227; // Initial value from Z3 model
int var_12 = 445; // Initial value from Z3 model
int var_13 = -814; // Initial value from Z3 model
int var_14 = 934; // Initial value from Z3 model

int pass_counter = 0;
int main() {
BB0:
    var_3 = -1 * var_2 + -1 * var_12 + -1;
    if (-1 * var_3 + -1 * var_8 + -1 * var_12 + -1 * var_2 + 841 >= 0) goto BB14;
    goto BB23;

BB1:
    var_7 = 693660638 * var_11 + 659895883 * var_8 + 344178707;
    if (250714551 * var_7 + -1742978555 * var_6 + -1642487512 * var_4 + 1938784068 * var_14 + -1322141179 >= 0) goto BB3;
    goto BB2;

BB2:
    var_7 = -1379207374 * var_7 + -178183983 * var_3 + -1328300030;
    if (-995636715 * var_7 + -1766066623 * var_3 + 972256049 * var_9 + 93543825 * var_2 + 1602668799 >= 0) goto BB21;
    goto BB24;

BB3:
    var_11 = 852410525 * var_7 + -1935218434 * var_3 + -599668342;
    if (1646700419 * var_11 + 1899244319 * var_0 + -1491546878 * var_5 + 895221692 * var_3 + 1307416577 >= 0) goto BB24;
    goto BB19;

BB4:
    var_10 = -350943139 * var_9 + 1227543733 * var_14 + -1484278178;
    if (-691271549 * var_10 + -1436981072 * var_7 + 1661407064 * var_0 + -636099011 * var_12 + -825500044 >= 0) goto BB29;
    goto BB23;

BB5:
    var_1 = 644077377 * var_0 + -1766994213 * var_6 + 1360633380;
    if (-1841088917 * var_1 + 713661829 * var_11 + 551774058 * var_4 + 224493390 * var_12 + 229151176 >= 0) goto BB7;
    goto BB2;

BB6:
    var_10 = -165758691 * var_6 + 1092604442 * var_7 + 1036216987;
    goto BB27;

BB7:
    var_1 = 319062584 * var_5 + 625672660 * var_13 + 1151992392;
    if (593950575 * var_1 + 1269419617 * var_0 + -978594682 * var_12 + 2131685121 * var_9 + 1019740841 >= 0) goto BB20;
    goto BB11;

BB8:
    var_1 = 523739190 * var_13 + -1229337407 * var_1 + 1102421028;
    if (-604871883 * var_1 + 1954167200 * var_2 + -33022899 * var_14 + -2119311695 * var_6 + -551148719 >= 0) goto BB28;
    goto BB12;

BB9:
    var_10 = -2139364452 * var_6 + 326226291 * var_0 + -161168388;
    if (-614005193 * var_10 + -2007328613 * var_12 + 535700835 * var_0 + -1026517803 * var_14 + -165030093 >= 0) goto BB15;
    goto BB6;

BB10:
    var_8 = -2131817835 * var_7 + 1201200816 * var_12 + -180593187;
    if (348037272 * var_8 + -1299973159 * var_6 + 1986445453 * var_14 + -620975247 * var_5 + 471108911 >= 0) goto BB18;
    goto BB29;

BB11:
    var_13 = 1468561553 * var_10 + 1742119144 * var_8 + -1984124042;
    if (1417693189 * var_13 + -1825195282 * var_1 + 1695223352 * var_0 + 221845211 * var_7 + -873603503 >= 0) goto BB28;
    goto BB5;

BB12:
    var_10 = -1 * var_7 + -1 * var_1 + -1;
    if (-1 * var_10 + -1 * var_1 + -1 * var_5 + -1 * var_3 + 2397 >= 0) goto BB18;
    goto BB17;

BB13:
    var_7 = -138183143 * var_0 + -1116378047 * var_3 + -1372333835;
    if (848665756 * var_7 + 227784303 * var_1 + 707468897 * var_13 + 1297397951 * var_9 + -109342842 >= 0) goto BB12;
    goto BB20;

BB14:
    var_7 = -450477991 * var_3 + -1169507964 * var_8 + -1199130293;
    if (-595245872 * var_7 + -1193172253 * var_9 + -1584426308 * var_2 + 731293051 * var_11 + -1729307481 >= 0) goto BB13;
    goto BB27;

BB15:
    var_0 = -635374230 * var_3 + 1451598189 * var_8 + -1974815881;
    goto BB4;

BB16:
    var_0 = 886815049 * var_14 + 1483198125 * var_9 + -821862205;
    if (-1389903572 * var_0 + 1537055886 * var_5 + 1196794425 * var_2 + -1366741010 * var_12 + 1961428078 >= 0) goto BB19;
    goto BB2;

BB17:
    var_6 = -1053489763 * var_5 + -1373393168 * var_7 + 760311902;
    if (-1360578174 * var_6 + 1246052069 * var_7 + -960200720 * var_8 + -1216394059 * var_3 + -291075826 >= 0) goto BB23;
    goto BB10;

BB18:
    var_6 = -1 * var_12 + -1 * var_14 + -1;
    if (-1 * var_6 + -1 * var_13 + -1 * var_12 + -1 * var_3 + -957 >= 0) goto BB22;
    goto BB21;

BB19:
    var_2 = 255853181 * var_8 + -226770264 * var_6 + 519709155;
    if (-1253280587 * var_2 + -1613005767 * var_0 + 990663233 * var_3 + 975428490 * var_6 + 1119555046 >= 0) goto BB22;
    goto BB26;

BB20:
    var_4 = 1584816936 * var_12 + 704890738 * var_4 + 2010772234;
    if (-1880519941 * var_4 + 1291634133 * var_9 + -629822900 * var_3 + -1114746283 * var_7 + 1350416695 >= 0) goto BB18;
    goto BB12;

BB21:
    var_2 = -1700725980 * var_3 + -16555018 * var_11 + -1201596261;
    if (-1517638565 * var_2 + -300406194 * var_8 + 1494127524 * var_12 + -1523802690 * var_4 + -1152786807 >= 0) goto BB1;
    goto BB19;

BB22:
    var_1 = -1 * var_10 + -1 * var_3 + -1;
    if (-1 * var_1 + -1 * var_11 + -1 * var_13 + -1 * var_2 + -1563 >= 0) goto BB10;
    goto BB29;

BB23:
    var_5 = -1 * var_11 + -1 * var_13 + -1;
    if (-1 * var_5 + -1 * var_1 + -1 * var_3 + -1 * var_4 + 1710 >= 0) goto BB4;
    goto BB25;

BB24:
    var_1 = -1735768494 * var_11 + 430282121 * var_3 + 194345457;
    if (923226598 * var_1 + 155816947 * var_10 + 2008902964 * var_5 + 1694427721 * var_0 + 1454586532 >= 0) goto BB13;
    goto BB29;

BB25:
    var_3 = -1 * var_7 + -1 * var_11 + -1;
    if (-1 * var_3 + -1 * var_0 + -1 * var_2 + -1 * var_5 + 2471 >= 0) goto BB12;
    goto BB5;

BB26:
    var_12 = -1907709421 * var_7 + -1062156417 * var_6 + 1010304334;
    if (384998248 * var_12 + -552776164 * var_11 + -2104021318 * var_7 + -1041181313 * var_2 + -111703234 >= 0) goto BB28;
    goto BB24;

BB27:
    var_9 = 267732111 * var_1 + 1105773571 * var_12 + 1540928792;
    if (313056363 * var_9 + 1824173258 * var_8 + 1938752729 * var_14 + 1804626229 * var_2 + -986873922 >= 0) goto BB2;
    goto BB13;

BB28:
    var_7 = 407226382 * var_2 + 482018661 * var_0 + -1523562762;
    goto BB21;

BB29:
    var_4 = -1 * var_2 + -1 * var_13 + -1;
    printf("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, var_10, var_11, var_12, var_13, var_14);
    return 0;

}
