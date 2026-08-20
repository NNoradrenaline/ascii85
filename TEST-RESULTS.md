# Verification results

Upstream: https://github.com/roukaour/ascii85
Upstream HEAD inspected: 069f585cbe7642a3f0f08de71ced639ce48375ba

## Normal build and tests
```text
cc -std=c11 -Wall -Wextra -Wconversion -Wshadow -pedantic -O2  -o ascii85 ascii85.c
python3 tests.py ./ascii85
ok - test_ascii85_vectors
ok - test_ascii85_abbreviations
ok - test_strict_rejections
ok - test_a85x_vectors
ok - test_a85x_corruption
ok - test_a85x_canonical_rules
ok - test_fuzz

7 test groups passed
```

## ASan + UBSan
```text
cc -std=c11 -Wall -Wextra -pedantic -O1 -g \
	-fsanitize=address,undefined -fno-omit-frame-pointer \
	-o ascii85-san ascii85.c
python3 tests.py ./ascii85-san
ok - test_ascii85_vectors
ok - test_ascii85_abbreviations
ok - test_strict_rejections
ok - test_a85x_vectors
ok - test_a85x_corruption
ok - test_a85x_canonical_rules
ok - test_fuzz

7 test groups passed
```
