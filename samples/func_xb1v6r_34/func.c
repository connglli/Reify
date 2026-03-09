extern int computeStatelessChecksum(int num_args, int args[]);
struct func_xb1v6r_34_S0 {
  int f0;
  int f1;
  int f2;
};
int func_xb1v6r_34(struct func_xb1v6r_34_S0 v0, int v1, int v2[3][1], int v3, int v4, int v5[2][1]) {
  int v6 = 0;
  int v7 = -1;
BB0:
  v1 = (-1 % v7) - (-1 * v2[2][0]) - (0);
  v6 = (-1 % v0.f0) + (-1 * v4) + (-2);
  goto BB5;
BB1:
  v2[2][0] = (0 % v2[2][0]) - (0 - v6) - (0);
  v6 = (0 + v7) + (0 / v4) + (0);
  goto BB10;
BB2:
  v6 = (0 % v4) - (0 / v5[1][0]) - (0);
  v5[1][0] = (0 - v5[1][0]) - (0 - v4) - (0);
  if ((0 * v6) + (0 % v5[1][0]) + (0 + v3) + (0) == 0) goto BB5;
  goto BB12;
BB3:
  v1 = (0 * v1) + (0);
  v6 = (0 * v6) + (-2147483648);
  v7 = (0 * v7) + (1);
  v6 = (0 * v6) + (-2147483648);
  v3 = (0 + v1) + (0 + v6) + (0);
  v5[1][0] = (0 / v7) + (0 + v6) + (0);
  if ((0 + v3) - (0 % v5[1][0]) - (0 / v7) - (0) == 0) goto BB6;
  goto BB14;
BB4:
  v7 = (0 + v0.f0) + (0 - v2[2][0]) + (0);
  v0.f1 = (0 % v5[1][0]) - (0 / v0.f1) - (0);
  if ((0 % v7) + (0 - v0.f0) + (0 * v5[1][0]) + (0) > 0) goto BB2;
  goto BB10;
BB5:
  v5[1][0] = (-1 + v4) - (-1 / v6) - (-1);
  v6 = (-1 / v0.f0) - (-1 + v6) - (4);
  if ((-1 / v5[1][0]) - (-1 % v6) - (-1 % v0.f0) - (-1) == 0) goto BB6;
  goto BB14;
BB6:
  v2[2][0] = (0 / v1) - (0 % v3) - (0);
  v1 = (0 + v0.f1) - (0 / v5[1][0]) - (0);
  if ((0 / v2[2][0]) - (0 % v1) - (0 % v7) - (0) == 0) goto BB2;
  goto BB8;
BB7:
  v7 = (0 * v7) + (1073741824);
  v6 = (0 * v6) + (-1073741825);
  v5[1][0] = (0 * v5[1][0]) + (0);
  v2[2][0] = (0 * v2[2][0]) + (-1);
  v1 = (0 + v7) - (0 - v6) - (0);
  v5[1][0] = (0 + v5[1][0]) - (0 - v2[2][0]) - (0);
  if ((0 - v1) - (0 * v5[1][0]) - (0 - v7) - (0) == 0) goto BB8;
  goto BB9;
BB8:
  v4 = (0 % v2[2][0]) - (0 * v4) - (0);
  v5[1][0] = (0 * v4) + (0 / v3) + (0);
  if ((0 * v4) + (0 * v5[1][0]) + (0 + v2[2][0]) + (0) < 0) goto BB2;
  goto BB4;
BB9:
  v0.f2 = (0 / v4) - (0 / v6) - (0);
  v5[1][0] = (0 * v3) - (0 % v5[1][0]) - (0);
  if ((0 % v0.f1) - (0 * v5[1][0]) - (0 / v6) - (0) < 0) goto BB7;
  goto BB10;
BB10:
  v7 = (0 + v0.f0) + (0 / v6) + (0);
  v6 = (0 - v4) - (0 * v3) - (0);
  if ((0 / v7) - (0 / v6) - (0 + v5[1][0]) - (0) < 0) goto BB2;
  goto BB11;
BB11:
  v4 = (0 / v3) - (0 * v5[1][0]) - (0);
  v3 = (0 % v0.f2) - (0 / v7) - (0);
  if ((0 - v4) + (0 * v3) + (0 * v1) + (0) == 0) goto BB2;
  goto BB6;
BB12:
  v2[2][0] = (0 * v2[2][0]) + (0);
  v1 = (0 * v1) + (1);
  v1 = (0 * v1) + (1);
  v5[1][0] = (0 * v5[1][0]) + (1);
  v0.f2 = (0 + v2[2][0]) + (0 + v1) + (0);
  v6 = (0 % v1) + (0 + v5[1][0]) + (0);
  if ((0 + v0.f0) + (0 % v6) + (0 - v3) + (0) < 0) goto BB9;
  goto BB13;
BB13:
  v1 = (0 * v1) + (1);
  v0.f1 = (0 * v0.f1) + (1);
  v0.f2 = (0 * v0.f2) + (-2147483648);
  v1 = (0 * v1) + (1);
  v5[1][0] = (0 % v1) - (0 / v0.f1) - (0);
  v6 = (0 - v0.f2) + (0 / v1) + (0);
  goto BB12;
BB14:
  v0.f0 = (-1 % v2[2][0]) + (-1 - v1) + (-1);
  v2[2][0] = (-1 % v7) + (-1 / v2[2][0]) + (-1);
  {
    int __chk_args[] = {v0.f0, v0.f1, v0.f2, v1, v2[0][0], v2[1][0], v2[2][0], v3, v4, v5[0][0], v5[1][0]};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 2119900799 --Xbitwuzla-threads 8 -o samples -n 34 xb1v6r