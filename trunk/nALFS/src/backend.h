/*
 *  backend.h - Backend.
 *
 *  Copyright (C) 2001, 2002, 2004
 *
 *  Neven Has <haski@sezampro.yu>
 *  Kevin P. Fleming <kpfleming@linuxfromscratch.org>
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


#ifndef H_BACKEND_
#define H_BACKEND_


#include "parser.h"
#include "comm.h"
#include "handlers.h"


#define Start_receiving_sigio() \
	set_receive_sigio(comm_get_socket(BACKEND_CTRL_SOCK), 1)

#define Stop_receiving_sigio() \
	set_receive_sigio(comm_get_socket(BACKEND_CTRL_SOCK), 0)

void set_receive_sigio(int s, int receive_it);


void fatal_backend_error(const char *format, ...);

int execute_direct_command(const char *command, char *const argv[]);
int execute_command(const element_s * const element, const char *format, ...);

int do_execute_test_element(element_s *element, int *result);
int execute_children(element_s *element);
int execute_children_filtered(element_s *element, handler_type_e type_filter);

void start_backend(element_s *el);


#endif /* H_BACKEND_ */
