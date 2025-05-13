#include <stdio.h>

extern int var_0;
extern int var_1;
extern int var_2;
extern int var_3;
extern int var_4;
extern int var_5;
extern int var_6;
extern int var_7;
extern int pass_counter;

int main() {
BB0:
    var_1 = -1 * var_4 + -1 * var_7 + -293;
    goto BB3;

BB1:
    var_3 = 273096076 * var_6 + -1108542927 * var_3 + -1620291991;
    goto BB4;

BB2:
    var_5 = -2031799643 * var_4 + -637336717 * var_2 + 127994785;
    if (640530843 * var_5 + 233974482 * var_3 + -1494451507 * var_1 + 1624690023 * var_6 + -423793053 >= 0) goto BB4;
    goto BB8;

BB3:
    var_1 = -1 * var_3 + -2 * var_1 + -1;
    if (-1 * var_1 + -1 * var_4 + -1 * var_5 + -1 * var_6 + -2147481850 >= 0) goto BB5;
    goto BB4;

BB4:
    var_1 = -1 * var_4 + -1024 * var_2 + -1;
    if (-1 * var_1 + -1024 * var_2 + -1 * var_3 + -1024 * var_4 + -2146834485 >= 0) goto BB7;
    goto BB9;

BB5:
    var_4 = -166439362 * var_0 + -1007017283 * var_1 + -1119391322;
    if (1343378747 * var_4 + 1205196723 * var_6 + 2115607813 * var_7 + 1162285984 * var_2 + -660152190 >= 0) goto BB6;
    goto BB2;

BB6:
    var_3 = -458520158 * var_4 + -102253097 * var_2 + -513972170;
    goto BB2;

BB7:
    var_4 = 315036696 * var_2 + 940062705 * var_1 + -1863739602;
    goto BB1;

BB8:
    var_1 = -147593361 * var_4 + -339018384 * var_6 + 821683160;
    goto BB3;

BB9:
    var_2 = -1 * var_5 + -1024 * var_4 + -1;
    printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
    return 0;

}
