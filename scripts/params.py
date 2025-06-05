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

from pathlib import Path

PROG_TPL = """
{chksum_code}

#include <stdio.h>

{func_code}

int main(int argc, int* argv[]) {{
    printf("%d", {func_name}({func_args}));
    return 0;
}}
"""

PROCEDURES_DIR = (Path(__file__).parent.parent / 'procedures').absolute()
MAPPINGS_DIR = (Path(__file__).parent.parent / 'mappings').absolute()

CHKSUM_FILE = (Path(__file__).parent.parent / 'res' / 'cchksum.txt').absolute()
CHKSUM_CODE = CHKSUM_FILE.read_text()


def get_simple_program(func_name, func_code, func_args):
    return PROG_TPL.format(
        chksum_code=CHKSUM_CODE,
        func_code=func_code,
        func_name=func_name,
        func_args=", ".join(str(x) for x in func_args)
    )


def get_func_map_files(fuuid, fsano, procedures_dir=PROCEDURES_DIR, mappings_dir=MAPPINGS_DIR):
    fuuid_ = fuuid.replace('-', '_')
    return (
        procedures_dir / f"function_{fuuid_}_{fsano}.c",
        mappings_dir / f"function_{fuuid_}_{fsano}_mapping"
    )


def get_map_file_for_func_file(func_path, mappings_dir=MAPPINGS_DIR):
    return mappings_dir / f"{Path(func_path).stem}_mapping"


def parse_mapping(map_path):
    return [
        (pair[0].rstrip(',').split(','), pair[1].rstrip(',').split(','))
        for pair in [
            tuple(line.split(' : ', maxsplit=1))
            for line in Path(map_path).read_text().splitlines()
        ]
    ]
