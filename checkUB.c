#include <stdio.h>

static int var_0 = 1; // Initial value from Z3 model
static int var_1 = 1; // Initial value from Z3 model
static int var_2 = 1; // Initial value from Z3 model
static int var_3 = 1; // Initial value from Z3 model
static int var_4 = 1; // Initial value from Z3 model

int main() {
BB0:
    var_1 = 0 * var_4 + -2147483646 * var_0 + 2147483646;
    var_2 = -2147483646 * var_0 + 0 * var_2 + 2147483646;
    if (-2147483647 * var_1 + 0 * var_2 + -2147483647 * var_4 + 2147483647 >= 0) goto BB5;
    goto BB3;

BB1:
    var_0 = -444044651 * var_1 + 783270972 * var_4 + -2047738311;
    var_4 = -105892211 * var_1 + -1510334872 * var_4 + -983340092;
    if (690693014 * var_4 + 1466674636 * var_0 + 529072180 * var_3 + 600181550 >= 0) goto BB2;
    goto BB4;

BB2:
    var_1 = -1950544695 * var_1 + 648041750 * var_4 + 1554202684;
    var_2 = 483008806 * var_4 + -335286568 * var_3 + -1287976330;
    if (541221589 * var_2 + 141441971 * var_1 + 864015964 * var_3 + 1043563298 >= 0) goto BB9;
    goto BB3;

BB3:
    var_0 = 1322607316 * var_0 + -1166736291 * var_1 + -2110668432;
    var_3 = -729281782 * var_0 + 1874797123 * var_2 + -693876400;
    if (-176386926 * var_0 + -17293431 * var_3 + 888513060 * var_1 + 1472560921 >= 0) goto BB6;
    goto BB1;

BB4:
    var_1 = -2147483647 * var_3 + -2147483647 * var_1 + 0;
    var_3 = 0 * var_3 + -1 * var_4 + -2147483647;
    if (-1 * var_1 + 0 * var_3 + -1 * var_4 + 0 >= 0) goto BB10;
    goto BB5;

BB5:
    var_1 = 0 * var_1 + -2147483646 * var_3 + 2147483646;
    var_4 = 0 * var_1 + -2147483646 * var_0 + 2147483646;
    if (0 * var_1 + -2147483647 * var_4 + -2147483647 * var_3 + -1 >= 0) goto BB10;
    goto BB6;

BB6:
    var_1 = 0 * var_3 + -2147483646 * var_0 + 2147483646;
    var_4 = 0 * var_3 + -2147483647 * var_2 + -2147483647;
    if (1 * var_1 + 0 * var_4 + -2147483647 * var_3 + -1 >= 0) goto BB10;
    goto BB7;

BB7:
    var_1 = 1 * var_3 + 0 * var_4 + -1;
    var_3 = -2147483646 * var_0 + 0 * var_1 + 2147483646;
    if (0 * var_3 + -2147483646 * var_1 + 1 * var_2 + -2147483648 >= 0) goto BB1;
    goto BB4;

BB8:
    var_2 = 1387104639 * var_2 + 1950742289 * var_1 + 550265110;
    var_4 = 949285485 * var_4 + 257675731 * var_2 + -1343565767;
    goto BB5;

BB9:
    var_0 = -1302525951 * var_3 + -282497827 * var_1 + -762880142;
    var_3 = -445431619 * var_2 + 2034190397 * var_3 + -33509624;
    if (928204502 * var_3 + -1277675783 * var_0 + 1072852693 * var_1 + 340809481 >= 0) goto BB10;
    goto BB3;

BB10:
    var_2 = -2147483646 * var_0 + 0 * var_4 + 2147483646;
    var_4 = -1 * var_1 + -1 * var_0 + 1;
    if (-1 * var_4 + 0 * var_2 + -2147483647 * var_0 + 0 >= 0) goto BB11;
    goto BB5;

BB11:
    var_0 = 1 * var_2 + 0 * var_1 + -2147483647;
    var_4 = 0 * var_0 + -1 * var_1 + -1;
    printf("%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4);
    return 0;

}
