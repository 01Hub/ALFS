/*
 *  comm.h - Communication functions (mostly)
 *
 *  Copyright (C) 2001-2003
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


/*
 * Sockets for frontend<->backend communication.
 */
typedef enum socket_e {
	FRONTEND_DATA_SOCK,
	BACKEND_DATA_SOCK,

	FRONTEND_CTRL_SOCK,
	BACKEND_CTRL_SOCK
} socket_e;


/*
 * Types of control messages between frontend and backend -- for sending
 * instructions, reporting statuses etc.
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
	CTRL_REQUEST_EL_STATUS,

	CTRL_UNKNOWN /* Has to be the last one (used for max number length). */
} ctrl_msg_type_e;


typedef struct ctrl_msg_s {
	ctrl_msg_type_e type;

	char *content;
} ctrl_msg_s;


int comm_get_socket(socket_e s);

ctrl_msg_type_e comm_msg_type(ctrl_msg_s *msg);
char *comm_msg_content(ctrl_msg_s *msg);

void comm_free_message(ctrl_msg_s *msg);

int comm_read_to_file(socket_e s, const char *file);
int comm_read_to_memory(socket_e s, char **ptr, size_t *size);

int comm_send_from_file(socket_e s, ctrl_msg_type_e t, const char *file);
int comm_send_from_memory(socket_e s, const char *ptr, size_t size);

ctrl_msg_s *comm_read_ctrl_message(socket_e s);
int comm_send_ctrl_msg(socket_e s, ctrl_msg_type_e t, const char *f, ...);

void comm_create_socket_pairs(void);


#endif /* H_COMM_ */
