#include <stdio.h>

static int var_0 = 0;
static int var_1 = 0;
static int var_2 = 0;
static int var_3 = 0;
static int var_4 = 0;
static int var_5 = 0;
static int var_6 = 0;
static int var_7 = 0;

int main() {
BB0:
    var_3 = 623153173 * var_2 + 911113341 * var_7 + -672233315;
    if (1762899276 * var_3 + 291585467 * var_0 + 287322903 * var_1 + 1753116777 * var_7 + 218234363 >= 0) goto BB1;
    goto BB2;

BB1:
    var_7 = -300209210 * var_1 + -744738919 * var_7 + -34034880;
    if (-26479352 * var_7 + -1240383546 * var_2 + -1112274377 * var_4 + 1292296258 * var_0 + 1631462328 >= 0) goto BB4;
    goto BB8;

BB2:
    var_1 = -334997371 * var_6 + -1433456299 * var_7 + -1170892709;
    if (-1481337001 * var_1 + -486289934 * var_3 + 1585033590 * var_7 + 443924439 * var_6 + -930329969 >= 0) goto BB4;
    goto BB8;

BB3:
    var_5 = 1443496269 * var_7 + 1207480579 * var_1 + -926060049;
    if (-321271519 * var_5 + 206720999 * var_7 + -1874173650 * var_0 + 1013654261 * var_4 + 973372207 >= 0) goto BB8;
    goto BB4;

BB4:
    var_4 = -1915894619 * var_7 + -890952795 * var_6 + -1961226204;
    if (-804756369 * var_4 + -986462231 * var_6 + 781346678 * var_3 + 745037539 * var_0 + 1624328677 >= 0) goto BB3;
    goto BB6;

BB5:
    var_2 = 1134101022 * var_0 + -793649547 * var_1 + 599837869;
    goto BB8;

BB6:
    var_5 = 599503930 * var_2 + -1788380695 * var_6 + 1383272172;
    if (545069844 * var_5 + 1018101989 * var_0 + -702126053 * var_4 + -1459442261 * var_3 + -1374116793 >= 0) goto BB7;
    goto BB9;

BB7:
    var_2 = 940782445 * var_7 + 1951997756 * var_2 + -1815102796;
    goto BB2;

BB8:
pass_counter++;
    var_4 = -160394733 * var_4 + -824307268 * var_6 + 321522738;
    if(pass_counter >= 247)
    {
        goto BB9;
    }
    goto BB3;

BB9:
    var_2 = -1563631739 * var_4 + 390190962 * var_1 + -1570114006;
    printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
    return 0;

}
