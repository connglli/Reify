from pathlib import Path

PROG_TPL = """
#include <stdarg.h>
unsigned int g[256];
int ac, i, ad;
void ax(unsigned int *a){{
  unsigned int c = 3988292384;
  for (unsigned int d = 0; d < 256; d++){{
    unsigned int aj = d;
    for (int j = 8; j; j--)
      if (aj & 1)
        aj = aj >> 1 ^ c;
      else
        aj >>= 1;
    a[d] = aj;
  }}
}}
unsigned int ak(unsigned int as, unsigned char b){{ return as >> 8 ^ g[(as ^ b) & 255]; }}
unsigned int at(unsigned int as, unsigned int e){{
  as = ak(as, e);
  as = ak(as, e >> 8);
  as = ak(as, e >> 16);
  as = ak(as, e >> 24);
  return as;
}}
int computeStatelessChecksum(int av, ...){{
  va_list args;
  va_start(args, av);
  unsigned int aa = 4294967295;
  for (int d = 0; d < av; ++d) {{
    int ab = va_arg(args, int);
    aa = at(aa, ab);
  }}
  aa = aa ^ 4294967295;
  return aa;
}}
{func_code}
int main(int argc, int* argv[]) {{
    ax(g);
    {func_name}({func_args});
    return 0;
}}
"""

PROCEDURES_DIR = (Path(__file__).parent.parent / 'procedures').absolute()
MAPPINGS_DIR = (Path(__file__).parent.parent / 'mappings').absolute()


def get_simple_program(func_name, func_code, func_args):
  return PROG_TPL.format(
    func_code=func_code,
    func_name=func_name,
    func_args=", ".join(str(x) for x in func_args)
  )


def get_func_map_files(fuuid, findex):
  fuuid_ = fuuid.replace('-', '_')
  return (
    PROCEDURES_DIR / f"function_{fuuid_}_{findex}.c",
    MAPPINGS_DIR / f"function_{fuuid_}_{findex}_mapping"
  )


def get_map_file_for_func_file(func_path):
  return MAPPINGS_DIR / f"{Path(func_path).stem}_mapping"


def parse_mapping(map_path):
  return [
    (pair[0].rstrip(',').split(','), pair[1].rstrip(',').split(','))
    for pair in [
      tuple(line.split(' : ', maxsplit=1))
      for line in Path(map_path).read_text().splitlines()
    ]
  ]
