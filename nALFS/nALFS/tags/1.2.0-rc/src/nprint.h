/*
 *  nprint.h - Macros to provide access to "printing" functions.
 *
 *  Copyright (C) 2001, 2002, 2003
 *
 *  Neven Has <haski@sezampro.yu>
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


#ifndef H_NPRINT_
#define H_NPRINT_

#include "logging.h"


/*
 * Types of messages.  They are also used as message_types[] indexes.
 * First should be 0, last T_ERR, order matters.
 */
typedef enum msg_id_e {
	T_RAW = 0,
	T_INF,
	T_HLP,
	T_SYS,
	T_WAR,
	T_ERR
} msg_id_e;

/*
 * Printing macros.
 */

extern void (*nprint)(msg_id_e mid, const char *format,...);

/* Print macros. */
#define Nprint(a, b...)		nprint(T_INF, a, ## b)
#define Nprint_warn(a, b...)	nprint(T_WAR, a, ## b)
#define Nprint_err(a, b...)	nprint(T_ERR, a, ## b)
#define Nprint_sys(a, b...)	nprint(T_SYS, a, ## b)
#define Nprint_raw(a, b...)	nprint(T_RAW, a, ## b)
#define Nprint_help(a, b...)	nprint(T_HLP, a, ## b)

/*
 * Print macros used in handlers.  TODO: Remove them and use the same
 * macros as above, just define them differently when used in handlers.
 */
#define Nprint_h(a, b...)	do { log_handler_action(a, ## b); \
					Nprint(a, ## b); } while (0)
#define Nprint_h_warn(a, b...)	do { log_handler_action(a, ## b); \
					Nprint_warn(a, ## b); } while (0)
#define Nprint_h_err(a, b...)	do { log_handler_action(a, ## b); \
					Nprint_err(a, ## b); } while (0)
#define Nprint_h_sys(a, b...)	do { log_handler_action(a, ## b); \
					Nprint_sys(a, ## b); } while (0)
#define Nprint_h_raw(a, b...)	do { log_handler_action(a, ## b); \
					Nprint_raw(a, ## b); } while (0)
#define Nprint_h_help(a, b...)	do{ log_handler_action(a, ## b); \
					Nprint_help(a, ## b); } while (0)
#endif /* H_NPRINT_ */
