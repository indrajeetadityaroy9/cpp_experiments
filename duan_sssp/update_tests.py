import os
import glob
import re

test_files = glob.glob("tests/test_*.cpp")

for test_file in test_files:
    with open(test_file, "r") as f:
        content = f.read()

    # Replace includes
    content = re.sub(r'#include "\.\./include/algorithms/.*\.hpp"', '#include "../include/duan_sssp.hpp"', content)
    # Ensure only one include for duan_sssp.hpp if there are multiple now
    # Actually just a simple string replace is fine, but deduplicate includes
    lines = content.split('\n')
    new_lines = []
    seen_duan = False
    for line in lines:
        if '#include "../include/duan_sssp.hpp"' in line:
            if seen_duan:
                continue
            seen_duan = True
        new_lines.append(line)
    content = '\n'.join(new_lines)

    # Replace class static method calls with free functions
    content = content.replace("BaseCase::Execute", "execute_base_case")
    content = content.replace("FindPivots::Execute", "execute_find_pivots")
    content = content.replace("BMSSP::Execute", "execute_bmssp")
    content = content.replace("DuanSSSP::ComputeSSSP", "compute_sssp")
    content = content.replace("Dijkstra::ComputeSSSP", "compute_dijkstra_sssp")

    with open(test_file, "w") as f:
        f.write(content)

print("Tests updated.")
