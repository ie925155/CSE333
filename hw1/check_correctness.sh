./test_suite
read -p "Executed ./test_suite\n"
valgrind ./example_program_ht
read -p "Executed valgrind ./example_program_ht"
valgrind ./example_program_ll
read -p "Executed valgrind ./example_program_ll"
valgrind ./test_suite
read -p "Executed valgrind./test_suite"
../clint.py HashTable.c
read -p "Executed ../clint.py HashTable.c"
../clint.py LinkedList.c
read -p "Executed ../clint.py LinkedList.c"
