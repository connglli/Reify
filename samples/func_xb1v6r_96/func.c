extern int computeStatelessChecksum(int num_args, int args[]);
struct func_xb1v6r_96_S0 {
  int f0[3];
  int f1;
};
struct func_xb1v6r_96_S1 {
  struct func_xb1v6r_96_S0 f0;
  int f1;
};
int func_xb1v6r_96(int v0, int v1, struct func_xb1v6r_96_S1 v2, int v3, int v4, int v5) {
  int v6 = 4;
  int v7 = 0;
BB0:
  v1 = (-1 + v4) - (-1 / v6) - (-1);
  v0 = (-1 * v4) - (-1 + v6) - (-1);
  if ((-1 / v1) + (-1 % v0) + (-1 % v4) + (-1) < 0) goto BB15;
  goto BB18;
BB1:
  v5 = (0 * v5) + (1);
  v4 = (0 * v4) + (1);
  v4 = (0 * v4) + (1);
  v6 = (0 * v6) + (1);
  v2.f0.f0[2] = (0 % v5) - (0 / v4) - (0);
  v6 = (0 + v4) + (0 / v6) + (0);
  if ((0 - v2.f1) - (0 / v6) - (0 + v1) - (0) == 0) goto BB4;
  goto BB15;
BB2:
  v0 = (0 * v0) + (0);
  v6 = (0 * v6) + (1);
  v2.f0.f0[2] = (0 * v2.f0.f0[2]) + (-2147483648);
  v7 = (0 - v0) - (0 - v6) - (0);
  v3 = (0 / v7) - (0 + v2.f0.f1) - (0);
  goto BB1;
BB3:
  v6 = (0 % v2.f0.f0[2]) + (0 / v1) + (0);
  v5 = (0 % v6) + (0 + v1) + (0);
  goto BB1;
BB4:
  v2.f0.f1 = (0 + v0) - (0 - v6) - (0);
  v6 = (0 - v2.f0.f0[2]) + (0 * v6) + (0);
  if ((0 / v2.f1) + (0 * v6) + (0 - v7) + (0) == 0) goto BB1;
  goto BB6;
BB5:
  v4 = (0 + v5) + (0 + v6) + (0);
  v0 = (0 / v0) + (0 - v5) + (0);
  goto BB10;
BB6:
  v3 = (0 / v2.f1) + (0 / v7) + (0);
  v5 = (0 / v5) + (0 / v6) + (0);
  if ((0 % v3) + (0 / v5) + (0 - v0) + (0) < 0) goto BB7;
  goto BB8;
BB7:
  v1 = (0 / v2.f1) - (0 * v4) - (0);
  v5 = (0 + v7) - (0 - v2.f0.f0[2]) - (0);
  if ((0 * v1) + (0 - v5) + (0 - v4) + (0) == 0) goto BB6;
  goto BB16;
BB8:
  v1 = (0 % v0) + (0 / v5) + (0);
  v4 = (0 % v4) + (0 / v5) + (0);
  if ((0 % v1) + (0 - v4) + (0 + v0) + (0) == 0) goto BB10;
  goto BB15;
BB9:
  v0 = (0 * v2.f0.f0[2]) - (0 + v4) - (0);
  v4 = (0 / v3) - (0 + v6) - (0);
  if ((0 + v0) - (0 * v4) - (0 % v2.f0.f0[2]) - (0) < 0) goto BB6;
  goto BB16;
  do {
BB10:
    v4 = (0 - v2.f0.f1) - (0 % v5) - (0);
    v6 = (0 - v5) + (0 / v2.f1) + (0);
    goto BB13;
BB11:
    v7 = (0 * v1) - (0 * v2.f1) - (0);
    v1 = (0 + v7) - (0 % v2.f1) - (0);
    goto BB12;
BB12:
    v1 = (0 * v1) + (-1);
    v7 = (0 * v7) + (-1);
    v0 = (0 * v0) + (-2147483648);
    v7 = (0 * v7) + (-1);
    v6 = (0 / v1) + (0 / v7) + (0);
    v3 = (0 + v0) - (0 / v7) - (0);
    goto BB13;
BB13:
    v4 = (0 * v4) + (-1);
    v5 = (0 * v5) + (0);
    v4 = (0 * v4) + (-1);
    v2.f0.f0[2] = (0 * v2.f0.f0[2]) + (-2147483648);
    v0 = (0 / v4) + (0 + v5) + (0);
    v3 = (0 * v4) + (0 + v2.f0.f1) + (0);
  } while ((0 - v0) - (0 * v3) - (0 + v7) - (0) < 0);
  goto BB14;
BB14:
  v1 = (0 + v5) + (0 * v3) + (0);
  v5 = (0 % v2.f0.f0[2]) + (0 + v5) + (0);
  if ((0 + v1) - (0 - v5) - (0 / v2.f0.f0[2]) - (0) > 0) goto BB8;
  goto BB9;
BB15:
  v0 = (0 / v3) + (0 * v2.f0.f1) + (0);
  v3 = (0 % v0) + (0 + v3) + (0);
  if ((0 / v0) - (0 % v3) - (0 % v4) - (0) < 0) goto BB1;
  goto BB18;
BB16:
  v4 = (0 + v5) + (0 % v3) + (0);
  v1 = (0 - v2.f1) + (0 * v4) + (0);
  if ((0 + v4) + (0 + v1) + (0 / v5) + (0) < 0) goto BB17;
  goto BB18;
BB17:
  v0 = (0 + v6) + (0 + v4) + (0);
  v7 = (0 % v7) - (0 % v3) - (0);
  goto BB15;
BB18:
  v1 = (-1 % v2.f0.f0[2]) + (-1 - v1) + (-1);
  v3 = (-1 - v2.f0.f1) + (-1 + v6) + (-1);
  {
    int __chk_args[] = {v0, v1, v2.f0.f0[0], v2.f0.f0[1], v2.f0.f0[2], v2.f0.f1, v2.f1, v3, v4, v5};
    return computeStatelessChecksum(sizeof(__chk_args)/sizeof(int), __chk_args);
  }
}



// rysmith options: ./build/bin/rysmith --Xnum-bbls-per-fun 15 --Xnum-vars-per-fun 8 --Xnum-assigns-per-bbl 2 --Xnum-vars-per-assign 2 --Xnum-vars-in-cond 3 -m -S -A -U -v -s 1830219288 --Xbitwuzla-threads 8 -o samples -n 96 xb1v6r