/*
 *  md5.h - MD5 (RFC 1321) sum computation.
 * 
 *  Copyright (C) 2001-2003
 *  
 *  Christophe Devine <devine@cr0.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef H_MD5_
#define H_MD5_

#define uint8  unsigned char
#define uint32 unsigned long int

struct md5_context
{
    uint32 total[2];
    uint32 state[4];
    uint8 buffer[64];
};

extern void md5_starts( struct md5_context *ctx );
extern void md5_update( struct md5_context *ctx, uint8 *input, uint32 length );
extern void md5_finish( struct md5_context *ctx, uint8 digest[16] );

#endif /* H_MD5_ */
