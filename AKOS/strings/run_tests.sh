#!/bin/bash
# run as "$ . run_tests.sh"

gcc main.c -o test/strings && cd test && ./strings < test-1 > res1.txt && diff res1.txt test-1-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-2 > res2.txt && diff res2.txt test-2-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-eof > res-eof.txt && diff res-eof.txt test-eof-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-err > res-err.txt 2> res-err.txt && diff res-err.txt test-err-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-koi > res-koi.txt && diff res-koi.txt test-koi-res.txt     && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-quotes > res-quotes.txt && diff res-quotes.txt test-quotes-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-small > res-small.txt && diff res-small.txt test-small-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-sort > res-sort.txt && diff res-sort.txt test-sort-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-tricky > res-tricky.txt && diff res-tricky.txt test-tricky-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < test-spaces > res-spaces.txt && diff res-spaces.txt test-spaces-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < many-strings > myres_many-strings.txt && diff myres_many-strings.txt many-strings-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < long-string > myres_long-string.txt && diff myres_long-string.txt long-string-res.txt && cd ..
gcc main.c -o test/strings && cd test && ./strings < long-string-new > myres_long-string-new.txt && diff myres_long-string-new.txt long-string-new-res.txt && cd ..
