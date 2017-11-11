LIBRARY_PATH=/usr/local/lib gcc -DSQLITE_CORE -O3 -std=gnu11 -I/usr/local/include -I/usr/local/include/evhtp *.c -pthread  -lfswatch -levent -lsqlite3 -levhtp -lconfuse -levent -o daap
