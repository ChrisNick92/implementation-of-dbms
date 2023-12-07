ht:
	@echo " Compile ht_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/ht_main.c ./src/hash_file.c -lbf -o ./build/runner -O2

bf:
	@echo " Compile bf_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bf_main.c -lbf -o ./build/runner -O2

test:
	@echo " Compile test ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/test.c ./src/hash_file.c -lbf -o ./build/test.o -O2

test_2:
	@echo " Compile test ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/test_2.c ./src/hash_file.c -lbf -o ./build/test_2.o -O2