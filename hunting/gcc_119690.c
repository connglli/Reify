int printf(const char *, ...);
int a, b = -2147483647, c;

int main() {
d:
  c = a + b + 2147483647;
  if (-c >= 0)
    goto e;
  goto f;
e:
  a = c + 1;
  goto d;
f:
  printf("%d\n", a);
  return 0;
}
