int printf(const char *, ...);
int a;
int main() {
  int b = 0;
  if (a >= 0)
    b += 3;
  printf("%d\n", b);
  return 0;
}