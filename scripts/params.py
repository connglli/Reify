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
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
// We won't have more than 20 arguments, so use 20 here
#define __COUNT_ARGS__(...) \\
    __COUNT_ARGS_IMPL__(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)
#define __COUNT_ARGS_IMPL__(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
uint32_t g[256];
int32_t uint32_to_int32(uint32_t u){{
    return u%2147483647;
}}
void generate_crc32_table(uint32_t* crc32_table) {{
    const uint32_t poly = 0xEDB88320UL;
    for (uint32_t i = 0; i < 256; i++) {{
        uint32_t crc = i;
        for (int j = 8; j > 0; j--) {{
            if (crc & 1) {{
                crc = (crc >> 1) ^ poly;
            }} else {{
                crc >>= 1;
            }}
        }}
        crc32_table[i] = crc;
    }}
}}
uint32_t context_free_crc32_byte(uint32_t context, uint8_t b) {{
    return ((context >> 8) & 0x00FFFFFF) ^ g[(context ^ b) & 0xFF];
}}
uint32_t context_free_crc32_4bytes(uint32_t context, uint32_t val) {{
    context = context_free_crc32_byte(context, (val >> 0) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 8) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 16) & 0xFF);
    context = context_free_crc32_byte(context, (val >> 24) & 0xFF);
    return context;
}}
int computeStatelessChecksumImpl(int num_args, ...)
{{
    va_list args;
    va_start(args, num_args);
    uint32_t checksum = 0xFFFFFFFFUL;
    for (int i = 0; i < num_args; ++i) {{
        int arg = va_arg(args, int);
        checksum = context_free_crc32_4bytes(checksum, arg);
    }}
    checksum = uint32_to_int32(checksum ^ 0xFFFFFFFFUL);
    va_end(args);
    return checksum;
}}
// These are hacks as procedure_gen did not count the number of arguments ...
#define computeStatelessChecksum(...) \\
    computeStatelessChecksumImpl(__COUNT_ARGS__(__VA_ARGS__), __VA_ARGS__)
{func_code}
int main(int argc, int* argv[]) {{
    generate_crc32_table(g);
    printf("%d", {func_name}({func_args}));
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
