
#line 1 "jsonparser.rl"
// Copyright 2023 Mark Wharton (mark@jynx.com)
// https://opensource.org/license/mit/

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h" // for xsID_ values

struct jsonparser
{
	int size, bs;
	char *buffer;

	int cs, top, sd;
	int *stack;

	int dot, E;

	int mark;
};

char *grow_buffer( char *buffer, int *pbs )
{
	int old = *pbs;
	*pbs = *pbs * 2;

	char *nb = c_malloc( *pbs * sizeof(char) );
	if ( nb != C_NULL )
		c_memcpy( nb, buffer, old * sizeof(char) );
	c_free( buffer );
	return nb;
}

int *grow_stack( int *stack, int *psd )
{
	int old = *psd;
	*psd = *psd * 2;

	int *ns = c_malloc( *psd * sizeof(int) );
	if ( ns != C_NULL )
		c_memcpy( ns, stack, old * sizeof(int) );
	c_free( stack );
	return ns;
}

static const char not_enough_memory[] ICACHE_XS6STRING_ATTR = "jsonparser out of memory";

void buffer_char( xsMachine *the, struct jsonparser *fsm, char c )
{
	if ( fsm->size == fsm->bs ) {
		fsm->buffer = grow_buffer( fsm->buffer, &fsm->bs );
		if ( fsm->buffer == C_NULL )
			xsUnknownError( (char *)not_enough_memory );
	}
	fsm->buffer[fsm->size++] = c;
}

bool pop( xsMachine *the, struct jsonparser *fsm, xsSlot *slot )
{
	if ( fsm->mark == 0 ) {
		xsmcCall( xsResult, xsVar(9), xsID_pop, slot, C_NULL );
		return xsmcToBoolean( xsResult );
	}
	--fsm->mark;
	return true;
}

void push( xsMachine *the, struct jsonparser *fsm, xsSlot *slot )
{
	if ( fsm->mark == 0 )
		xsmcCall( xsResult, xsVar(9), xsID_push, slot, C_NULL );
	else
		++fsm->mark;
}

bool pop_null( xsMachine *the, struct jsonparser *fsm )
{
	return pop( the, fsm, &xsVar(0) );
}

void push_null( xsMachine *the, struct jsonparser *fsm )
{
	push( the, fsm, &xsVar(0) );
}

bool pop_false( xsMachine *the, struct jsonparser *fsm )
{
	return pop( the, fsm, &xsVar(1) );
}

void push_false( xsMachine *the, struct jsonparser *fsm )
{
	push( the, fsm, &xsVar(1) );
}

bool pop_true( xsMachine *the, struct jsonparser *fsm )
{
	return pop( the, fsm, &xsVar(2) );
}

void push_true( xsMachine *the, struct jsonparser *fsm )
{
	push( the, fsm, &xsVar(2) );
}

bool pop_number( xsMachine *the, struct jsonparser *fsm )
{
	return pop( the, fsm, &xsVar(3) );
}

void push_number( xsMachine *the, struct jsonparser *fsm )
{
	push( the, fsm, &xsVar(3) );
}

bool pop_string( xsMachine *the, struct jsonparser *fsm )
{
	return pop( the, fsm, &xsVar(4) );
}

void push_string( xsMachine *the, struct jsonparser *fsm )
{
	push( the, fsm, &xsVar(4) );
}

bool pop_array( xsMachine *the, struct jsonparser *fsm )
{
	return pop( the, fsm, &xsVar(5) );
}

void push_array( xsMachine *the, struct jsonparser *fsm )
{
	push( the, fsm, &xsVar(5) );
}

bool pop_object( xsMachine *the, struct jsonparser *fsm )
{
	return pop( the, fsm, &xsVar(6) );
}

void push_object( xsMachine *the, struct jsonparser *fsm )
{
	push( the, fsm, &xsVar(6) );
}

bool pop_field( xsMachine *the, struct jsonparser *fsm )
{
	return pop( the, fsm, &xsVar(7) );
}

void push_field( xsMachine *the, struct jsonparser *fsm )
{
	push( the, fsm, &xsVar(7) );
}

void set_text( xsMachine *the, struct jsonparser *fsm, char *buffer, int size )
{
	if ( fsm->mark == 0 ) {
		if ( fsm->buffer != C_NULL )
			xsmcSetStringBuffer( xsResult, fsm->buffer, fsm->size );
		xsmcCall( xsResult, xsVar(9), xsID_setText, &xsResult, C_NULL );
	}
}


#line 358 "jsonparser.rl"



#line 172 "jsonparser.c"
static const unsigned char _JSON_key_offsets[] ICACHE_XS6RO_ATTR = {
	0, 0, 0, 1, 3, 5, 9, 12, 
	17, 19, 24, 28, 30, 35, 40, 41, 
	45, 56, 62, 68, 74, 80, 81, 95, 
	101, 114, 115, 121, 126, 139, 145, 150, 
	163, 164, 165, 166, 167, 168, 169, 170, 
	171, 172, 173, 177, 177, 177, 177, 177, 
	177, 177
};

static const char _JSON_trans_keys[] ICACHE_XS6RO_ATTR = {
	34, 34, 92, 34, 92, 45, 48, 49, 
	57, 48, 49, 57, 46, 69, 101, 48, 
	57, 48, 57, 46, 69, 101, 48, 57, 
	43, 45, 48, 57, 48, 57, 46, 69, 
	101, 48, 57, 46, 69, 101, 48, 57, 
	34, 34, 92, 0, 31, 34, 47, 92, 
	98, 102, 110, 114, 116, 117, 0, 31, 
	48, 57, 65, 70, 97, 102, 48, 57, 
	65, 70, 97, 102, 48, 57, 65, 70, 
	97, 102, 48, 57, 65, 70, 97, 102, 
	91, 13, 32, 34, 45, 91, 93, 102, 
	110, 116, 123, 9, 10, 48, 57, 13, 
	32, 44, 93, 9, 10, 13, 32, 34, 
	45, 91, 102, 110, 116, 123, 9, 10, 
	48, 57, 123, 13, 32, 34, 125, 9, 
	10, 13, 32, 58, 9, 10, 13, 32, 
	34, 45, 91, 102, 110, 116, 123, 9, 
	10, 48, 57, 13, 32, 44, 125, 9, 
	10, 13, 32, 34, 9, 10, 13, 32, 
	34, 45, 91, 102, 110, 116, 123, 9, 
	10, 48, 57, 97, 108, 115, 101, 117, 
	108, 108, 114, 117, 101, 13, 32, 9, 
	10, 0
};

static const char _JSON_single_lengths[] ICACHE_XS6RO_ATTR = {
	0, 0, 1, 2, 2, 2, 1, 3, 
	0, 3, 2, 0, 3, 3, 1, 2, 
	9, 0, 0, 0, 0, 1, 10, 4, 
	9, 1, 4, 3, 9, 4, 3, 9, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 2, 0, 0, 0, 0, 0, 
	0, 0
};

static const char _JSON_range_lengths[] ICACHE_XS6RO_ATTR = {
	0, 0, 0, 0, 0, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 0, 1, 
	1, 3, 3, 3, 3, 0, 2, 1, 
	2, 0, 1, 1, 2, 1, 1, 2, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 1, 0, 0, 0, 0, 0, 
	0, 0
};

static const unsigned char _JSON_index_offsets[] ICACHE_XS6RO_ATTR = {
	0, 0, 1, 3, 6, 9, 13, 16, 
	21, 23, 28, 32, 34, 39, 44, 46, 
	50, 61, 65, 69, 73, 77, 79, 92, 
	98, 110, 112, 118, 123, 135, 141, 146, 
	158, 160, 162, 164, 166, 168, 170, 172, 
	174, 176, 178, 182, 183, 184, 185, 186, 
	187, 188
};

static const char _JSON_indicies[] ICACHE_XS6RO_ATTR = {
	0, 1, 2, 4, 5, 3, 3, 3, 
	2, 6, 7, 8, 2, 9, 10, 2, 
	12, 13, 13, 2, 11, 14, 2, 2, 
	13, 13, 14, 11, 15, 15, 16, 2, 
	16, 2, 2, 2, 2, 16, 11, 12, 
	13, 13, 10, 11, 17, 2, 19, 20, 
	2, 18, 21, 22, 23, 24, 25, 26, 
	27, 28, 29, 2, 17, 30, 30, 30, 
	2, 31, 31, 31, 2, 32, 32, 32, 
	2, 33, 33, 33, 2, 34, 2, 34, 
	34, 35, 35, 35, 36, 35, 35, 35, 
	35, 34, 35, 2, 37, 37, 38, 36, 
	37, 2, 38, 38, 35, 35, 35, 35, 
	35, 35, 35, 38, 35, 2, 39, 2, 
	39, 39, 40, 41, 39, 2, 42, 42, 
	43, 42, 2, 43, 43, 44, 44, 44, 
	44, 44, 44, 44, 43, 44, 2, 45, 
	45, 46, 47, 45, 2, 48, 48, 40, 
	48, 2, 49, 49, 50, 51, 52, 53, 
	54, 55, 56, 49, 51, 2, 57, 2, 
	58, 2, 59, 2, 60, 2, 61, 2, 
	62, 2, 63, 2, 64, 2, 65, 2, 
	66, 2, 67, 67, 67, 2, 2, 2, 
	2, 2, 2, 68, 2, 0
};

static const char _JSON_trans_targs[] ICACHE_XS6RO_ATTR = {
	42, 3, 0, 3, 43, 4, 6, 7, 
	13, 7, 13, 44, 8, 10, 9, 11, 
	12, 15, 15, 45, 16, 15, 15, 15, 
	15, 15, 15, 15, 15, 17, 18, 19, 
	20, 15, 22, 23, 46, 23, 24, 26, 
	27, 47, 27, 28, 29, 29, 30, 47, 
	30, 31, 48, 48, 48, 32, 36, 39, 
	48, 33, 34, 35, 48, 37, 38, 48, 
	40, 41, 48, 42, 49
};

static const char _JSON_trans_actions[] ICACHE_XS6RO_ATTR = {
	1, 0, 0, 2, 3, 2, 4, 4, 
	4, 2, 2, 5, 6, 7, 2, 2, 
	2, 0, 2, 8, 0, 9, 10, 11, 
	12, 13, 14, 15, 16, 0, 0, 0, 
	0, 17, 0, 1, 18, 0, 0, 0, 
	19, 20, 0, 0, 1, 0, 21, 22, 
	0, 0, 23, 24, 25, 0, 0, 0, 
	26, 0, 0, 0, 27, 0, 0, 28, 
	0, 0, 29, 0, 30
};

static const int JSON_start = 1;
static const int JSON_first_final = 42;
static const int JSON_error = 0;

static const int JSON_en_name = 2;
static const int JSON_en_number = 5;
static const int JSON_en_string = 14;
static const int JSON_en_array = 21;
static const int JSON_en_object = 25;
static const int JSON_en_value = 31;
static const int JSON_en_main = 1;


#line 361 "jsonparser.rl"

#pragma unused (JSON_en_name)
#pragma unused (JSON_en_number)
#pragma unused (JSON_en_string)
#pragma unused (JSON_en_array)
#pragma unused (JSON_en_object)
#pragma unused (JSON_en_value)
#pragma unused (JSON_en_main)

int arg_to_index(xsMachine *the, int argi, int index, int length)
{
	if (xsmcArgc > argi && xsmcTypeOf(xsArg(argi)) != xsUndefinedType) {
		float i = c_trunc(xsmcToNumber(xsArg(argi)));
		if (c_isnan(i) || (i == 0))
			i = 0;
		if (i < 0) {
			i = length + i;
			if (i < 0)
				i = 0;
		}
		else if (i > length)
			i = length;
		index = i;
	}
	return index;
}

void xs_jsonparser_constructor(xsMachine *the)
{
	struct jsonparser *fsm = C_NULL;

	xsmcSet(xsThis, xsID_vpt, xsArg(0)); // mandatory vpt instance

	xsTry {
		fsm = c_malloc(sizeof(struct jsonparser));
		if (fsm == C_NULL)
			xsUnknownError((char *)not_enough_memory);
		xsmcSetHostData(xsThis, fsm);
		fsm->buffer = C_NULL;
		fsm->stack = C_NULL;
		fsm->mark = 0;
	}
	xsCatch {
		if (fsm != C_NULL)
			c_free(fsm);
		xsThrow(xsException);
	}
}

void xs_jsonparser_destructor(void* data)
{
	struct jsonparser *fsm = data;

	if (fsm != C_NULL) {
		if (fsm->buffer != C_NULL)
			c_free(fsm->buffer);
		if (fsm->stack != C_NULL)
			c_free(fsm->stack);
		c_free(fsm);
	}
}

void xs_jsonparser_initialize(xsMachine *the)
{
	struct jsonparser *fsm = xsmcGetHostData(xsThis);

	if (fsm->buffer != C_NULL)
		c_free(fsm->buffer);

	if (fsm->stack != C_NULL)
		c_free(fsm->stack);

	fsm->bs = xsmcArgc >= 2 ? xsmcToInteger(xsArg(1)) : 64; // optional initialBufferSize (default value is 64)
	fsm->buffer = c_malloc(fsm->bs * sizeof(char));
	if (fsm->buffer == C_NULL)
		xsUnknownError((char *)not_enough_memory);
	fsm->size = 0;

	fsm->sd = xsmcArgc >= 1 ? xsmcToInteger(xsArg(0)) : 8; // optional initialStackDepth (default value is 8)
	fsm->stack = c_malloc(fsm->sd * sizeof(int));
	if (fsm->stack == C_NULL)
		xsUnknownError((char *)not_enough_memory);

	
#line 388 "jsonparser.c"
	{
	 fsm->cs = JSON_start;
	 fsm->top = 0;
	}

#line 445 "jsonparser.rl"

	xsmcVars(1);
	xsmcGet(xsVar(0), xsThis, xsID_vpt);
	xsmcCall(xsResult, xsVar(0), xsID_initialize, C_NULL);
}

void xs_jsonparser_receive(xsMachine *the)
{
	struct jsonparser *fsm = xsmcGetHostData(xsThis);

	int count = 0;
	if (fsm->cs != JSON_error) {

		xsmcVars(12);
		xsmcSetInteger(xsVar(0), 0); // null
		xsmcSetInteger(xsVar(1), 1); // false
		xsmcSetInteger(xsVar(2), 2); // true
		xsmcSetInteger(xsVar(3), 3); // number
		xsmcSetInteger(xsVar(4), 4); // string
		xsmcSetInteger(xsVar(5), 5); // array
		xsmcSetInteger(xsVar(6), 6); // object
		xsmcSetInteger(xsVar(7), 7); // field
		xsmcSetInteger(xsVar(8), 8); // root
		xsmcGet(xsVar(9), xsThis, xsID_vpt);
		xsmcGet(xsVar(10), xsVar(9), xsID_keys);
		xsmcSetUndefined(xsVar(11)); // for includes

		// be careful with XS macros between here and copying our segment string into the buffer

		char *segment = xsmcToString(xsArg(0)); // mandatory segment string
		int length = c_strlen(segment);
		int start = arg_to_index(the, 1, 0, length); // optional segment slice start (default value is 0)
		int end = arg_to_index(the, 2, length, length); // optional segment slice end (default value is length)
		if (start < end) {
			int offset = start;
			int size = end - start;
			if (offset >= 0 && size > 0) {
				char *buffer = C_NULL;
				xsTry {
					buffer = c_malloc(size);
					if (buffer == C_NULL)
						xsUnknownError((char *)not_enough_memory);
					c_memcpy(buffer, segment + offset, size);

					const char *p = buffer;
					const char *pe = buffer + size;

					
#line 443 "jsonparser.c"
	{
	int _klen;
	const char *_keys;
	int _trans;

	if ( p == pe )
		goto _test_eof;
	if (  fsm->cs == 0 )
		goto _out;
_resume:
	_keys = _JSON_trans_keys + (unsigned char)c_read8( &_JSON_key_offsets[ fsm->cs] );
	_trans = (unsigned char)c_read8( &_JSON_index_offsets[ fsm->cs] );

	_klen = c_read8( &_JSON_single_lengths[ fsm->cs] );
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( ( c_read8( p ) ) < c_read8( _mid ))
				_upper = _mid - 1;
			else if ( ( c_read8( p ) ) > c_read8( _mid ))
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = c_read8( &_JSON_range_lengths[ fsm->cs] );
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( ( c_read8( p ) ) < c_read8( &_mid[0] ) )
				_upper = _mid - 2;
			else if ( ( c_read8( p ) ) > c_read8( &_mid[1] ) )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = c_read8( &_JSON_indicies[_trans] );
	 fsm->cs = c_read8( &_JSON_trans_targs[_trans] );

	if ( c_read8( &_JSON_trans_actions[_trans] ) == 0 )
		goto _again;

	switch ( c_read8( &_JSON_trans_actions[_trans] ) ) {
	case 2:
#line 193 "jsonparser.rl"
	{ buffer_char( the, fsm, c_read8( p ) ); }
	break;
	case 3:
#line 204 "jsonparser.rl"
	{
			xsmcSetStringBuffer( xsResult, fsm->buffer, fsm->size );
			if ( fsm->mark == 0 ) {
				if ( xsmcTypeOf( xsVar(10) ) != xsUndefinedType ) {
					xsmcCall( xsVar(11), xsVar(10), xsID_includes, &xsResult, C_NULL );
					if ( !xsmcToBoolean( xsVar(11) ) ) {
						// fields are pushed before the name is known, so we have to check and balance the tree
						// prune node that was rejected because it failed to match any of the keys
						pop_field( the, fsm );
						++fsm->mark;
					}
				}
			}
			set_text( the, fsm, C_NULL, 0 );
			fsm->size = 0;
			{ fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
		}
	break;
	case 5:
#line 230 "jsonparser.rl"
	{
			set_text( the, fsm, fsm->buffer, fsm->size );
			fsm->size = 0;
			if ( !pop_number( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
			p--; { fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
		}
	break;
	case 9:
#line 243 "jsonparser.rl"
	{ buffer_char( the, fsm, '"' ); }
	break;
	case 11:
#line 244 "jsonparser.rl"
	{ buffer_char( the, fsm, '\\' ); }
	break;
	case 10:
#line 245 "jsonparser.rl"
	{ buffer_char( the, fsm, '/' ); }
	break;
	case 12:
#line 246 "jsonparser.rl"
	{ buffer_char( the, fsm, '\b' ); }
	break;
	case 13:
#line 247 "jsonparser.rl"
	{ buffer_char( the, fsm, '\f' ); }
	break;
	case 14:
#line 248 "jsonparser.rl"
	{ buffer_char( the, fsm, '\n' ); }
	break;
	case 15:
#line 249 "jsonparser.rl"
	{ buffer_char( the, fsm, '\r' ); }
	break;
	case 16:
#line 250 "jsonparser.rl"
	{ buffer_char( the, fsm, '\t' ); }
	break;
	case 17:
#line 252 "jsonparser.rl"
	{ buffer_char( the, fsm, '?' ); }
	break;
	case 8:
#line 257 "jsonparser.rl"
	{
		set_text( the, fsm, fsm->buffer, fsm->size );
		fsm->size = 0;
		if ( !pop_string( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
		{ fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
	}
	break;
	case 1:
#line 264 "jsonparser.rl"
	{ p--; {
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	{ fsm->stack[ fsm->top++] =  fsm->cs;  fsm->cs = 31;goto _again;}} }
	break;
	case 18:
#line 279 "jsonparser.rl"
	{
			if ( !pop_array( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
			{ fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
		}
	break;
	case 19:
#line 284 "jsonparser.rl"
	{ push_field( the, fsm ); p--; {
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	{ fsm->stack[ fsm->top++] =  fsm->cs;  fsm->cs = 2;goto _again;}} }
	break;
	case 21:
#line 302 "jsonparser.rl"
	{
					if ( !pop_field( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
				}
	break;
	case 22:
#line 309 "jsonparser.rl"
	{
			if ( !pop_field( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
			if ( !pop_object( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
			{ fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
		}
	break;
	case 20:
#line 320 "jsonparser.rl"
	{
			if ( !pop_object( the, fsm ) ) { fsm->cs = (JSON_error); goto _again;}
			{ fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
		}
	break;
	case 28:
#line 326 "jsonparser.rl"
	{ push_null( the, fsm ); pop_null( the, fsm ); }
	break;
	case 27:
#line 327 "jsonparser.rl"
	{ push_false( the, fsm ); pop_false( the, fsm ); }
	break;
	case 29:
#line 328 "jsonparser.rl"
	{ push_true( the, fsm ); pop_true( the, fsm ); }
	break;
	case 24:
#line 329 "jsonparser.rl"
	{ push_number( the, fsm ); p--; {
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	{ fsm->stack[ fsm->top++] =  fsm->cs;  fsm->cs = 5;goto _again;}} }
	break;
	case 23:
#line 330 "jsonparser.rl"
	{ push_string( the, fsm ); p--; {
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	{ fsm->stack[ fsm->top++] =  fsm->cs;  fsm->cs = 14;goto _again;}} }
	break;
	case 25:
#line 331 "jsonparser.rl"
	{ push_array( the, fsm ); p--; {
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	{ fsm->stack[ fsm->top++] =  fsm->cs;  fsm->cs = 21;goto _again;}} }
	break;
	case 26:
#line 332 "jsonparser.rl"
	{ push_object( the, fsm ); p--; {
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	{ fsm->stack[ fsm->top++] =  fsm->cs;  fsm->cs = 25;goto _again;}} }
	break;
	case 30:
#line 346 "jsonparser.rl"
	{
				p--;
				{ fsm->cs =  fsm->stack[-- fsm->top]; goto _again;}
			}
	break;
	case 6:
#line 226 "jsonparser.rl"
	{ fsm->dot = true; }
#line 193 "jsonparser.rl"
	{ buffer_char( the, fsm, c_read8( p ) ); }
	break;
	case 7:
#line 227 "jsonparser.rl"
	{ fsm->E = true; }
#line 193 "jsonparser.rl"
	{ buffer_char( the, fsm, c_read8( p ) ); }
	break;
	case 4:
#line 228 "jsonparser.rl"
	{ fsm->dot = false; fsm->E = false; }
#line 193 "jsonparser.rl"
	{ buffer_char( the, fsm, c_read8( p ) ); }
	break;
#line 713 "jsonparser.c"
	}

_again:
	if (  fsm->cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	_out: {}
	}

#line 493 "jsonparser.rl"

					c_free(buffer);
					count = size;
				}
				xsCatch {
					if (buffer != C_NULL)
						c_free(buffer);
				}
				buffer = C_NULL;
			}
		}
	}
	xsResult = xsInteger(count);
}

void xs_jsonparser_terminate(xsMachine *the)
{
	struct jsonparser *fsm = xsmcGetHostData(xsThis);

	if (fsm->buffer != C_NULL) {
		c_free(fsm->buffer);
		fsm->buffer = C_NULL;
	}

	if (fsm->stack != C_NULL) {
		c_free(fsm->stack);
		fsm->stack = C_NULL;
	}

	xsmcVars(2);
	xsmcGet(xsVar(0), xsThis, xsID_constructor);
	xsmcGet(xsVar(1), xsThis, xsID_vpt);
	xsmcCall(xsResult, xsVar(1), xsID_terminate, C_NULL); // discard result

	if (fsm->cs == JSON_error)
		xsmcGet(xsResult, xsVar(0), xsID_failure);
	else if (fsm->cs >= JSON_first_final)
		xsmcGet(xsResult, xsVar(0), xsID_success);
	else
		xsmcGet(xsResult, xsVar(0), xsID_receive);
}
