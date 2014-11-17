/*
	$Id: lcdb.c,v 1.1.1.1 2009/10/26 11:16:01 erik Exp $
*/

/*
	***********************************************************
	djb's cdb
	lua binding
*/

#include <string.h>

#include <lua.h>
#include <lauxlib.h>

#include "srv.h"
#include "size.h"

static const char lcdbtype[] = "cdb_handle";

typedef struct {
	Cdb cdb;
	char *fn;
} Lcdb;

static int lcdb_open( lua_State *L ) {
	Cdb cdb;
	Lcdb *u;
	SLEN_T n;
	const char *fn = luaL_checklstring( L, 1, &n );
	if ( cdb_open( fn, &cdb ) )
		return 0;
	u = (Lcdb *) lua_newuserdata( L, sizeof( *u ) + n + 1 );
	memcpy( &u->cdb, &cdb, sizeof( cdb ) );
	u->fn = ( (char *) u ) + sizeof( *u );
	memcpy( u->fn, fn, n );
	u->fn[n] = 0;
	luaL_getmetatable( L, lcdbtype );
	lua_setmetatable( L, -2 );
	return 1;
}

static int lcdb_close ( lua_State *L ) {
	Lcdb *lcdb = luaL_checkudata( L, 1, lcdbtype );
	if ( lcdb->cdb.map ) {
		cdb_close( &lcdb->cdb );
	}
	return 0;
}

#define LCDB_UDATA \
	Lcdb *lcdb  = luaL_checkudata( L, 1, lcdbtype ); \
	if ( ! lcdb->cdb.map ) \
		luaL_error( L, "cdb closed." )

static int lcdb_get ( lua_State *L ) {
	const char *k;
	char *v;
	int klen, vlen;
	SLEN_T l;
	LCDB_UDATA;
	k = luaL_checklstring( L, 2, &l );
	switch ( cdb_read( &lcdb->cdb, &v, &vlen, k, klen = (int)l ) ) {
	case CDB_OK:
		lua_pushlstring( L, v, vlen );
		return 1;
	case CDB_CORRUPT:
		luaL_error( L, "lcdb_get - cdb `%s´ is corrupt", lcdb->fn );
	}
	if ( 2 < lua_gettop( L ) ) {
		lua_pushvalue( L, 3 );
		return 1;
	}
	return 0;
}

static int lcdb_sget ( lua_State *L )
{
	if ( !lcdb_get( L ) ) lua_pushstring( L, "" );
	return 1;
}

static int lcdb_fn ( lua_State *L, int pre ) {
	Lcdb *lcdb = luaL_checkudata( L, 1, lcdbtype );
	if ( pre ) {
		lua_pushstring( L, lcdbtype );
		lua_pushstring( L, ": " );
	}
	lua_pushstring( L, lcdb->fn );
	if ( pre )
		lua_concat( L, 3 );
	return 1;
}

static int lcdb_tostring ( lua_State *L ) {
	return lcdb_fn( L, 1 );
}

static int lcdb_name ( lua_State *L ) {
	return lcdb_fn( L, 0 );
}

/*
	***********************************************************
	public functions
*/

static struct luaL_reg lcdb_meta_lib[] = {
	 { "__gc", lcdb_close }
	,{ "__tostring", lcdb_tostring }
	,{ "close", lcdb_close }
	,{ "get", lcdb_get }
	,{ "sget", lcdb_sget }
	,{ "name", lcdb_name }
	,{ 0, 0 }
};

static const luaL_reg lcdb_lib[] = {
	 { "open", lcdb_open }
	,{ 0, 0 }
};

int luaopen_cdb ( lua_State *L ) {
	luaL_newmetatable( L, lcdbtype );
	luaL_register(L, 0, lcdb_meta_lib);
	lua_pushliteral( L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	lua_pop( L, 1 );
	luaL_register( L, SLN_CDBNAME, lcdb_lib );
	return 1;
}

