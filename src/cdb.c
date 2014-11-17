/*
	$Id: cdb.c,v 1.1.1.1 2009/10/26 11:16:01 erik Exp $
*/

/*
	***********************************************************
	djb's cdb
*/

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define READLE(x) ( *(unsigned int*) (x) )

#include "cdb.h"

/*
	open a cdb
	return 0 on success
*/
int cdb_open ( const char* name, Cdb *cdb )
{
	struct stat st;
	int fd = open( name, O_RDONLY, 0);
	cdb->map = -1 == fd
		? MAP_FAILED 
		: ( fstat(fd, &st) || 2048 > st.st_size
			? MAP_FAILED
			: mmap( 0, cdb->len = st.st_size, PROT_READ, MAP_SHARED, fd, 0 )
		);
	if ( -1 != fd ) {
		close( fd );
	}
	return MAP_FAILED == cdb->map;

}

/*
	close the cdb ( munmap )
*/
void cdb_close ( Cdb *cdb )
{
	munmap(cdb->map, cdb->len);
	cdb->map = 0;
}

/*
	read value for key
	a pointer to the value is stored in dst, the length is stored in dlen
	return 0 on success
*/
int cdb_read ( Cdb *cdb, char **dst, unsigned int *dlen,
		const uchar *key, unsigned int klen )
{
	unsigned int h = 5381, max = cdb->len;
	char *tb, *te, *ts, *tp;
	if ( klen > cdb->len - 2048 )
		return CDB_NOT_FOUND;

	/* compute hash */
	{
		register const uchar *p = key;
		register const uchar *e = key + klen;
		while ( p < e )
			h = ( ( h << 5 ) + h ) ^ *p++;
	}
	/* offset, #slots of table */
	{
		/* we checked c->len >= 2K */
		char *t = cdb->map + ( ( h & 0xff ) << 3) ; /* 2*4 byte units */
		unsigned int tpos = READLE( t );
		unsigned int tlen = READLE( t + 4 );
		unsigned int tend = tpos + ( tlen << 3 );
		if ( ! tlen ) return CDB_NOT_FOUND;
		if ( tpos > max || tlen > max || tend > max || tend < tpos
				|| tlen != ( tend-tpos ) >> 3 )  /* paranoia */
			return CDB_CORRUPT;
		tb = cdb->map + tpos; /* table base */
		te = tb + ( tlen << 3 ); /* table end */
		tp = ts = tb + ( ( h >> 8 ) % tlen << 3 ); /* table pointer, start slot */
	}
	max -= klen + 8; /* record pos */
	do { /* hash and a pos as tp */
		unsigned int rpos = READLE( tp + 4 );
		if ( !rpos ) return CDB_NOT_FOUND; /* no pos - empty slot */
		if ( h == READLE( tp ) ) { /* possible match */
			char *rec;
			if ( rpos > max )
				break;
			if ( klen == READLE(rec=cdb->map+rpos) && !memcmp( key, rec+8, klen) )
			{
				unsigned int rlen = READLE(rec+4);
				if ( rlen <= max-rpos ) {
					*dlen = rlen;
					*dst = rec + 8 + klen;
					return CDB_OK;
				}
				break;
			}
		}
		if ( te == ( tp += 8 ) )
			tp = tb;
	} while ( ts != tp );
	return CDB_CORRUPT;
}

