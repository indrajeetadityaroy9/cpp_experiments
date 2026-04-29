with open("include/duan_sssp.hpp", "r") as f:
    content = f.read()

import re
# Find something like:
# std::cout << "=== Duan SSSP Statistics ===
# ";
# And replace with std::cout << "=== Duan SSSP Statistics ===\\n";
content = re.sub(r'<< "([^"]*)\n";', r'<< "\1\\n";', content)

with open("include/duan_sssp.hpp", "w") as f:
    f.write(content)
