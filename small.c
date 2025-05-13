#include <stdio.h>

int var_0 = 1; // Initial value from Z3 model
int var_1 = -1; // Initial value from Z3 model
int var_2 = -1; // Initial value from Z3 model
int var_3 = -1; // Initial value from Z3 model
int var_4 = -1; // Initial value from Z3 model
int var_5 = -1; // Initial value from Z3 model
int var_6 = -1; // Initial value from Z3 model
int var_7 = -1; // Initial value from Z3 model

int main() {
BB0:
    var_0 = -2147483646 * var_6 + -1 * var_4 + -2147483523;
    goto BB9;

BB1:
    var_3 = -628107758 * var_0 + 1178621026 * var_7 + 1370739142;
    if (1860083615 * var_3 + 331081179 * var_4 + 658769679 >= 0) goto BB6;
    goto BB2;

BB2:
    var_0 = -62 * var_7 + -62 * var_3 + -1;
    if (-1 * var_0 + -1 * var_1 + 122 >= 0) goto BB5;
    goto BB3;

BB3:
    var_7 = 872926641 * var_7 + -647060199 * var_2 + -1685361788;
    if (1367585682 * var_7 + -256179441 * var_0 + 1704055139 >= 0) goto BB4;
    goto BB2;

BB4:
    var_1 = 1282547178 * var_6 + -1733856615 * var_0 + -1302905725;
    if (-503061704 * var_1 + -670011140 * var_4 + -1918663653 >= 0) goto BB7;
    goto BB10;

BB5:
    var_7 = -1 * var_2 + -2 * var_4 + -6;
    if (-1 * var_7 + -1 * var_3 + 0 >= 0) goto BB9;
    goto BB7;

BB6:
    var_2 = -995926843 * var_5 + 1541515338 * var_2 + -496946;
    if (2060970672 * var_2 + 616834814 * var_3 + -1128841633 >= 0) goto BB9;
    goto BB3;

BB7:
    var_2 = -1182779127 * var_1 + 1079745253 * var_3 + 1828407529;
    if (-940132721 * var_2 + 1988615235 * var_5 + -378797178 >= 0) goto BB8;
    goto BB4;

BB8:
    var_2 = -335242621 * var_1 + 1268574921 * var_6 + -1615548643;
    if (-811776912 * var_2 + 669095598 * var_4 + -1950043853 >= 0) goto BB11;
    goto BB7;

BB9:
    var_4 = -246 * var_3 + -2 * var_0 + -1;
    if (-2 * var_4 + -1 * var_1 + -3 >= 0) goto BB2;
    goto BB11;

BB10:
    var_1 = 1017738052 * var_5 + 353817461 * var_0 + 1448521553;
    if (1490814150 * var_1 + 2086755464 * var_3 + -1252467186 >= 0) goto BB6;
    goto BB11;

BB11:
    var_5 = -1 * var_6 + -1 * var_5 + -2147483648;
    printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);
    return 0;

}
