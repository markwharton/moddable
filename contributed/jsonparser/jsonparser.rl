// Copyright (c) 2023 Mark Wharton
// https://opensource.org/license/mit/

// This Ragel code is based on the JSON parsing machine by Adrian Thurston.
// https://github.com/adrian-thurston/netp/blob/master/parse/json.rl

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
		xsmcCall( xsResult, xsVar(1), xsID_pop, slot, C_NULL );
		return xsmcToBoolean( xsResult );
	}
	--fsm->mark;
	return true;
}

void push( xsMachine *the, struct jsonparser *fsm, xsSlot *slot )
{
	if ( fsm->mark == 0 )
		xsmcCall( xsResult, xsVar(1), xsID_push, slot, C_NULL );
	else
		++fsm->mark;
}

bool pop_null( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(0);
	return pop( the, fsm, &xsResult );
}

void push_null( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(0);
	push( the, fsm, &xsResult );
}

bool pop_false( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(1);
	return pop( the, fsm, &xsResult );
}

void push_false( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(1);
	push( the, fsm, &xsResult );
}

bool pop_true( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(2);
	return pop( the, fsm, &xsResult );
}

void push_true( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(2);
	push( the, fsm, &xsResult );
}

bool pop_number( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(3);
	return pop( the, fsm, &xsResult );
}

void push_number( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(3);
	push( the, fsm, &xsResult );
}

bool pop_string( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(4);
	return pop( the, fsm, &xsResult );
}

void push_string( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(4);
	push( the, fsm, &xsResult );
}

bool pop_array( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(5);
	return pop( the, fsm, &xsResult );
}

void push_array( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(5);
	push( the, fsm, &xsResult );
}

bool pop_object( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(6);
	return pop( the, fsm, &xsResult );
}

void push_object( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(6);
	push( the, fsm, &xsResult );
}

bool pop_field( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(7);
	return pop( the, fsm, &xsResult );
}

void push_field( xsMachine *the, struct jsonparser *fsm )
{
	xsResult = xsInteger(7);
	push( the, fsm, &xsResult );
}

void set_text( xsMachine *the, struct jsonparser *fsm, char *buffer, int size )
{
	if ( fsm->mark == 0 ) {
		if ( fsm->buffer != C_NULL )
			xsmcSetStringBuffer( xsResult, fsm->buffer, fsm->size );
		xsmcCall( xsResult, xsVar(1), xsID_setText, &xsResult, C_NULL );
	}
}

%%{
	machine JSON;
	access fsm->;
	getkey c_read8( p ) ;

	prepush {
		if ( fsm->top == fsm->sd ) {
			fsm->stack = grow_stack( fsm->stack, &fsm->sd );
			if ( fsm->stack == C_NULL )
				xsUnknownError( (char *)not_enough_memory );
		}
	}

	ws              = [ \t\r\n];
	ignore          = ws; # | comment
	name_separator  = ':';
	value_separator = ',';
	Vnull           = 'null';
	Vfalse          = 'false';
	Vtrue           = 'true';
	begin_value     = [nft\"\-\[\{] | digit;
	begin_object    = '{';
	end_object      = '}';
	begin_array     = '[';
	end_array       = ']';
	begin_string    = '"';
	begin_name      = begin_string;
	begin_number    = digit | '-';

	action buf { buffer_char( the, fsm, c_read8( p ) ); }

	name := (
			'"'
			(
				'\\"' |
				'\\\\' |
				[^"\\]+
			)* $buf
			'"'
		)
		@{
			xsmcSetStringBuffer( xsResult, fsm->buffer, fsm->size );
			if ( fsm->mark == 0 ) {
				if ( xsmcTypeOf( xsVar(0) ) != xsUndefinedType ) {
					xsmcCall( xsVar(2), xsVar(0), xsID_includes, &xsResult, C_NULL );
					if ( !xsmcToBoolean( xsVar(2) ) ) {
						// fields are pushed before the name is known, so we have to check and balance the tree
						// prune field node that was rejected because it failed to match any of the keys
						pop_field( the, fsm );
						++fsm->mark;
					}
				}
			}
			set_text( the, fsm, C_NULL, 0 );
			fsm->size = 0;
			fret;
		};

	number :=
		(
			'-'?
			( '0' | [1-9][0-9]* )
			( '.' @{ fsm->dot = true; } [0-9]+ )?
			( [Ee] @{ fsm->E = true; } [+\-]? [0-9]+ )?
		) >{ fsm->dot = false; fsm->E = false; } $buf
		[^.Ee0-9]
		@{
			set_text( the, fsm, fsm->buffer, fsm->size );
			fsm->size = 0;
			if ( !pop_number( the, fsm ) ) fgoto *JSON_error;
			fhold; fret;
		};

	string :=
	(
		'"'
		(
			^( [\"\\] | 0..0x1f ) @buf |

			'\\"'  @{ buffer_char( the, fsm, '"' ); } |
			'\\\\' @{ buffer_char( the, fsm, '\\' ); } |
			'\\/'  @{ buffer_char( the, fsm, '/' ); } |
			'\\b'  @{ buffer_char( the, fsm, '\b' ); } |
			'\\f'  @{ buffer_char( the, fsm, '\f' ); } |
			'\\n'  @{ buffer_char( the, fsm, '\n' ); } |
			'\\r'  @{ buffer_char( the, fsm, '\r' ); } |
			'\\t'  @{ buffer_char( the, fsm, '\t' ); } |

			( '\\u'[0-9a-fA-F]{4} ) @{ buffer_char( the, fsm, '?' ); } | # TODO: \u value substitution
			( '\\'^([\"\\/bfnrtu]|0..0x1f) )
		)*
		'"'
	)
	@{
		set_text( the, fsm, fsm->buffer, fsm->size );
		fsm->size = 0;
		if ( !pop_string( the, fsm ) ) fgoto *JSON_error;
		fret;
	};

	action call_value { fhold; fcall value; }

	next_element =
		value_separator
		ignore*
		begin_value >call_value;

	array :=
		begin_array
		ignore*
		(
			begin_value >call_value
			ignore*
			( next_element ignore* )*
		)?
		end_array @{
			if ( !pop_array( the, fsm ) ) fgoto *JSON_error;
			fret;
		};

	action call_name { push_field( the, fsm ); fhold; fcall name; }

	pair =
		ignore*
		begin_name >call_name
		ignore*
		name_separator
		ignore*
		begin_value >call_value;

	object :=
	# At lease one field.
	(
		begin_object
		(
			pair
			(
				ignore*
				value_separator @{
					if ( !pop_field( the, fsm ) ) fgoto *JSON_error;
				}
				pair
			)*
		)
		ignore*
		end_object @{
			if ( !pop_field( the, fsm ) ) fgoto *JSON_error;
			if ( !pop_object( the, fsm ) ) fgoto *JSON_error;
			fret;
		}
	)
	|
	# Empty object.
	(
		begin_object
		ignore*
		end_object @{
			if ( !pop_object( the, fsm ) ) fgoto *JSON_error;
			fret;
		}
	);

	action parse_null   { push_null( the, fsm ); pop_null( the, fsm ); }
	action parse_false  { push_false( the, fsm ); pop_false( the, fsm ); }
	action parse_true   { push_true( the, fsm ); pop_true( the, fsm ); }
	action parse_number { push_number( the, fsm ); fhold; fcall number; }
	action parse_string { push_string( the, fsm ); fhold; fcall string; }
	action parse_array  { push_array( the, fsm ); fhold; fcall array; }
	action parse_object { push_object( the, fsm ); fhold; fcall object; }

	value :=
		ignore*
		(
			Vnull  @parse_null |
			Vfalse @parse_false |
			Vtrue  @parse_true |
			begin_number >parse_number |
			begin_string >parse_string |
			begin_array  >parse_array |
			begin_object >parse_object
		)
		(
			any @{
				fhold;
				fret;
			}
		|
			zlen @{
				fret;
			}
		);

	main := any @call_value ignore*;

}%%

%% write data;

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

	fsm->bs = xsmcArgc >= 3 ? xsmcToInteger(xsArg(2)) : 64; // optional initialBufferSize (default value is 64)
	fsm->buffer = c_malloc(fsm->bs * sizeof(char));
	if (fsm->buffer == C_NULL)
		xsUnknownError((char *)not_enough_memory);
	fsm->size = 0;

	fsm->sd = xsmcArgc >= 2 ? xsmcToInteger(xsArg(1)) : 8; // optional initialStackDepth (default value is 8)
	fsm->stack = c_malloc(fsm->sd * sizeof(int));
	if (fsm->stack == C_NULL)
		xsUnknownError((char *)not_enough_memory);

	%% write init;

	fsm->mark = 0;

	xsResult = xsmcArgc >= 1 ? xsArg(0) : xsUndefined;
	xsmcSet(xsThis, xsID_keys, xsResult); // optional keys array

	xsmcGet(xsResult, xsThis, xsID_vpt);
	xsmcCall(xsResult, xsResult, xsID_initialize, C_NULL);
}

void xs_jsonparser_receive(xsMachine *the)
{
	struct jsonparser *fsm = xsmcGetHostData(xsThis);

	int count = 0;
	if (fsm->cs != JSON_error) {

		xsmcVars(3);
		xsmcGet(xsVar(0), xsThis, xsID_keys);
		xsmcGet(xsVar(1), xsThis, xsID_vpt);
		xsmcSetUndefined(xsVar(2)); // for xsID_includes result

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

					%% write exec;

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

	xsmcGet(xsResult, xsThis, xsID_vpt);
	xsmcCall(xsResult, xsResult, xsID_terminate, C_NULL);

	xsmcGet(xsResult, xsThis, xsID_constructor);
	if (fsm->cs == JSON_error)
		xsmcGet(xsResult, xsResult, xsID_failure);
	else if (fsm->cs >= JSON_first_final)
		xsmcGet(xsResult, xsResult, xsID_success);
	else
		xsmcGet(xsResult, xsResult, xsID_receive);
}
