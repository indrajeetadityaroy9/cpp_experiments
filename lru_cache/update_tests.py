import os
import re

files = ["lru_demo.cpp", "lru_test.cpp", "lru_bench.cpp", "Makefile"]

for file in files:
    with open(file, 'r') as f:
        content = f.read()
    
    # Replace header inclusion
    content = content.replace('"lru.h"', '"lru_cache.h"')
    
    # Replace cache.set(...).has_value() with cache.set(...)
    content = re.sub(r'cache\.set\(([^,]+),\s*(.+?)\)\.has_value\(\)', r'cache.set(\1, \2)', content)
    
    if file == "lru_test.cpp" or file == "lru_demo.cpp":
        # Replace get_optional and get_ref with get
        content = content.replace('get_optional', 'get')
        content = content.replace('get_ref', 'get')
        
        # result.has_value() -> result != nullptr
        content = content.replace('.has_value()', ' != nullptr')
        
        # result.value().get() -> *result
        content = content.replace('result.value().get()', '*result')
        
        # result.value() -> *result
        content = content.replace('result.value()', '*result')
        
        # result->get() -> *result
        content = content.replace('result->get()', '*result')
        
        # !result -> result == nullptr (only if checking optional/pointer boolean)
        # Actually in catch2 REQUIRE(result) works for pointers.
        
    if file == "Makefile":
        content = content.replace('lru.h lru.tpp', 'lru_cache.h')
        content = re.sub(r'BASELINE_HEADERS\s*=\s*lru_baseline\.h\n', '', content)
        content = re.sub(r'COMPARATIVE_TARGET\s*=\s*comparative_benchmark\n', '', content)
        content = re.sub(r'COMPARATIVE_SRCS\s*=\s*comparative_benchmark\.cpp\n', '', content)
        content = content.replace('$(COMPARATIVE_TARGET)', '')
        content = re.sub(r'# Comparative benchmark.*?\ncompare:.*?\n\n', '', content, flags=re.DOTALL)
        content = re.sub(r'compare:.*?\n', '', content)
        
    with open(file, 'w') as f:
        f.write(content)

# Delete unnecessary files
try:
    os.remove('lru.h')
    os.remove('lru.tpp')
    os.remove('lru_baseline.h')
    os.remove('comparative_benchmark.cpp')
    os.remove('refactor.py')
except FileNotFoundError:
    pass
