hp:
	@echo " Compile hp_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/hp_main.c ./src/record.c ./src/hp_file.c -lbf -o ./build/hp_main -O2

test:
	@echo " Compile test ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/test.c ./src/record.c ./src/hp_file.c -lbf -o ./build/test -O2;