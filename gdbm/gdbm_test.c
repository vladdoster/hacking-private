#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gdbm.h>

int main() {
    GDBM_FILE dbf;
    datum key, content;

    char *dbfile = "db.gdbm";

    dbf = gdbm_open(dbfile, 0, GDBM_SYNC | GDBM_WRCREAT, 0666, 0);
    if (!dbf) {
        printf("error opening database file %s (%s)", dbfile, gdbm_strerror(gdbm_errno));
        return 1;
    }
    
    char *keystr = "test1", *contstr="value1";
    key.dptr = keystr;
    key.dsize = strlen(keystr);
    content.dptr = contstr;
    content.dsize = strlen(contstr);

    gdbm_store(dbf, key, content, GDBM_REPLACE);
    gdbm_reorganize(dbf);
    gdbm_close(dbf);

    dbf = gdbm_open(dbfile, 0, GDBM_SYNC | GDBM_WRCREAT, 0666, 0);
    if (!dbf) {
        printf("error opening database file %s (%s)", dbfile, gdbm_strerror(gdbm_errno));
        return 1;
    }

    return 0;
}
