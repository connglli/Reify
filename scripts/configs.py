#  MIT License
#
#  Copyright (c) 2025
#
#  Kavya Chopra (chopra.kavya04@gmail.com)
#  Cong Li (cong.li@inf.ethz.ch)
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
#  SOFTWARE.

import json
from pathlib import Path

PROG_TPL = """
{chksum_code}

#include <stdio.h>

{func_code}

int main(int argc, char* argv[]) {{
    generate_crc32_table();
    printf("%d", {func_name}({func_args}));
    return 0;
}}
"""

ROOT_DIR = Path(__file__).parent.parent.absolute()

DEFAULT_OUTPUT_DIR = ROOT_DIR / "generated"

CHKSUM_FUNC = "computeStatelessChecksum"
CHKSUM_FILE = ROOT_DIR / "res" / "cchksum.txt"
CHKSUM_CODE = CHKSUM_FILE.read_text()

# Must match C++ constants in include/global.hpp
FUNCTION_PREFIX = "func"
PROGRAM_PREFIX = "prog"
FILENAME_FUNCTION_C = "func.c"
FILENAME_MAIN_C = "main.c"
FILENAME_MAPPING_JSONL = "inout.jsonl"
FILENAME_SEXPRESSION = "func.sexp"
FILENAME_LOGGING = "func.log"
FILENAME_CHECKSUM_C = "chksum.c"
FILENAME_PROTOTYPES_H = "proto.h"


class FunArts:
  def __init__(self, fuuid, fsano, gen_dir=DEFAULT_OUTPUT_DIR):
    self.fuuid = str(fuuid).replace("-", "_")
    self.fsano = fsano
    self.gen_dir = Path(gen_dir)

  @staticmethod
  def is_test_dir(test_dir):
    test_dir = Path(test_dir)
    return (
      test_dir.exists()
      and test_dir.is_dir()
      and test_dir.name.startswith(FUNCTION_PREFIX)
      and (test_dir / FILENAME_FUNCTION_C).exists()
      and (test_dir / FILENAME_MAPPING_JSONL).exists()
    )

  @staticmethod
  def from_test_dir(test_dir, gen_dir=DEFAULT_OUTPUT_DIR):
    assert FunArts.is_test_dir(test_dir), f"Not a valid function test dir: {test_dir}"
    test_dir = Path(test_dir)
    parts = test_dir.name.split("_")
    if len(parts) < 3:
      raise ValueError(f"Invalid test dir name: {test_dir.name}")
    fuuid = "_".join(parts[1:-1]).replace("_", "-")
    fsano = parts[-1]
    return FunArts(fuuid, fsano, gen_dir)

  def get_test_dir(self):
    return self.gen_dir / f"{FUNCTION_PREFIX}_{self.fuuid}_{self.fsano}"

  def get_func_name(self):
    return f"func_{self.fuuid}_{self.fsano}"

  def get_func_file(self):
    return self.get_test_dir() / FILENAME_FUNCTION_C

  def get_map_file(self):
    return self.get_test_dir() / FILENAME_MAPPING_JSONL

  def get_main_file(self):
    return self.get_test_dir() / FILENAME_MAIN_C

  def get_log_file(self):
    return self.get_test_dir() / FILENAME_LOGGING

  def get_sexp_file(self):
    return self.get_test_dir() / FILENAME_SEXPRESSION

  @staticmethod
  def get_simple_program(func_name, func_code, func_args):
    return PROG_TPL.format(
      chksum_code=CHKSUM_CODE,
      func_code=func_code,
      func_name=func_name,
      func_args=", ".join(x.unflat_c_str() for x in func_args),
    )


class ProgArts:
  def __init__(self, puuid, psano, gen_dir=DEFAULT_OUTPUT_DIR):
    self.puuid = str(puuid).replace("-", "_")
    self.psano = psano
    self.gen_dir = Path(gen_dir)

  @staticmethod
  def is_test_dir(test_dir):
    test_dir = Path(test_dir)
    return (
      test_dir.exists()
      and test_dir.is_dir()
      and test_dir.name.startswith(PROGRAM_PREFIX)
      and (test_dir / FILENAME_MAIN_C).exists()
      and (test_dir / FILENAME_CHECKSUM_C).exists()
      and (test_dir / FILENAME_PROTOTYPES_H).exists()
    )

  @staticmethod
  def from_test_dir(test_dir, gen_dir=DEFAULT_OUTPUT_DIR):
    assert ProgArts.is_test_dir(test_dir), f"Not a valid program test dir: {test_dir}"
    test_dir = Path(test_dir)
    parts = test_dir.name.split("_")
    if len(parts) < 3:
      raise ValueError(f"Invalid test dir name: {test_dir.name}")
    puuid = "_".join(parts[1:-1]).replace("_", "-")
    psano = parts[-1]
    return ProgArts(puuid, psano, gen_dir)

  def get_test_dir(self):
    return self.gen_dir / f"{PROGRAM_PREFIX}_{self.puuid}_{self.psano}"

  def get_main_file(self):
    return self.get_test_dir() / FILENAME_MAIN_C

  def get_chksum_file(self):
    return self.get_test_dir() / FILENAME_CHECKSUM_C

  def get_proto_file(self):
    return self.get_test_dir() / FILENAME_PROTOTYPES_H

  def get_func_file(self, fname):
    return self.get_test_dir() / (fname + ".c")


class ArgVal:
  def __init__(self, shape, elems):
    self.shape = shape
    self.elems = elems

  def is_scalar(self):
    return len(self.shape) == 0

  def is_vector(self):
    return len(self.shape) != 0

  def get_c_type(self):
    return f"int{''.join('[' + str(d) + ']' for d in self.shape) if self.shape else ''}"

  def get_shaped_value(self):
    if self.is_scalar():
      return self.elems[0]

    def reshape(flat, shape):
      if not shape:
        return flat[0]
      if len(shape) == 1:
        return flat[: shape[0]]
      stride = 1
      for d in shape[1:]:
        stride *= d
      out = []
      for i in range(shape[0]):
        start = i * stride
        end = start + stride
        out.append(reshape(flat[start:end], shape[1:]))
      return out

    return reshape(self.elems, self.shape)

  def num_els(self):
    return len(self.elems)

  def flat_c_str(self):
    # Flatten all array elements
    return ",".join(str(e) for e in self.elems)

  def unflat_c_str(self):
    # Keep the original shape
    if not self.shape:
      return str(self.elems[0])
    else:
      oss = []
      oss.append(f"(int(*){''.join('[' + str(d) + ']' for d in self.shape[1:])})")
      oss.append(f"((int[{len(self.elems)}])")
      oss.append("{" + ", ".join(str(e) for e in self.elems) + "})")
      return "".join(oss)

  @staticmethod
  def from_json(obj):
    shape = obj["shape"]
    elems = obj["elems"]
    # Validate size
    expected = 1
    for d in shape:
      expected *= d
    if expected != len(elems):
      raise ValueError(
        f"The elements length {len(elems)} doesn't match shape (expecting {expected} elements)"
      )
    return ArgVal(shape, elems)


def parse_mapping(map_path):
  maps = []
  for line in Path(map_path).read_text().splitlines():
    obj = json.loads(line)
    maps.append(
      (
        [ArgVal.from_json(a) for a in obj["ini"]],
        [ArgVal.from_json(a) for a in obj["fin"]],
        obj["chk"],
      )
    )
  return maps
