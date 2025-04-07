import re

def instrument_basic_blocks(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()

    output_lines = []
    bb_pattern = re.compile(r'^\s*(BB(\d+))\s*:\s*$')

    for line in lines:
        output_lines.append(line)
        match = bb_pattern.match(line)
        if match:
            label = match.group(1)
            block_num = match.group(2)
            indent = re.match(r'^(\s*)', line).group(1)
            print_line = f'{indent}printf("BB{block_num}\\n");\n'
            output_lines.append(print_line)
            output_lines.append(f'{indent}printf("%d,%d,%d,%d,%d,%d,%d,%d", var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7);\n')

    with open(filename, 'w') as f:
        f.writelines(output_lines)

# Example usage:
instrument_basic_blocks('checkUB.c')
