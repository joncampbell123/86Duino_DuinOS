/* $Id: lastdset.c,v 1.1 2001/04/12 00:33:53 skaus Exp $

	Set Last Directory with current directory

*/

#include "../config.h"

#include <stdlib.h>

#include "../include/context.h"
#include "../include/misc.h"
#include "../err_fcts.h"

void lastDirSet(void)
{	char *dir;

	if((dir = cwd(0)) != 0) {
		ctxtSet(CTXT_TAG_LASTDIR, 1, dir);
		free(dir);
	} else
		error_out_of_memory();
}
