int a, b;
int f(int g) {
  a = 0;
  for (; a < 32; a++)
    if (g >> a & 1)
      return a;
  return 0;
}
int main() {
  if (f(b-2) != 1)
    __builtin_abort();
  return 0;
}