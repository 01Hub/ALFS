/*
 *  comm.c - Communication functions (mostly)
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


#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "win.h"
#include "utility.h"
#include "nalfs.h"
#include "config.h"

#include "comm.h"


struct ctrl_msg_s {
	ctrl_msg_type_e type;

	char *content;
};


static int ctrl_sock[2]; /* Sockets for control messages. */
static int data_sock[2]; /* Sockets for backend Nprint() messages. */


int comm_get_socket(socket_e sock)
{
	switch (sock) {
		case FRONTEND_DATA_SOCK:
			return data_sock[0];

		case BACKEND_DATA_SOCK:
			return data_sock[1];

		case FRONTEND_CTRL_SOCK:
			return ctrl_sock[0];

		case BACKEND_CTRL_SOCK:
			return ctrl_sock[1];
	}

	return -1;
}


ctrl_msg_type_e comm_msg_type(ctrl_msg_s *msg)
{
	return msg->type;
}

char *comm_msg_content(ctrl_msg_s *msg)
{
	return msg->content;
}


void comm_free_message(ctrl_msg_s *msg)
{
	xfree(msg->content);
	xfree(msg);
}


/*
 * Reads from socket into specified file.  File can be NULL, in which
 * case we only read all CTRL_DATA messages, without storing them.
 */
int comm_read_to_file(socket_e sock, const char *file)
{
	int status = 0;
	FILE *fp = NULL;
	ctrl_msg_s *message;


	if (file) {
		if ((fp = fopen(file, "w")) == NULL) {
			Nprint_err("Can't open \"%s\" for writing:", file);
			Nprint_err("%s. Receiving file will be ignored.",
				strerror(errno));
			status = 1;

		} else {
			Debug_logging("Reading to a file...");
		}
	} else {
		Nprint_warn("File is NULL, ignoring received file.");
		status = 1;
	}

	while (1) {
		if ((message = comm_read_ctrl_message(sock)) == NULL) {
			napms(1);
			continue;
		}

		if (message->type == CTRL_DATA) {
			if (fp) {
				fprintf(fp, "%s", message->content);
			}

		} else if (message->type == CTRL_DATA_SENT) {
			xfree(message);
			break;

		} else {
			Nprint_warn("Got unexpected control "
				"message while reading a file:");
			Nprint_warn("\"%s\"", message->content);
		}

		xfree(message->content);
		xfree(message);
	}

	if (fp) {
		fclose(fp);
	}

	return status;
}

int comm_read_to_memory(socket_e sock, char **ptr, size_t *size)
{
	ctrl_msg_s *message;


	Debug_logging("Reading from backend to memory...");

	*ptr = NULL;
	*size = 0;

	while (1) {
		if ((message = comm_read_ctrl_message(sock)) == NULL) {
			napms(1);
			continue;
		}

		if (message->type == CTRL_DATA) {
			/* Write to memory here. */
			append_str(ptr, message->content);
			*size += strlen(message->content);

		} else if (message->type == CTRL_DATA_SENT) {
			xfree(message);
			break;

		} else {
			Nprint_warn("Got unexpected control "
				"message while reading a file:");
			Nprint_warn("\"%s\"", message->content);
		}

		xfree(message->content);
		xfree(message);
	}

	return 0;
}


int comm_send_from_file(socket_e s, ctrl_msg_type_e type, const char *file)
{
	int fd;
	char buf[MAX_CTRL_MSG_LEN + 1];
	ssize_t i;


	if ((fd = open(file, O_RDONLY)) == -1) {
		Nprint_err("Can't open %s for reading:", file);
		Nprint_err("%s", strerror(errno));
		return -1;
	}

	/* Notify the other side that we are going to start sending data. */
	comm_send_ctrl_msg(s, type, "Sending data...");

	while ((i = read(fd, buf, sizeof buf - 1)) > 0) {
		buf[i] = '\0';
		comm_send_ctrl_msg(s, CTRL_DATA, "%s", buf);
	}

	close(fd);

	/* Tell the other side that we are done sending DATA. */
	comm_send_ctrl_msg(s, CTRL_DATA_SENT, "Done sending DATA.");

	return 0;
}

int comm_send_from_memory(
	socket_e s, ctrl_msg_type_e type, const char *ptr, size_t size)
{
	char buf[MAX_CTRL_MSG_LEN + 1];
	const char *current = ptr;


	/* Notify the other side that we are going to start sending data. */
	comm_send_ctrl_msg(s, type, "Sending data...");

	while (current < ptr + size) {
		size_t max = Min(MAX_CTRL_MSG_LEN, ptr + size - current);

		strncpy(buf, current, max);
		buf[max] = '\0';

		comm_send_ctrl_msg(s, CTRL_DATA, "%s", buf);

		current += max;
	}

	/* Tell the other side that we are done sending data. */
	comm_send_ctrl_msg(s, CTRL_DATA_SENT, "Done sending data.");

	return 0;
}


static int is_read_available(int s, int sec)
{
	int i;
	fd_set fds;
	struct timeval tv;


	if (s == -1) {
		return 0;
	}

	FD_ZERO(&fds);
	FD_SET(s, &fds);
	tv.tv_sec = sec;
	tv.tv_usec = 0;

	i = select(s + 1, &fds, NULL, NULL, &tv);

	if (i && FD_ISSET(s, &fds)) {
		return 1;
	}

	return 0;
}

/*
 * Returns number of bytes read, or zero if there is nothing on
 * the socket for reading.  It reads maximum "top" bytes.  Socket must block.
 */
static INLINE int socket_read_max(int s, char *buf, int max)
{
	int total_read = 0;
	ssize_t now_read;


	while (total_read < max && is_read_available(s, 0)) {
		now_read = read(s, buf, max - total_read);

		if (now_read < 0) {
			return -1;

		} else if (now_read == 0) {
			return total_read;

		} else {
			total_read += (int) now_read;
			buf += (int) now_read;
		}
	}

	return total_read;
}

/*
 * Reads a message from the socket by checking its size at the beginning.
 * Returns a pointer to a message (without the size).
 */
static char *do_read_ctrl_message(int s)
{
	int total_read, size_len;


	if (! is_read_available(s, 0)) {
		return NULL;
	}

	total_read = 0;
	size_len = number_len(number_len(CTRL_UNKNOWN)+1+MAX_CTRL_MSG_LEN)+1;

	while (total_read < size_len) {
		char size_buf[size_len + 1];
		char *pos = size_buf + total_read;

		if (read(s, pos, 1) != 1) {
			break;
		}

		pos[1] = '\0';

		++total_read;

		if (*pos == '|') {
			int i;
			char *message = NULL;
			long size = strtol(size_buf, (char **)NULL, 10);


			if (size == LONG_MIN || size == LONG_MAX || size == 0) {
				Nprint_err("Can find out msg size from \"%s\".",
					size_buf);
				break;
			}

			message = xmalloc(size + 1);

			if ((i = socket_read_max(s, message, size)) > 0) {
				message[i] = '\0';
				return message;
			}

			xfree(message);

			break;
		}
	}

	return NULL;
}

ctrl_msg_s *comm_read_ctrl_message(socket_e s)
{
	char *tmp, *body;
	ctrl_msg_s *message;


	if ((body = do_read_ctrl_message(comm_get_socket(s))) == NULL) {
		return NULL;
	}

	if ((tmp = strchr(body, '|')) == NULL) {
		xfree(body);
		return NULL;
	}
	*tmp++ = '\0';

	message = xmalloc(sizeof *message);

	message->type = atoi(body);

	if (tmp && *tmp) {
		message->content = xstrdup(tmp);
	} else {
		message->content = NULL;
	}

	xfree(body);

	return message;
}

int comm_send_ctrl_msg(
	socket_e s, ctrl_msg_type_e t, const char *format, ...)
{
	size_t i;
	ssize_t ret;
	va_list ap;
	char *full_message;
	char *size, *type, raw_msg[MAX_CTRL_MSG_LEN + 1];


	/* Create raw message string. */
	va_start(ap, format);
	vsnprintf(raw_msg, sizeof raw_msg, format, ap); /* TODO: Check. */
	va_end(ap);

	/* Create type string. */
	type = xmalloc(number_len((int)t) + 2);
	sprintf(type, "%d|", (int)t);

	i = strlen(type) + strlen(raw_msg);

	/* Create size string. */
	size = xmalloc(number_len((int)i) + 2);
	sprintf(size, "%lu|", (unsigned long)i);


	full_message = xmalloc(strlen(size) + i + 1);

	strcpy(full_message, size);
	strcat(full_message, type);
	strcat(full_message, raw_msg);

	xfree(size);
	xfree(type);

	ret = write(comm_get_socket(s), full_message, strlen(full_message));

	xfree(full_message);

	if (ret == -1) {
		Nprint_err("write() failed: %s", strerror(errno));
		return -1;
	}

	return 0;
}


void comm_create_socket_pairs(void)
{
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, data_sock) == -1) {
		Fatal_error("socketpair() failed: %s", strerror(errno));
	}
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, ctrl_sock) == -1) {
		Fatal_error("socketpair() failed: %s", strerror(errno));
	}

	/* Non-blocking reading for our data socket. */
	if (fcntl(comm_get_socket(FRONTEND_DATA_SOCK), F_SETFL, O_NONBLOCK) == -1) {
		Fatal_error("fcntl() failed: %s", strerror(errno));
	}
}
