/*
 *  comm.h - Communication functions (mostly)
 *
 *  Copyright (C) 2001, 2002
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


#ifndef H_COMM_
#define H_COMM_


/* Types of control messages between frontend and backend 
 * - for sending instructions, reporting statuses etc.
 */
typedef enum ctrl_msg_type_e {
	/* frontend <- backend */
	CTRL_ELEMENT_STARTED,
	CTRL_ELEMENT_ENDED,
	CTRL_ELEMENT_FAILED,
	CTRL_REQUESTING_STATE,

	/* frontend -> backend */
	CTRL_NO_STATE,
	CTRL_LOG_CHANGED_FILES,
	CTRL_LOG_HANDLER_ACTIONS,
	CTRL_SYSTEM_OUTPUT,
	CTRL_LOG_BACKEND,
	CTRL_PAUSE,
	CTRL_STOP,
	// CTRL_GENERATE_STAMP,

	/* frontend <-> backend */
	CTRL_SENDING_LOG_FILE,
	CTRL_SENDING_FILES_FILE,
	CTRL_SENDING_STATE,
	CTRL_DATA,
	CTRL_DATA_SENT,

	CTRL_UNKNOWN /* Has to be last (using it for maximum number length) */
} ctrl_msg_type_e;

typedef struct ctrl_msg_s {
	ctrl_msg_type_e type;
	char *content;
} ctrl_msg_s;

/*
 * Sockets for frontend<->backend communication.
 */

extern int data_sock[2];
#define FRONTEND_DATA_SOCKET (data_sock[0])
#define BACKEND_DATA_SOCKET (data_sock[1])

extern int ctrl_sock[2];
#define FRONTEND_CTRL_SOCKET (ctrl_sock[0])
#define BACKEND_CTRL_SOCKET (ctrl_sock[1])


int send_ctrl_msg(int s, ctrl_msg_type_e t, const char *format, ...);
ctrl_msg_s *read_ctrl_message(int sock);

int read_to_file(int sock, const char *file);
void read_to_memory(int sock, char **ptr, int *size);

int send_from_file(int s, const char *file, ctrl_msg_type_e type);
int send_from_memory(int s, const char *ptr, int size, ctrl_msg_type_e type);


#endif
