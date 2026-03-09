extern int computeStatelessChecksum(int num_args, int args[]);
struct func_xb1v6r_55_S0 {
  int f0;
  int f1;
};
int func_xb1v6r_55(int v0, struct func_xb1v6r_55_S0 v1, struct func_xb1v6r_55_S0 v2, int v3, int v4[1][3], int v5) {
  int v6 = -1;
  int v7[3][2] = {-1, -1, -1, -1, -1, -1};
BB0:
  v4[0][2] = (-3 - v7[2][1]) - (-1 - v2.f1) - (-1);
  v5 = (-3 - v1.f1) - (-1 % v6) - (-1);
  goto BB5;
BB1:
  v7[2][1] = (0 / v6) - (0 / v1.f1) - (0);
  v1.f1 = (0 % v1.f0) + (0 - v2.f1) + (0);
  goto BB11;
BB2:
  v6 = (0 / v4[0][2]) - (0 / v3) - (0);
  v7[2][1] = (0 / v0) - (0 + v2.f0) - (0);
  if ((0 % v6) + (0 * v7[2][1]) + (0 / v0) + (0) > 0) goto BB10;
  goto BB13;
BB3:
  v7[2][1] = (0 % v5) - (0 / v7[2][1]) - (0);
  v3 = (0 * v3) + (0 + v4[0][2]) + (0);
  if ((0 / v7[2][1]) + (0 + v3) + (0 + v0) + (0) > 0) goto BB2;
  goto BB9;
BB4:
  v0 = (0 * v0) + (-2147483648);
  v7[2][1] = (0 * v7[2][1]) + (-1);
  v5 = (0 * v5) + (-2147483648);
  v1.f1 = (0 - v0) - (0 % v7[2][1]) - (0);
  v3 = (0 * v1.f0) - (0 * v5) - (0);
  if ((0 + v1.f0) + (0 * v3) + (0 % v2.f1) + (0) < 0) goto BB3;
  goto BB12;
BB5:
  v0 = (-1 * v1.f0) + (-1 * v3) + (-1);
  v6 = (-1 + v3) - (-1 / v0) - (-2);
  if ((-1 + v0) - (-1 * v6) - (-1 + v4[0][2]) - (-1) > 0) goto BB7;
  goto BB10;
BB6:
  v4[0][2] = (0 + v0) + (0 * v4[0][2]) + (0);
  v3 = (0 * v7[2][1]) - (0 + v4[0][2]) - (0);
  if ((0 - v4[0][2]) - (0 - v3) - (0 / v5) - (0) > 0) goto BB5;
  goto BB10;
BB7:
  v3 = (-1 % v6) - (3 - v0) - (-1);
  v2.f0 = (-1 / v6) + (-1 + v4[0][2]) + (-1);
  if ((-1 % v3) + (-1 / v2.f0) + (-1 % v7[2][1]) + (2147483647) < 0) goto BB6;
  goto BB14;
BB8:
  v5 = (0 % v5) + (0 / v6) + (0);
  v2.f1 = (0 * v2.f0) - (0 - v5) - (0);
  goto BB12;
BB9:
  v0 = (0 * v0) + (1);
  v1.f0 = (0 * v1.f0) + (1);
  v1.f0 = (0 * v1.f0) + (1);
  v4[0][2] = (0 * v4[0][2]) + (-2147483648);
  v2.f1 = (0 / v0) + (0 % v1.f0) + (0);
  v4[0][2] = (0 / v1.f0) - (0 * v4[0][2]) - (0);
  if ((0 / v2.f0) - (0 - v4[0][2]) - (0 + v5) - (0) > 0) goto BB7;
  goto BB11;
BB10:
  v6 = (0 * v6) + (-1);
  v4[0][2] = (0 * v4[0][2]) + (-2147483648);
  v7[2][1] = (0 * v7[2][1]) + (-2147483648);
  v2.f1 = (0 - v6) - (0 * v4[0][2]) - (0);
  v1.f1 = (0 + v2.f0) - (0 * v7[2][1]) - (0);
  if ((0 / v2.f1) - (0 / v1.f1) - (0 + v6) - (0) > 0) goto BB9;
  goto BB11;
BB11:
  v1.f1 = (0 - v4[0][2]) - (0 % v2.f1) - (0);
  v6 = (0 - v7[2][1]) - (0 % v5) - (0);
  if ((0 % v1.f0) + (0 - v6) + (0 / v4[0][2]) + (0) < 0) goto BB10;
  goto BB12;
BB12:
  v7[2][1] = (0 % v1.f1) + (0 + v7[2][1]) + (0);
  v5 = (0 + v0) - (0 + v1.f1) - (0);
  if ((0 / v7[2][1]) - (0 - v5) - (0 * v2.f1) - (0) == 0) goto BB2;
  goto BB7;
BB13:
  v0 = (0 / v4[0][2]) + (0 + v7[2][1]) + (0);
  v2.f1 = (0 + v5) + (0 * v0) + (0);
  goto BB11;
BB14:
  v5 = (-1 - v2.f1) + (-1 - v4[0][2]) + (-1);
  v6 = (-1 % v3) - (-1 / v2.f1) - (0);
  {
    int __chk_args[] = {v0, v1.f0, v1.f1, v2.f0, v2.f1, v3, v4[0][0], v4[0][1], v4[0][2], v5};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 1911213317 --Xbitwuzla-threads 8 -o samples -n 55 xb1v6r