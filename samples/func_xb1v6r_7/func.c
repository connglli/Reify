extern int computeStatelessChecksum(int num_args, int args[]);
struct func_xb1v6r_7_S0 {
  int f0;
  int f1[2];
  int f2;
};
int func_xb1v6r_7(int v0, struct func_xb1v6r_7_S0 v1, int v2, int v3, int v4, int v5[1]) {
  int v6 = -1;
  int v7 = -1;
BB0:
  v3 = (-1 / v3) + (-1 + v1.f2) + (-1);
  v4 = (-1 % v6) + (-1 % v7) + (-1);
  goto BB11;
BB1:
  v5[0] = (0 % v4) + (0 * v0) + (0);
  v3 = (0 / v4) + (0 * v3) + (0);
  if ((0 + v5[0]) - (0 % v3) - (0 / v7) - (0) < 0) goto BB2;
  goto BB8;
BB2:
  v1.f2 = (0 / v2) + (0 / v3) + (0);
  v3 = (0 % v0) + (0 % v7) + (0);
  if ((0 / v1.f1[1]) + (0 % v3) + (0 - v4) + (0) == 0) goto BB1;
  goto BB8;
BB3:
  v1.f0 = (0 * v1.f0) + (-2147483648);
  v5[0] = (0 * v5[0]) + (-1);
  v6 = (0 * v6) + (-2147483648);
  v0 = (0 * v0) + (-1);
  v4 = (0 * v1.f0) - (0 % v5[0]) - (0);
  v7 = (0 - v6) + (0 % v0) + (0);
  if ((0 + v4) + (0 - v7) + (0 % v3) + (0) > 0) goto BB2;
  goto BB6;
BB4:
  v3 = (-1 + v5[0]) - (-1 % v2) - (-1);
  v2 = (-1 / v0) - (-1 / v7) - (-1);
  if ((-1 + v3) + (-1 + v2) + (-1 - v6) + (-1) > 0) goto BB2;
  goto BB6;
BB5:
  v2 = (0 % v1.f2) - (0 * v6) - (0);
  v4 = (0 - v4) + (0 - v7) + (0);
  if ((0 % v2) + (0 + v4) + (0 / v6) + (0) > 0) goto BB3;
  goto BB9;
BB6:
  v3 = (-1 + v1.f2) + (-1 % v3) + (-1);
  v6 = (-1 * v3) + (-1 + v6) + (-2);
  if ((-1 + v3) + (-1 * v6) + (-1 * v1.f2) + (2) == 0) goto BB7;
  goto BB9;
BB7:
  v2 = (-1 % v3) - (0 - v4) - (-1);
  v0 = (-1 / v6) + (-1 + v5[0]) + (-1);
  goto BB18;
BB8:
  v4 = (0 / v5[0]) - (0 + v0) - (0);
  v0 = (0 * v5[0]) + (0 % v7) + (0);
  goto BB5;
BB9:
  v2 = (0 % v0) + (0 % v2) + (0);
  v3 = (0 / v7) - (0 / v2) - (0);
  if ((0 + v2) + (0 + v3) + (0 + v1.f2) + (0) > 0) goto BB2;
  goto BB12;
BB10:
  v4 = (0 % v7) - (0 + v3) - (0);
  v1.f2 = (0 - v3) + (0 * v0) + (0);
  if ((0 / v4) + (0 % v1.f0) + (0 + v0) + (0) > 0) goto BB4;
  goto BB17;
BB11:
  v0 = (-1 / v1.f0) + (-1 - v3) + (-1);
  v1.f2 = (-1 / v2) - (2 - v5[0]) - (-1);
  if ((-1 - v0) - (-1 * v1.f0) - (-1 + v7) - (-1) == 0) goto BB4;
  goto BB10;
  do {
BB12:
    v0 = (0 / v2) + (0 - v5[0]) + (0);
    v3 = (0 % v2) + (0 / v1.f2) + (0);
    goto BB14;
BB13:
    v4 = (0 / v5[0]) - (0 - v1.f1[1]) - (0);
    v1.f1[1] = (0 % v2) - (0 / v7) - (0);
    goto BB15;
BB14:
    v3 = (0 + v4) - (0 - v0) - (0);
    v4 = (0 * v3) - (0 * v0) - (0);
    goto BB13;
BB15:
    v2 = (0 * v0) - (0 % v4) - (0);
    v6 = (0 - v4) + (0 - v2) + (0);
  } while ((0 * v2) + (0 - v6) + (0 + v0) + (0) > 0);
  goto BB16;
BB16:
  v2 = (0 - v6) - (0 - v3) - (0);
  v0 = (0 + v0) + (0 % v2) + (0);
  if ((0 + v2) + (0 + v0) + (0 * v3) + (0) == 0) goto BB9;
  goto BB17;
BB17:
  v5[0] = (0 - v0) - (0 / v3) - (0);
  v1.f0 = (0 / v5[0]) + (0 * v0) + (0);
  goto BB9;
BB18:
  v4 = (-1 / v4) + (-1 / v5[0]) + (-1);
  v3 = (-1 + v4) - (-1 % v0) - (2);
  {
    int __chk_args[] = {v0, v1.f0, v1.f1[0], v1.f1[1], v1.f2, v2, v3, v4, v5[0]};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 2046968324 --Xbitwuzla-threads 8 -o samples -n 7 xb1v6r