/* $Id: ctxt_as.c,v 1.1 2001/04/12 00:33:52 skaus Exp $

	Add status entry of a tag into context

	silent

	The status is sizemax encoded like this:
		sizemax | 0x8001
	--> neither of the bytes can become 0

	Return:
		E_None on success
*/

#include "../config.h"

#include <assert.h>

#include "../include/command.h"
#include "../include/context.h"

#if sizeof(unsigned) != 2
#error "sizeof(unsigned) != 2 is not supported"
#endif

int ctxtAddStatus(const Context_Tag tag)
{
	char value[3];

	ctxtCheckInfoTag(tag);

	*(unsigned*)value = CTXT_INFO(tag, sizemax) | 0x8001;
	value[sizeof(unsigned)] = 0;


	return ctxtSet(tag, 0, value);
}
