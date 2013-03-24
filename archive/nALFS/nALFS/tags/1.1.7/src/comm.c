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
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "win.h"
#include "utility.h"
#include "nalfs.h"
#include "config.h"

#include "comm.h"


int data_sock[2];	/* Sockets for backend Nprint() messages.	*/
int ctrl_sock[2];	/* Sockets for control messages.		*/


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
 * the socket for reading. It reads maximum "top" bytes. Socket must block.
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

static INLINE int number_len(int num)
{
	int i = 1;


	while ((num /= 10))
		++i;

	return i;
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

ctrl_msg_s *read_ctrl_message(int sock)
{
	char *tmp, *body;
	ctrl_msg_s *message;


	if ((body = do_read_ctrl_message(sock)) == NULL) {
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

int send_ctrl_msg(int s, ctrl_msg_type_e t, const char *format, ...)
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

	ret = write(s, full_message, strlen(full_message));

	xfree(full_message);

	if (ret == -1) {
		Nprint_err("write() failed: %s", strerror(errno));
		return -1;
	}

	return 0;
}



/*
 * Reads from socket into specified file. File can be NULL, in which
 * case we only read all CTRL_DATA messages, without storing them.
 */
int read_to_file(int sock, const char *file)
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
		if ((message = read_ctrl_message(sock)) == NULL) {
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

void read_to_memory(int sock, char **ptr, int *size)
{
	ctrl_msg_s *message;


	Debug_logging("Reading from backend to memory...");

	*ptr = NULL;
	*size = 0;

	while (1) {
		if ((message = read_ctrl_message(sock)) == NULL) {
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
}



int send_from_file(int s, const char *file, ctrl_msg_type_e type)
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
	send_ctrl_msg(s, type, "Sending data...");

	while ((i = read(fd, buf, sizeof buf - 1)) > 0) {
		buf[i] = '\0';
		send_ctrl_msg(s, CTRL_DATA, "%s", buf);
	}

	close(fd);

	/* Tell the other side that we are done sending DATA. */
	send_ctrl_msg(s, CTRL_DATA_SENT, "Done sending DATA.");

	return 0;
}

int send_from_memory(int s, const char *ptr, int size, ctrl_msg_type_e type)
{
	int max;
	char buf[MAX_CTRL_MSG_LEN + 1];
	const char *current = ptr;


	/* Notify the other side that we are going to start sending data. */
	send_ctrl_msg(s, type, "Sending data...");

	while (current < ptr + size) {
		max = Min(MAX_CTRL_MSG_LEN, ptr + size - current);

		strncpy(buf, current, max);
		buf[max] = '\0';

		send_ctrl_msg(s, CTRL_DATA, "%s", buf);

		current += max;
	}


	/* Tell the other side that we are done sending data. */
	send_ctrl_msg(s, CTRL_DATA_SENT, "Done sending data.");

	return 0;
}
