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

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <limits.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bufsize.h"
#include "win.h"
#include "utility.h"
#include "nalfs-core.h"
#include "options.h"

#include "comm.h"


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
		Nprint_warn("Filename not specified, ignoring received data.");
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

int comm_send_from_memory(socket_e s, const char *ptr, size_t size)
{
	char buf[MAX_CTRL_MSG_LEN + 1];
	const char *current = ptr;


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
static INLINE unsigned long socket_read_max(int s, char *buf, unsigned long max)
{
	unsigned long total_read = 0;
	ssize_t now_read;


	while (total_read < max && is_read_available(s, 0)) {
		now_read = read(s, buf, max - total_read);

		if (now_read < 0) {
			return 0;

		} else if (now_read == 0) {
			return total_read;

		} else {
			total_read += now_read;
			buf += now_read;
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
	ssize_t num_read;
	size_t to_read = 9;
	char size_buf[10];
	char *pos;
	char *message = NULL;

	if (! is_read_available(s, 0)) {
		return NULL;
	}

	pos = &size_buf[0];

	while (to_read > 0) {
		num_read = read(s, pos, to_read);
		if (num_read < 0)
			break;
		to_read -= num_read;
		pos += num_read;
	}

	if (to_read > 0)
		return NULL;

	/* overwrite the last character read, which will be '|' */
	*--pos = '\0';

	to_read = strtoul(size_buf, (char **) NULL, 10);

	if (to_read == ULONG_MAX || to_read == 0) {
		Nprint_err("Can't read msg size from \"%s\".", size_buf);
		return NULL;
	}

	message = xmalloc(to_read + 1);

	if ((num_read = socket_read_max(s, message, to_read)) > 0) {
		message[num_read] = '\0';
		return message;
	}

	xfree(message);

	return NULL;
}

ctrl_msg_s *comm_read_ctrl_message(socket_e s)
{
	char *content, *body;
	ctrl_msg_s *message;


	if ((body = do_read_ctrl_message(comm_get_socket(s))) == NULL) {
		return NULL;
	}

	content = body + 5;
	*(body + 4) = '\0';

	message = xmalloc(sizeof *message);

	message->type = atoi(body);

	if (*content) {
		message->content = xstrdup(content);
	} else {
		message->content = NULL;
	}

	xfree(body);

	return message;
}

int comm_send_ctrl_msg(
	socket_e s, ctrl_msg_type_e t, const char *format, ...)
{
	ssize_t ret;
	va_list ap;
	char header[15];
	int result;
	char *buf;
	int buf_size = 1024;

	/* Create raw message string. */
	if ((buf = xmalloc(buf_size)) == NULL) {
		Nprint_err("xmalloc() failed: %s", strerror(errno));
		return -1;
	}
	while (1) {
		va_start(ap, format);
		result = vsnprintf(buf, buf_size, format, ap);
		va_end(ap);
		if (result >= buf_size) {
			buf_size = result + 1;
			if ((buf = xrealloc(buf, buf_size)) == NULL) {
				Nprint_err("xrealloc() failed: %s", strerror(errno));
				xfree(buf);
				return -1;
			}
		} else if (result < 0) {
			Nprint_err("vsnprintf() failed: %s", strerror(errno));
			xfree(buf);
			return -1;
		} else {
			break;
		}
	}

	/* header contains size of message (buf) plus type number and separators */
	/* message size does _NOT_ include trailing null, as it is not sent by write() */
	/* message size does _NOT_ include size prefix, or '|' separator */
	sprintf(header, "%08d|%04d|", result + 5, t);
	ret = write(comm_get_socket(s), header, 14);
	ret = write(comm_get_socket(s), buf, result);
	xfree(buf);
	if (ret == -1) {
		Nprint_err("write() failed: %s", strerror(errno));
		return -1;
	}
	return 0;
}


void comm_create_socket_pairs(void)
{
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, data_sock) == -1) {
		if (errno == EAFNOSUPPORT) {
			Fatal_error("Your kernel does not support UNIX domain sockets.");
		} else {
			Fatal_error("socketpair() failed: %s", strerror(errno));
		}
	}
	if (socketpair(PF_UNIX, SOCK_STREAM, 0, ctrl_sock) == -1) {
		if (errno == EAFNOSUPPORT) {
			Fatal_error("Your kernel does not support UNIX domain sockets.");
		} else {
			Fatal_error("socketpair() failed: %s", strerror(errno));
		}
	}

	/* Non-blocking reading for our data socket. */
	if (fcntl(comm_get_socket(FRONTEND_DATA_SOCK), F_SETFL, O_NONBLOCK) == -1) {
		Fatal_error("fcntl() failed: %s", strerror(errno));
	}
}
