extern int computeStatelessChecksum(int num_args, int args[]);
int func_xb1v6r_54(int v0, int v1, int v2, int v3, int v4, int v5) {
  int v6 = -1;
  int v7 = 0;
BB0:
  v0 = (-1 + v2) - (-1 / v3) - (-1);
  v7 = (-1 % v4) + (-1 + v6) + (-1);
  if ((-1 / v0) + (-1 / v7) + (-1 + v6) + (-1) == 0) goto BB3;
  goto BB4;
BB1:
  v2 = (0 % v0) + (0 / v2) + (0);
  v7 = (0 % v1) + (0 + v6) + (0);
  if ((0 / v2) - (0 + v7) - (0 * v3) - (0) < 0) goto BB5;
  goto BB6;
BB2:
  v4 = (0 % v6) - (0 / v1) - (0);
  v2 = (0 / v2) + (0 * v3) + (0);
  if ((0 + v4) + (0 + v2) + (0 - v6) + (0) == 0) goto BB8;
  goto BB10;
BB3:
  v4 = (0 % v5) - (0 + v6) - (0);
  v1 = (0 % v5) + (0 / v6) + (0);
  if ((0 * v4) - (0 - v1) - (0 - v3) - (0) == 0) goto BB5;
  goto BB11;
BB4:
  v6 = (-1 / v3) + (-1 - v7) + (-1);
  v5 = (-1 / v4) - (2 - v3) - (-1);
  if ((0 - v6) - (-1 * v5) - (-1 + v0) - (-1) == 0) goto BB9;
  goto BB10;
BB5:
  v6 = (0 + v1) - (0 % v5) - (0);
  v5 = (0 / v5) - (0 % v3) - (0);
  if ((0 * v6) - (0 % v5) - (0 + v1) - (0) > 0) goto BB1;
  goto BB11;
BB6:
  v2 = (0 / v0) + (0 + v3) + (0);
  v3 = (0 / v5) + (0 / v0) + (0);
  if ((0 - v2) + (0 + v3) + (0 / v5) + (0) == 0) goto BB2;
  goto BB5;
BB7:
  v6 = (0 + v2) - (0 - v0) - (0);
  v2 = (0 * v4) - (0 - v7) - (0);
  if ((0 - v6) + (0 * v2) + (0 + v5) + (0) < 0) goto BB8;
  goto BB9;
BB8:
  v2 = (0 * v2) + (-1);
  v3 = (0 * v3) + (-1);
  v2 = (0 * v2) + (-1);
  v5 = (0 * v5) + (1);
  v1 = (0 % v2) + (0 - v3) + (0);
  v5 = (0 / v2) - (0 + v5) - (0);
  if ((0 / v1) + (0 + v5) + (0 + v7) + (0) < 0) goto BB4;
  goto BB6;
BB9:
  v2 = (-1 % v0) - (-1 * v5) - (0);
  v1 = (-1 % v6) + (-1 * v5) + (-1);
  if ((-1 + v2) + (-1 - v1) + (-1 * v6) + (2147483647) < 0) goto BB13;
  goto BB18;
BB10:
  v4 = (0 % v5) - (0 * v0) - (0);
  v7 = (0 * v3) + (0 % v7) + (0);
  if ((0 / v4) - (0 * v7) - (0 / v0) - (0) < 0) goto BB12;
  goto BB13;
BB11:
  v6 = (0 / v5) - (0 - v7) - (0);
  v4 = (0 / v6) - (0 - v2) - (0);
  if ((0 / v6) + (0 / v4) + (0 / v0) + (0) > 0) goto BB3;
  goto BB7;
BB12:
  v7 = (0 * v0) - (0 % v4) - (0);
  v3 = (0 + v7) - (0 % v2) - (0);
  if ((0 - v7) - (0 / v3) - (0 % v1) - (0) < 0) goto BB4;
  goto BB13;
  do {
BB13:
    v0 = (0 * v0) + (1);
    v3 = (0 * v3) + (1);
    v6 = (0 * v6) + (-2147483648);
    v2 = (0 * v2) + (1);
    v7 = (0 % v0) + (0 + v3) + (0);
    v4 = (0 - v6) + (0 / v2) + (0);
    if ((0 % v7) - (0 * v4) - (0 / v6) - (0) > 0) goto BB14;
    goto BB16;
BB14:
    v4 = (0 / v0) - (0 * v5) - (0);
    v5 = (0 % v1) + (0 * v0) + (0);
    goto BB15;
BB15:
    v0 = (0 * v0) + (-1);
    v7 = (0 * v7) + (-2147483648);
    v3 = (0 * v3) + (-2147483648);
    v0 = (0 * v0) + (-1);
    v5 = (0 / v0) - (0 * v7) - (0);
    v2 = (0 + v3) - (0 / v0) - (0);
    goto BB16;
BB16:
    v1 = (0 * v6) - (0 % v5) - (0);
    v2 = (0 / v4) + (0 / v3) + (0);
  } while ((0 % v1) - (0 * v2) - (0 / v7) - (0) == 0);
  goto BB17;
BB17:
  v6 = (0 % v3) - (0 + v6) - (0);
  v2 = (0 * v4) + (0 * v0) + (0);
  goto BB4;
BB18:
  v4 = (-1 - v1) + (-1 + v4) + (-1);
  v3 = (-1 % v7) + (-1 - v2) + (-1);
  {
    int __chk_args[] = {v0, v1, v2, v3, v4, v5};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 1235478542 --Xbitwuzla-threads 8 -o samples -n 54 xb1v6r