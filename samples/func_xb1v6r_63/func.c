extern int computeStatelessChecksum(int num_args, int args[]);
int func_xb1v6r_63(int v0, int v1, int v2, int v3, int v4[1][2], int v5) {
  int v6[2][3] = {-1, -1, -1, -1, -1, -1};
  int v7 = 0;
BB0:
  v3 = (-1 * v1) - (-1 * v6[1][2]) - (1);
  v4[0][1] = (-1 % v6[1][2]) + (-1 - v2) + (-1);
  goto BB4;
BB1:
  v7 = (0 * v1) + (0 + v0) + (0);
  v2 = (0 % v0) + (0 - v4[0][1]) + (0);
  if ((0 / v7) - (0 % v2) - (0 / v1) - (0) > 0) goto BB2;
  goto BB6;
BB2:
  v5 = (0 * v5) + (-1);
  v6[1][2] = (0 * v6[1][2]) + (-1);
  v2 = (0 * v2) + (-2147483648);
  v4[0][1] = (0 % v5) + (0 - v6[1][2]) + (0);
  v6[1][2] = (0 + v2) - (0 % v4[0][1]) - (0);
  if ((0 - v4[0][1]) - (0 % v6[1][2]) - (0 % v5) - (0) == 0) goto BB6;
  goto BB9;
BB3:
  v2 = (0 * v2) + (1073741824);
  v5 = (0 * v5) + (-2147483648);
  v2 = (0 * v2) + (1073741824);
  v7 = (0 * v7) + (-1073741825);
  v1 = (0 * v2) + (0 * v5) + (0);
  v2 = (0 - v2) + (0 + v7) + (0);
  goto BB6;
BB4:
  v3 = (-1 % v4[0][1]) + (-1 * v0) + (-1);
  v1 = (-1 + v5) - (-1 / v0) - (-1);
  if ((-1 + v3) + (-1 + v1) + (-1 - v2) + (-1) == 0) goto BB5;
  goto BB14;
BB5:
  v1 = (0 * v6[1][2]) + (0 % v3) + (0);
  v3 = (0 + v7) - (0 / v2) - (0);
  if ((0 / v1) - (0 % v3) - (0 * v7) - (0) > 0) goto BB6;
  goto BB13;
BB6:
  v6[1][2] = (0 - v5) + (0 - v2) + (0);
  v2 = (0 + v5) + (0 % v7) + (0);
  if ((0 / v6[1][2]) + (0 % v2) + (0 + v5) + (0) > 0) goto BB4;
  goto BB7;
BB7:
  v1 = (0 / v1) - (0 * v3) - (0);
  v3 = (0 * v7) - (0 % v3) - (0);
  if ((0 / v1) + (0 * v3) + (0 * v2) + (0) > 0) goto BB4;
  goto BB10;
BB8:
  v3 = (0 * v2) - (0 % v1) - (0);
  v7 = (0 % v4[0][1]) - (0 - v3) - (0);
  if ((0 + v3) + (0 - v7) + (0 + v2) + (0) > 0) goto BB2;
  goto BB10;
BB9:
  v7 = (0 % v1) + (0 - v0) + (0);
  v4[0][1] = (0 + v1) + (0 + v6[1][2]) + (0);
  if ((0 - v7) + (0 / v4[0][1]) + (0 * v3) + (0) > 0) goto BB4;
  goto BB13;
BB10:
  v1 = (0 - v7) + (0 * v3) + (0);
  v2 = (0 % v2) + (0 % v1) + (0);
  goto BB11;
BB11:
  v1 = (0 / v5) + (0 + v6[1][2]) + (0);
  v2 = (0 + v4[0][1]) - (0 + v3) - (0);
  goto BB6;
BB12:
  v3 = (0 - v3) - (0 * v1) - (0);
  v7 = (0 * v6[1][2]) - (0 - v0) - (0);
  if ((0 * v3) - (0 - v7) - (0 - v1) - (0) > 0) goto BB1;
  goto BB8;
BB13:
  v2 = (0 / v7) - (0 * v3) - (0);
  v1 = (0 * v3) + (0 / v2) + (0);
  goto BB12;
BB14:
  v5 = (-1 + v2) - (-1 % v4[0][1]) - (-1);
  v7 = (-1 % v0) + (-1 / v4[0][1]) + (-1);
  {
    int __chk_args[] = {v0, v1, v2, v3, v4[0][0], v4[0][1], v5};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 801997237 --Xbitwuzla-threads 8 -o samples -n 63 xb1v6r