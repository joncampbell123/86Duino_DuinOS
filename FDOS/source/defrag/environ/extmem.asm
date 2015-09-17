;    
;   extmem.asm - routine to test wether extended memory is available.
;   Copyright (C) 2004 Imre Leber
;
;   This program is free software; you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation; either version 2 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program; if not, write to the Free Software
;   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
;
;   If you have any questions, comments, suggestions, or fixes please
;   email me at:  imre.leber@worldonline.be
;

segment EXTMEM_TEXT

;=========================================================================
;===                      GetExtendedMemorySize                        ===
;===-------------------------------------------------------------------===
;===  Returns the number of kb free extended memory.                   ===
;===                                                                   ===
;===  unsigned GetExtendedMemorySize (void);                           ===
;=========================================================================

        global _GetExtendedMemorySize
_GetExtendedMemorySize:

        mov  ah, 88h
        int  15h

	jc   .error
	retf

.error:
    
        xor  ax, ax
        retf
