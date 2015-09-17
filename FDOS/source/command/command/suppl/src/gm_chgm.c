/*
    This file is part of SUPPL - the supplemental library for DOS
    Copyright (C) 1996-2000 Steffen Kaiser

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
/* $RCSfile: gm_chgm.c,v $
   $Locker:  $	$Name:  $	$State: Exp $

ob(ject): Erealloc
su(bsystem): error
ty(pe): 0H
sh(ort description): Reallocate a chunk of memory
lo(ng description): Reallocate a dynamically allocated chunk of
	memory.\par
	If \tok{poi == NULL}, a new block is allocated.\newline
	If \tok{len == 0}, the block is deallocated.\newline
	If \tok{poi == NULL && len == 0}, the behaviour is undefined.\par
	On failure, the program is terminated with the error message:
	"Out of memory".
pr(erequistes): 
va(lue): pointer to reallocated chunk
re(lated to): 
se(condary subsystems): dynstr
in(itialized by): 
wa(rning): 
bu(gs): 
co(mpilers): 

*/

#include "initsupl.loc"

#include <assert.h>
#ifndef _MICROC_
#include <stdlib.h>
#include <string.h>
#endif /*#	!defined(_MICROC_) */
#include <portable.h>
#include "msgs.h"

#include "dynstr.h"


#include "suppldbg.h"

#ifdef RCS_Version
static char const rcsid[] = 
	"$Id: gm_chgm.c,v 1.1 2006/06/17 03:25:05 blairdude Exp $";
#endif /*#	defined(RCS_Version) */

/*
 *	Reallocate a chunk of memory and terminate the application on failure
 */
void *Erealloc(void *p1, size_t size)
{	void *p;

	DBG_ENTER("Erealloc", Suppl_error)
	DBG_ARGUMENTS( ("poi=%p, new_len=%u", p1, size) )

	chkHeap
	if((p = realloc(p1, size)) == 0 && size)
		Esuppl_noMem();
	chkHeap
	DBG_RETURN_P( p)
}
