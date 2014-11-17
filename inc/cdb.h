/*
	$Id: srv.h,v 1.1.1.1 2009/10/26 11:16:01 erik Exp $
*/

#ifndef SRV_H
#define SRV_H

/*
	------------------------------------------------------
	djb's cdb
*/
enum {
	 CDB_OK
	,CDB_NOT_FOUND
	,CDB_CORRUPT
};

typedef struct Cdb {
	char    *map; /* mmap */
	unsigned len;
} Cdb;

typedef unsigned char uchar;

int cdb_open ( const char *name, Cdb *cdb );
void cdb_close ( Cdb *cdb );
int cdb_read ( Cdb *cdb, char **dst, unsigned int *dlen,
		const uchar *key, unsigned int klen );

#define SESID_LEN 16

#endif /* SRV_H */
