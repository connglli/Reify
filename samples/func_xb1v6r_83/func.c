extern int computeStatelessChecksum(int num_args, int args[]);
int func_xb1v6r_83(int v0, int v1, int v2, int v3, int v4, int v5) {
  int v6 = -1;
  int v7 = -1;
BB0:
  v1 = (-1 * v4) - (2 - v7) - (-1);
  v7 = (-2 - v1) - (-1 * v4) - (-1);
  goto BB11;
BB1:
  v5 = (0 * v5) + (-2147483648);
  v1 = (0 * v1) + (-2147483648);
  v5 = (0 * v5) + (-2147483648);
  v7 = (0 * v7) + (-1);
  v0 = (0 + v5) - (0 * v1) - (0);
  v1 = (0 % v5) + (0 / v7) + (0);
  if ((0 % v0) - (0 + v1) - (0 * v6) - (0) == 0) goto BB2;
  goto BB3;
BB2:
  v1 = (0 * v3) - (0 * v5) - (0);
  v0 = (0 % v2) - (0 % v0) - (0);
  if ((0 % v1) - (0 / v0) - (0 - v3) - (0) == 0) goto BB5;
  goto BB8;
BB3:
  v7 = (0 * v7) + (-1);
  v5 = (0 * v5) + (-2147483648);
  v7 = (0 * v7) + (-1);
  v2 = (0 * v2) + (1);
  v0 = (0 * v7) + (0 * v5) + (0);
  v2 = (0 / v7) + (0 - v2) + (0);
  if ((0 * v0) + (0 * v2) + (0 + v1) + (0) < 0) goto BB8;
  goto BB13;
BB4:
  v5 = (0 - v3) - (0 + v5) - (0);
  v6 = (0 - v3) + (0 % v2) + (0);
  if ((0 * v5) + (0 % v6) + (0 / v1) + (0) == 0) goto BB7;
  goto BB11;
BB5:
  v3 = (0 % v2) + (0 - v3) + (0);
  v0 = (0 * v1) + (0 / v5) + (0);
  if ((0 / v3) - (0 - v0) - (0 / v4) - (0) == 0) goto BB6;
  goto BB9;
BB6:
  v7 = (0 / v6) + (0 % v1) + (0);
  v2 = (0 - v2) - (0 - v3) - (0);
  if ((0 * v7) + (0 % v2) + (0 * v4) + (0) == 0) goto BB7;
  goto BB10;
BB7:
  v5 = (0 - v7) - (0 - v2) - (0);
  v1 = (0 * v0) - (0 % v1) - (0);
  if ((0 / v5) + (0 + v1) + (0 + v2) + (0) < 0) goto BB2;
  goto BB11;
BB8:
  v0 = (0 + v7) - (0 * v6) - (0);
  v7 = (0 / v3) + (0 / v0) + (0);
  if ((0 + v0) + (0 + v7) + (0 / v3) + (0) < 0) goto BB3;
  goto BB5;
BB9:
  v0 = (0 * v3) + (0 / v1) + (0);
  v4 = (0 % v6) - (0 + v0) - (0);
  if ((0 / v0) - (0 + v4) - (0 * v6) - (0) > 0) goto BB11;
  goto BB14;
BB10:
  v1 = (0 + v7) + (0 + v4) + (0);
  v3 = (0 / v7) + (0 / v6) + (0);
  if ((0 % v1) + (0 + v3) + (0 - v4) + (0) > 0) goto BB4;
  goto BB13;
BB11:
  v5 = (-1 / v4) - (-1 + v5) - (-1);
  v0 = (-1 + v5) - (-1 + v1) - (-1);
  if ((-1 / v5) - (-1 % v0) - (-1 * v1) - (-1) > 0) goto BB12;
  goto BB14;
BB12:
  v0 = (0 / v4) + (0 + v2) + (0);
  v1 = (0 % v7) - (0 + v4) - (0);
  if ((0 / v0) - (0 + v1) - (0 % v5) - (0) < 0) goto BB5;
  goto BB13;
BB13:
  v7 = (0 % v2) - (0 / v1) - (0);
  v6 = (0 % v3) + (0 * v6) + (0);
  goto BB7;
BB14:
  v1 = (-1 / v6) - (2 - v3) - (-1);
  v0 = (-4 - v3) - (-1 % v5) - (-1);
  {
    int __chk_args[] = {v0, v1, v2, v3, v4, v5};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 1392964597 --Xbitwuzla-threads 8 -o samples -n 83 xb1v6r