LIBRARY_PATH=/usr/local/lib gcc  -DSQLITE_CORE -DSQLITE_THREADSAFE=2 -O3 -std=gnu99 -I/usr/local/include -I/usr/local/include/evhtp *.c -pthread  -lfswatch -levent -lsqlite3 -levhtp -lconfuse -levent -o daap-gnu
LIBRARY_PATH=/usr/local/lib gcc  -DSQLITE_CORE -DSQLITE_THREADSAFE=2 -O3 -std=gnu99 -I/usr/local/include -I/usr/local/include/evhtp *.c -pthread  -ltcmalloc -lfswatch -levent -lsqlite3 -levhtp -lconfuse -levent -o daap-tcmalloc
LIBRARY_PATH=/usr/local/lib gcc  -DSQLITE_CORE -DSQLITE_THREADSAFE=2 -O3 -std=gnu99 -I/usr/local/include -I/usr/local/include/evhtp *.c -pthread  -ljemalloc -lfswatch -levent -lsqlite3 -levhtp -lconfuse -levent -o daap-jemalloc
