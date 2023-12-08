The directory (hashtable) is implemented as double linked list. Every time
the file is opened, the hashtable is read from the a series of blocks and
creates the linked list which is stored on RAM while the file is open. When
the file closes the directory is written in a series of blocks on the file.

The utility functions are located in hash_file.c. To run the make script
type on terminal `make ht;`. Then you should see an output file `ht.o`
in `build` directory. To run the main program simply run `./build/ht.o`.

