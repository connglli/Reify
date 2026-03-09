extern int computeStatelessChecksum(int num_args, int args[]);
struct func_xb1v6r_78_S0 {
  int f0;
};
int func_xb1v6r_78(int v0, int v1[2], struct func_xb1v6r_78_S0 v2, int v3, int v4, int v5[2]) {
  int v6 = -1;
  int v7 = 0;
BB0:
  v7 = (-1 + v4) - (-1 - v0) - (-1);
  v2.f0 = (-1 % v5[1]) - (-1 % v4) - (0);
  goto BB7;
BB1:
  v0 = (-1 + v0) - (-1 + v7) - (-1);
  v1[1] = (-1 * v7) - (-1 + v5[1]) - (-2);
  if ((-1 * v0) - (-1 - v1[1]) - (-1 * v4) - (-1) == 0) goto BB3;
  goto BB5;
BB2:
  v0 = (-1 / v6) + (-1 - v0) + (-1);
  v7 = (-1 * v6) - (-1 % v4) - (2);
  if ((-1 * v0) + (-1 / v7) + (-1 * v6) + (-2) > 0) goto BB7;
  goto BB14;
BB3:
  v0 = (0 - v2.f0) - (0 - v3) - (0);
  v3 = (0 / v7) - (0 / v4) - (0);
  if ((0 % v0) - (0 / v3) - (0 / v6) - (0) > 0) goto BB11;
  goto BB14;
BB4:
  v0 = (0 + v2.f0) + (0 + v6) + (0);
  v3 = (0 % v5[1]) - (0 / v7) - (0);
  if ((0 / v0) - (0 % v3) - (0 + v7) - (0) < 0) goto BB9;
  goto BB13;
BB5:
  v3 = (2 - v5[1]) - (-1 / v1[1]) - (-1);
  v5[1] = (-1 + v6) + (-1 * v7) + (-1);
  if ((-1 % v3) - (-3 - v5[1]) - (-1 - v2.f0) - (-1) == 0) goto BB2;
  goto BB13;
BB6:
  v0 = (0 + v4) + (0 % v3) + (0);
  v4 = (0 % v7) + (0 / v4) + (0);
  if ((0 * v0) + (0 / v4) + (0 % v2.f0) + (0) > 0) goto BB11;
  goto BB12;
BB7:
  v1[1] = (-1 % v0) - (-1 + v5[1]) - (-1);
  v7 = (-1 * v1[1]) + (-1 * v7) + (-1);
  if ((-1 * v1[1]) - (2 - v7) - (-1 / v6) - (-1) == 0) goto BB1;
  goto BB8;
BB8:
  v3 = (0 % v4) - (0 / v3) - (0);
  v2.f0 = (0 / v3) + (0 * v5[1]) + (0);
  if ((0 * v3) + (0 % v2.f0) + (0 - v4) + (0) > 0) goto BB2;
  goto BB10;
BB9:
  v4 = (0 - v7) + (0 % v4) + (0);
  v3 = (0 - v2.f0) + (0 % v5[1]) + (0);
  if ((0 / v4) + (0 * v3) + (0 + v5[1]) + (0) < 0) goto BB1;
  goto BB12;
BB10:
  v7 = (0 / v1[1]) + (0 + v2.f0) + (0);
  v4 = (0 + v3) - (0 * v7) - (0);
  if ((0 - v7) - (0 - v4) - (0 - v1[1]) - (0) < 0) goto BB2;
  goto BB4;
BB11:
  v0 = (0 % v6) + (0 / v2.f0) + (0);
  v6 = (0 + v1[1]) + (0 - v2.f0) + (0);
  goto BB4;
BB12:
  v6 = (0 % v2.f0) - (0 / v3) - (0);
  v2.f0 = (0 - v5[1]) - (0 / v1[1]) - (0);
  goto BB7;
BB13:
  v3 = (0 - v1[1]) + (0 * v0) + (0);
  v6 = (0 * v0) + (0 - v5[1]) + (0);
  goto BB4;
BB14:
  v1[1] = (-1 - v2.f0) + (-1 - v6) + (-1);
  v3 = (-1 - v3) + (-1 / v1[1]) + (-1);
  {
    int __chk_args[] = {v0, v1[0], v1[1], v2.f0, v3, v4, v5[0], v5[1]};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 1182021441 --Xbitwuzla-threads 8 -o samples -n 78 xb1v6r