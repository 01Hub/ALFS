/*
 *  editor.c - Editing functions.
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

#include <form.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "utility.h"
#include "win.h"
#include "nalfs-core.h"
#include "options.h"

#include "editor.h"


#ifdef USE_EDITOR

static FORM *editor_form;

static const int tab_length = 8;

#endif


static void call_driver(FORM *form, int req)
{
	switch (form_driver(form, req)) {
		case E_OK:
			break;

		case E_SYSTEM_ERROR:
			Nprint_err("Editor: System error: %s.",
				strerror(errno));
			break;
		case E_BAD_ARGUMENT:
			Nprint_err("Editor: Bad argument.");
			break;
		case E_BAD_STATE:
			Nprint_err("Editor: Bad state.");
			break;
		case E_NOT_POSTED:
			Nprint_err("Editor: Not posted.");
			break;
		case E_UNKNOWN_COMMAND:
			/* Nprint_err("Editor: Unknown command."); */
			break;
		case E_INVALID_FIELD:
			Nprint_err("Editor: Invalid field.");
			break;
		case E_REQUEST_DENIED:
			/* Nprint_err("Editor: Request denied."); */
			break;
	}
}

static int input_to_request(FORM *form, int c)
{
	int req = c;


	switch (c) {
		case KEY_BACKSPACE:
			req = REQ_DEL_PREV;
			break;
		case KEY_DC:
			req = REQ_DEL_CHAR;
			break;

		case KEY_UP:
			field_info(current_field(form),
				&c, NULL, NULL, NULL, NULL, NULL);
			if (c == 1) {
				req = REQ_PREV_FIELD;
			} else {
				req = REQ_UP_CHAR;
			}
			break;

		case KEY_DOWN:
			field_info(current_field(form),
				&c, NULL, NULL, NULL, NULL, NULL);
			if (c == 1) {
				req = REQ_NEXT_FIELD;
			} else {
				req = REQ_DOWN_CHAR;
			}
			break;

		case KEY_LEFT:
			req = REQ_PREV_CHAR;
			break;
		case KEY_RIGHT:
			req = REQ_NEXT_CHAR;
			break;

		case KEY_HOME:
			req = REQ_BEG_LINE;
			break;
		case KEY_END:
			req = REQ_END_LINE;
			break;

		case '\t':
			req = REQ_NEXT_FIELD;
			break;

		case '\n':
			req = REQ_NEW_LINE;
			break;

		case KEY_PPAGE:
			req = REQ_PREV_PAGE;
			break;
		case KEY_NPAGE:
			req = REQ_NEXT_PAGE;
			break;
	}

	return req;
}

static FIELD *make_label(int row, int col, const char *str)
{
	FIELD *f;


	if ((f = new_field(1, (int)strlen(str), row, col, 0, 0)) == NULL) {
		Fatal_error("new_field() failed");
	}

	if (set_field_buffer(f, 0, str) != E_OK) {
		Fatal_error("set_field_buffer() failed");
	}
	if (set_field_opts(f, field_opts(f) & ~O_ACTIVE & ~O_EDIT) != E_OK) {
		Fatal_error("set_field_opts() failed");
	}
	if (set_field_back(f, A_BOLD) != E_OK) {
		Fatal_error("set_field_back() failed");
	}

	return f;
}

static FIELD *make_field(int row, int col, const char *str, int rows, int cols)
{
	FIELD *f;


	if ((f = new_field(rows, cols, row, col, 0, 0)) == NULL) {
		Fatal_error("new_field() failed");
	}

	if (field_opts_off(f, O_STATIC) != E_OK) {
		Fatal_error("field_opts_off() failed");
	}

	if (str) {
		if (set_field_buffer(f, 0, str) != E_OK) {
			Nprint_err("Can't edit this string.");
		} else {
			set_field_just(f, JUSTIFY_LEFT);
		}
	}

	return f;
}

#ifdef USE_EDITOR
/* Replaces tabs and new lines with necessary amount of spaces. */
static char *text_to_field(const char *text)
{
	int i;
	int line_offset = 0;
	int width = windows.max_cols - 2;
	char *result = NULL;
	char *tmp, *string;


	tmp = string = xstrdup(text);

	while (*tmp) {
		if (*tmp == '\t') {
			for (i = line_offset%tab_length; i < tab_length; ++i) {
				append_str(&result, " ");
				++line_offset;
			}

		} else if (*tmp == '\n') {
			for (i = line_offset % width; i < width; ++i) {
				append_str(&result, " ");
			}

			line_offset = 0;

		} else {
			char str[2];

			str[0] = *tmp;
			str[1] = '\0';

			append_str(&result, str);

			++line_offset;
		}

		++tmp;
	}

	xfree(string);

	return result;
}

static char *field_to_text(const char *field_)
{
	int i;
	size_t width = windows.max_cols - 2;
	char line[width + 1];
	char *end, *result = NULL;
	char *f, *field;


	if ((field = alloc_trimmed_str(field_)) == NULL) {
		return NULL;
	}

	f = field;
	end = field + strlen(field) - 1;

	while (1) {
		strncpy(line, f, width);
		line[width] = '\0';

		for (i = width-1; i >= 0 && line[i] == ' '; --i) {
			line[i] = '\n';
			line[i+1] = '\0';
		}

		append_str(&result, line);

		if ((f = f + width) > end) {
			break;
		}
	}

	xfree(field);

	return result;
}

static INLINE void save_changes(element_s *el, FIELD **fields)
{
	int i, j;
	char *attr, *value;


	/* Update content. Can return NULL. */
	value = field_to_text(field_buffer(fields[5], 0));

	if (el->content) { /* Content existed. */
		if (value && strlen(value)) {
			if (strcmp(el->content, value) != 0) {
				xfree(el->content);
				el->content = xstrdup(value);

				Nprint("Content changed.");
			}

		} else {
			xfree(el->content);
			el->content = NULL;

			Nprint("Content deleted.");
		}

	} else { /* Contend was empty. */
		if (value && strlen(value)) {
			el->content = xstrdup(value);
			Nprint("Content added.");
		}
	}

	xfree(value);

/* TODO: figure out if this is even relevant */
#if 0
	/* Update attributes. */
	for (i = 6; fields[i]; i += 2) {
		attr = field_to_text(field_buffer(fields[i], 0));

		/* Can return NULL. */
		value = field_to_text(field_buffer(fields[i+1], 0));

		/* Find pointer to the old attribute value.
		 * It will be el->attr[j+1].
		 */
		for (j = 0; el->attr[j]; j += 2) {
			if (strcmp(el->attr[j], attr) == 0) {
				break;
			}
		}

		if (value) {
			if (strcmp(value, el->attr[j+1]) != 0) {
				/* Change is made. */
				xfree(el->attr[j+1]);
				el->attr[j+1] = xstrdup(value);

				Nprint("Attribute %s changed to %s.",
					attr, el->attr[j+1]);
			}

		} else {
			xfree(el->attr[j+1]);
			el->attr[j+1] = xstrdup("<empty>");
		}

		xfree(value);
		xfree(attr);
	}
#endif
}

typedef enum quit_type_e {
	QUIT_WITHOUT_SAVING,
	QUIT_AND_SAVE
} quit_type_e;

static INLINE quit_type_e main_input_loop(WINDOW *win)
{
	int input;
	int request;
	quit_type_e qt = QUIT_WITHOUT_SAVING;


	set_form_page(editor_form, 0);

	/* Set current field to content. */
	call_driver(editor_form, REQ_NEXT_FIELD);
	call_driver(editor_form, REQ_NEXT_FIELD);
	call_driver(editor_form, REQ_NEXT_FIELD);

	while (1) {
		pos_form_cursor(editor_form);

		input = get_key(win);

		if (input == KEY_F(10)) {
			qt = QUIT_WITHOUT_SAVING;
			break;

		} else if (input == KEY_F(5)) {
			qt = QUIT_AND_SAVE;
			break;

		} else {
			request = input_to_request(editor_form, input);
			call_driver(editor_form, request);
		}
	}

	call_driver(editor_form, REQ_VALIDATION);

	return qt;
}

static FIELD *make_header(int row, int col, const char *str)
{
	FIELD *f;


	if ((f = new_field(1, (int)strlen(str), row, col, 0, 0)) == NULL) {
		Fatal_error("new_field() failed");
	}

	if (set_field_buffer(f, 0, str) != E_OK) {
		Fatal_error("set_field_buffer() failed");
	}
	if (set_field_opts(f, field_opts(f) & ~O_EDIT) != E_OK) {
		Fatal_error("set_field_opts() failed");
	}

	return f;
}

static INLINE FIELD **create_fields(element_s *el)
{
	int i, num = 0;
	FIELD **fields;


	fields = xmalloc(6 * sizeof *fields);

	/* Add short help field. */
	fields[num++] = make_header(0, 0,
	"Use TAB to change fields, F10 (F5) to quit (and save changes),");
	fields[num++] = make_header(1, 0,
	"and PageUp / PageDown to scroll pages.");

	/* Add name field. */
	fields[num++] = make_label(3, 0, "Name: ");
	fields[num++] = make_label(3, 6, el->name);
	if (field_opts_on(fields[num-1], O_ACTIVE) != E_OK) {
		Fatal_error("field_opts_on() failed");
	}

	/* Add content field. */
	fields[num++] = make_label(5, 0, "Content: ");
	if (el->content) {
		char *s = text_to_field(el->content);

		fields[num++] = make_field(6, 0, s,
			windows.main->lines - 6, windows.max_cols - 2);

		xfree(s);

	} else {
		fields[num++] = make_field(6, 0, NULL,
			windows.main->lines - 6, windows.max_cols - 2);
	}

	fields[num] = NULL;

/* TODO: figure out if this is relevant */
	/* Add fields for attributes (if any). */
#if 0
	if (el->attr) {
		for (i = 0; el->attr[i]; i += 2) {
			int row = (i/2 % windows.main->lines);

			fields = xrealloc(fields, (num + 3) * sizeof *fields);

			fields[num++] =	make_label(row, 0, el->attr[i]);
			fields[num++] =	make_field(row, strlen(el->attr[i]) + 1,
				el->attr[i+1],
				1, windows.max_cols - 4 - strlen(el->attr[i]));

			fields[num] = NULL;

			if (row == 0) {
				if (set_new_page(fields[num-2], TRUE) != E_OK) {
					Fatal_error("set_new_page() failed");
				}
			}
		}
	}
#endif

	return fields;
}

void edit_element(element_s *el)
{
	int i;
	FIELD **fields;
	WINDOW *win, *swin;


	windows.active = TMP_WINDOW;

	fields = create_fields(el);

	if ((editor_form = new_form(fields)) == NULL) {
		Fatal_error("form() failed");
	}
	if (form_opts_off(editor_form, O_NL_OVERLOAD) != E_OK) {
		Fatal_error("form_opts_off() failed");
	}

	/* Create windows. */
	if ((win = newwin(
	windows.main->lines, windows.max_cols - 2, 1, 1)) == NULL) {
		Fatal_error("newwin() failed");
	}
	keypad(win, TRUE);
	nodelay(win, TRUE);

	if ((swin = derwin(win,
	windows.main->lines, windows.max_cols - 2, 0, 0)) == NULL) {
		Fatal_error("derwin() failed");
	}
	keypad(swin, TRUE);
	nodelay(swin, TRUE);

	/* Make windows associations. */
	if (set_form_sub(editor_form, swin) != E_OK) {
		Fatal_error("set_form_sub() failed");
	}
	if (set_form_win(editor_form, win) != E_OK) {
		Fatal_error("set_form_win() failed");
	}

	if (post_form(editor_form) != E_OK) {
		Fatal_error("post_form() failed");
	}

	wrefresh(win);

	switch (main_input_loop(win)) {
		case QUIT_AND_SAVE:
			save_changes(el, fields);
			break;

		case QUIT_WITHOUT_SAVING:
			Nprint("Changes not saved.");
			break;
	}

	if (unpost_form(editor_form) != E_OK) {
		Fatal_error("unpost_form() failed");
	}

	/* TODO: These fail when more then one form page is used. */
	if (delwin(swin) == ERR) {
		/* Nprint_warn("delwin(swin) failed"); */
		/* Fatal_error("delwin() failed"); */
	}
	if (delwin(win) == ERR) {
		/* Nprint_warn("delwin(win) failed"); */
		/* Fatal_error("delwin() failed"); */
	}

	if (free_form(editor_form) != E_OK) {
		Fatal_error("free_form() failed");
	}

	for (i = 0; fields[i]; ++i) {
		if (free_field(fields[i]) != E_OK) {
			Fatal_error("free_field() failed");
		}
	}

	windows.active = MAIN_WINDOW;
}
#endif /* USE_EDITOR */


/*
 * One line edit.
 */

static INLINE void prepare_bottom_line(int start, int end)
{
	int x;
	int y = windows.max_lines - 1;


	Xmvaddch(y, start, ACS_RTEE);

	for (x = start + 1; x <= end - 1; ++x) {
		Xmvaddch(y, x, ' ');
	}

	Xmvaddch(y, end, ACS_LTEE);

	refresh();
}

static INLINE void restore_bottom_line(int start, int end)
{
	int x;
	int y = windows.max_lines - 1;


	for (x = start; x <= end; ++x) {
		Xmvaddch(y, x, ACS_HLINE);
	}

	refresh();
}

void get_string_from_bottom(const char *label_string, char **string)
{
	int i;
	int begin_y, begin_x, end_x;
	FIELD **fields;
	FORM *form;
	WINDOW *win, *swin;


	windows.active = BOTTOM_WINDOW;


	begin_x = 2;

	if (opt_display_timer) {
		end_x = windows.max_cols - 21;
	} else {
		end_x = windows.max_cols - 3;
	}

	begin_y = windows.max_lines - 1;


	prepare_bottom_line(begin_x, end_x);


	fields = xmalloc(3 * sizeof *fields);

	fields[0] = make_label(0, 0, label_string);
	fields[1] = make_field(0, strlen(label_string) + 1, *string,
		1, end_x - begin_x - 3 - strlen(label_string) - 1);

	fields[2] = NULL;

	if ((form = new_form(fields)) == NULL) {
		Fatal_error("form() failed");
	}

	/* Create windows. */
	if ((win = newwin(1, end_x - begin_x - 3, begin_y, begin_x + 2))
	== NULL) {
		Fatal_error("newwin() failed");
	}
	keypad(win, TRUE);
	nodelay(win, TRUE);

	if ((swin = derwin(win, 1, end_x - begin_x - 3, 0, 0)) == NULL) {
		Fatal_error("derwin() failed");
	}
	keypad(swin, TRUE);
	nodelay(swin, TRUE);

	if (set_form_sub(form, swin) != E_OK) {
		Fatal_error("set_form_sub() failed");
	}
	if (set_form_win(form, win) != E_OK) {
		Fatal_error("set_form_win() failed");
	}

	if (post_form(form) != E_OK) {
		Fatal_error("post_form() failed");
	}

	/* form_driver(form, REQ_END_FIELD); */

	while (1) {
		wrefresh(win);

		pos_form_cursor(form);

		i = get_key(win);

		call_driver(form, REQ_VALIDATION);

		if (i == '\n') {
			xfree(*string);
			*string = xstrdup(field_buffer(fields[1], 0));

			{	/* Trim only the trailing space. */
				char *s = *string;

				while (isspace(s[strlen(s)-1])) {
					s[strlen(s)-1]='\0';
				}

				if (! **string) {
					xfree(*string);
					*string = NULL;
				}
			}

			break;

		} else {
			call_driver(form, input_to_request(form, i));
		}
	}

	if (unpost_form(form) != E_OK) {
		Fatal_error("unpost_form() failed");
	}

	if (free_form(form) != E_OK) {
		Fatal_error("free_form() failed");
	}

	for (i = 0; fields[i]; ++i) {
		if (free_field(fields[i]) != E_OK) {
			Fatal_error("free_field() failed");
		}
	}

	if (delwin(swin) == ERR) {
		Fatal_error("delwin() failed");
	}
	if (delwin(win) == ERR) {
		Fatal_error("delwin() failed");
	}

	restore_bottom_line(begin_x, end_x);


	windows.active = MAIN_WINDOW;
}
