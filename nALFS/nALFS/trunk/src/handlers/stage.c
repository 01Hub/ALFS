/*
 *  stage.c - Handlers for <stage> and <stage>-related elements.
 * 
 *  Copyright (C) 2002-2004
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

#include <stdio.h>
#include <sys/wait.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define MODULE_NAME stage
#include <nALFS.h>

#include "handlers.h"
#include "utility.h"
#include "nprint.h"
#include "parser.h"
#include "nalfs-core.h"
#include "backend.h"
#include "options.h"

/* Forward declarations for cross-handler references */
static int environment_main(const element_s * const element);
static int variable_main(const element_s * const element);


extern char **environ;


static INLINE int set_supplementary_groups(const char *user, gid_t gid)
{
	size_t size;
	struct group *gr;
	FILE *fp;
	gid_t *list;


	/* Add user's group ID first. */
	list = xmalloc(sizeof *list);
	list[0] = gid;
	size = 1;

	/* Now check other groups if they contain user as a member. */
	if ((fp = fopen("/etc/group", "r"))) {
		/* getgrnam() is failing in chroot() */
		while ((gr = fgetgrent(fp))) {
			int i;

			for (i = 0; gr->gr_mem[i]; i++) {
				if (strcmp(gr->gr_mem[i], user) == 0) {
					size++;
					list = xrealloc(list, size * sizeof *list);
					list[size-1] = gr->gr_gid;
	
					break;
				}
			}
		}

		fclose(fp);

	} else {
		Nprint_h_warn("Unable to open /etc/group: %s",
			strerror(errno));
	}

	if (setgroups(size, list)) {
		Nprint_h_err("Unable to set supplementary group IDs: %s",
			strerror(errno));
		return -1;
	}

	xfree(list);

	return 0;
}

static INLINE int change_to_user(const char *user)
{
	struct passwd *pw;


	setpwent();

	/* getpwnam() is failing in chroot() */
	while ((pw = getpwent())) {
		if (strcmp(pw->pw_name, user) == 0) {
			break;
		}
	}

	endpwent();

	if (pw == NULL) {
		Nprint_h_err("User %s doesn't exist.", user);
		return -1;
	}

	if (set_supplementary_groups(user, pw->pw_gid)) {
		return -1;
	}

	if (setgid(pw->pw_gid)) {
		Nprint_h_err("Unable to set group ID: %s",
			strerror(errno));
		return -1;
	}

	if (setuid(pw->pw_uid)) {
		Nprint_h_err("Unable to set user ID: %s",
			strerror(errno));
		return -1;
	}

	return 0;
}

static INLINE int unset_variable(const char *variable)
{
	Nprint_h("Unsetting environment variable %s.", variable);
	unsetenv(variable);

	return 0;
}

static INLINE int append_to_variable(const char *variable, const char *value)
{
	int status = 0;
	char *old_value, *full_value = NULL;
       

	if ((old_value = getenv(variable))) {
		full_value = xstrdup(old_value);
	}
	append_str(&full_value, value);

	Nprint_h("Appending to environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, full_value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		status = -1;
	}

	xfree(full_value);

	return status;
}

static INLINE int prepend_to_variable(const char *variable, const char *value)
{
	int status = 0;
	char *old_value, *full_value = NULL;
       

	append_str(&full_value, value);

	if ((old_value = getenv(variable))) {
	        append_str(&full_value, old_value);
	}

	Nprint_h("Prepending to environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, full_value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		status = -1;
	}

	xfree(full_value);

	return status;
}

static INLINE int set_variable(const char *variable, const char *value)
{
	Nprint_h("Setting environment variable %s:", variable);
	Nprint_h("    %s", value);

	if (setenv(variable, value, 1)) {
		Nprint_h_err("Setting environment variable failed.");
		return -1;
	}

	return 0;
}

/* <stage> handler */

enum {
	STAGE_NAME,
};

static struct handler_attribute stage_attributes[] = {
	{ .name = "name", .private = STAGE_NAME },
	{ .name = NULL }
};

struct stage_data {
	char *name;
	const element_s *stageinfo;
};

/* common function used by all stage-like handlers to actually execute
   the element's children
*/

static int process_stage(const element_s * const element)
{
	struct stage_data *data = (struct stage_data *) element->handler_data;
	int status;
	pid_t pid, got_pid;

	/* if no <stageinfo> found, simply execute children */

	if (!data->stageinfo)
		return execute_children(element);

	/* found <stageinfo>, assume that a fork() is necessary */

	pid = fork();

	if (pid == 0) { /* Child. */
		status = execute_children_filtered(element, HTYPE_STAGEINFO);

		if (status)
			exit(status);

		exit(execute_children_filtered(element, ~HTYPE_STAGEINFO));

	} else if (pid == -1) { /* Error. */
		fatal_backend_error("fork() in <stage> failed.");
	}

	/* Parent. */

	if ((got_pid = waitpid(pid, &status, WUNTRACED)) == -1) {
		fatal_backend_error("waitpid() for %ld in <stage> failed.",
				    (long)pid);
	}

	if (WIFEXITED(status)) { /* Child exited normally. */
		status = WEXITSTATUS(status);

	} else if (WIFSIGNALED(status)) {
		Nprint_h_err("<stage> child (%ld) killed by signal %d%s.",
			     (long) got_pid, WTERMSIG(status),
			     WCOREDUMP(status) ? " (core dumped)" : "");
		status = -1;

	} else if (WIFSTOPPED(status)) {
		Nprint_h_err("<stage> child (%ld) stopped by signal %d.",
			     (long) got_pid, WSTOPSIG(status));
		status = -1;

	} else {
		Nprint_h_err("<stage> child (%ld) exited abnormally.",
			     (long) got_pid);
		status = -1;
	}

	return status;
}

static int stage_setup(element_s * const element)
{
	struct stage_data *data;

	if ((data = xmalloc(sizeof(struct stage_data))) == NULL)
		return 1;

	data->name = NULL;
	data->stageinfo = NULL;
	element->handler_data = data;

	return 0;
}

static void stage_free(const element_s * const element)
{
	struct stage_data *data = (struct stage_data *) element->handler_data;
	
	xfree(data->name);
	xfree(element->handler_data);
}

static int stage_attribute(const element_s * const element,
			   const struct handler_attribute * const attr,
			   const char * const value)
{
	struct stage_data *data = (struct stage_data *) element->handler_data;

	switch (attr->private) {
	case STAGE_NAME:
		if (data->name) {
			Nprint_err("<stage>: cannot specify \"name\" more than once.");
			return 1;
		}
		data->name = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int stage_valid_child(const element_s * const element,
			     const element_s * const child)
{
	struct stage_data *data = (struct stage_data *) element->handler_data;

	if (child->handler->type & HTYPE_STAGEINFO) {
		if (data->stageinfo) {
			Nprint_err("<stage>: only one <stageinfo> allowed.");
			return 0;
		}

		data->stageinfo = child;
		return 1;
	}

	return child->handler->type & (HTYPE_NORMAL |
				       HTYPE_COMMENT |
				       HTYPE_PACKAGE);
}

static int stage_main(const element_s * const element)
{
	int status;
	struct stage_data *data = (struct stage_data *) element->handler_data;

	if (*opt_be_verbose) {
		if (data->name) {
			Nprint_h("Entering new stage: %s", data->name);
		} else {
			Nprint_h("Entering new stage.");
		}
	}

	log_start_time(element);

	status = process_stage(element);

	log_end_time(element, status);

	if (*opt_be_verbose) {
		if (data->name) {
			Nprint_h("Exiting stage: %s", data->name);
		} else {
			Nprint_h("Exiting stage.");
		}
	}

	return status;
}

static char *stage_data(const element_s * const element,
			const handler_data_e data_requested)
{
	struct stage_data *data = (struct stage_data *) element->handler_data;

	/* some data elements currently supported by the <stage>
	   element are HDATA_BASE and HDATA_SHELL, which actually
	   are supplied by an HTYPE_STAGEINFO child, if it exists
	*/

	switch (data_requested) {
	case HDATA_DISPLAY_NAME:
	{
		char *display = NULL;

		append_str_format(&display, "%s%s",
				  *opt_display_stage_header ? "Enter stage: " : "",
				  data->name);

		return display;
	}
	case HDATA_BASE:
	case HDATA_SHELL:
		if (!data->stageinfo)
			return NULL;
	
		return data->stageinfo->handler->alloc_data(data->stageinfo, data_requested);
	default:
		break;
	}

	return NULL;
}

#if HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2

static int then_main(const element_s * const el)
{
	int status;

	if (*opt_be_verbose)
		Nprint_h("Executing <then> block.");

	log_start_time(el);

	status = process_stage(el);

	log_end_time(el, status);

	if (*opt_be_verbose)
		Nprint_h("<then> block complete.");

	return status;
}

static int else_main(const element_s * const el)
{
	int status;

	if (*opt_be_verbose)
		Nprint_h("Executing <else> block.");

	log_start_time(el);

	status = process_stage(el);

	log_end_time(el, status);

	if (*opt_be_verbose)
		Nprint_h("<else> block complete.");

	return status;
}

#endif /* HANDLER_SYNTAX_3_1 || HANDLER_SYNTAX_3_2 */

/* <stageinfo> handler */

enum {
	STAGEINFO_BASE,
	STAGEINFO_ROOT,
	STAGEINFO_USER,
	STAGEINFO_SHELL,
};

static const struct handler_parameter stageinfo_parameters[] = {
	{ .name = "base", .private = STAGEINFO_BASE },
	{ .name = "root", .private = STAGEINFO_ROOT },
	{ .name = "user", .private = STAGEINFO_USER },
	{ .name = NULL }
};

#if HANDLER_SYNTAX_3_2

static const struct handler_parameter stageinfo_parameters_3_2[] = {
	{ .name = "base", .private = STAGEINFO_BASE },
	{ .name = "root", .private = STAGEINFO_ROOT },
	{ .name = "user", .private = STAGEINFO_USER },
	{ .name = "shell", .private = STAGEINFO_SHELL },
	{ .name = NULL }
};

#endif

struct stageinfo_data {
	char *base;
	char *root;
	char *user;
	char *shell;
};

static int stageinfo_setup(element_s * const element)
{
	struct stageinfo_data *data;

	if ((data = xmalloc(sizeof(struct stageinfo_data))) == NULL)
		return 1;

	data->base = NULL;
	data->root = NULL;
	data->user = NULL;
	data->shell = NULL;
	element->handler_data = data;

	return 0;
}

static void stageinfo_free(const element_s * const element)
{
	struct stageinfo_data *data = (struct stageinfo_data *) element->handler_data;
	
	xfree(data->base);
	xfree(data->root);
	xfree(data->user);
	xfree(data->shell);
	xfree(element->handler_data);
}

static int stageinfo_parameter(const element_s * const element,
			       const struct handler_parameter * const param,
			       const char * const value)
{
	struct stageinfo_data *data = (struct stageinfo_data *) element->handler_data;

	switch (param->private) {
	case STAGEINFO_BASE:
		if (data->base) {
			Nprint_err("<stageinfo>: cannot specify <base> more than once.");
			return 1;
		}
		data->base = xstrdup(value);
		return 0;
	case STAGEINFO_USER:
		if (data->user) {
			Nprint_err("<stageinfo>: cannot specify <user> more than once.");
			return 1;
		}
		data->user = xstrdup(value);
		return 0;
	case STAGEINFO_ROOT:
		if (data->root) {
			Nprint_err("<stageinfo>: cannot specify <root> more than once.");
			return 1;
		}
		data->root = xstrdup(value);
		return 0;
	case STAGEINFO_SHELL:
		if (data->shell) {
			Nprint_err("<stageinfo>: cannot specify <shell> more than once.");
			return 1;
		}
		data->shell = xstrdup(value);
		return 0;
	default:
		return 1;
	}
}

static int stageinfo_valid_child(const element_s * const element,
				 const element_s * const child)
{
	(void) element;

	if (child->handler->main != environment_main) {
		return 0;
	}

	return 1;
}

static int stageinfo_main(const element_s * const element)
{
	struct stageinfo_data *data = (struct stageinfo_data *) element->handler_data;

	if (data->root) {
		Nprint_h("Changing root directory to \"%s\".", data->root);

		if (change_current_dir(data->root))
			return -1;

		if (chroot(data->root)) {
			Nprint_h_err("    %s", strerror(errno));
			return -1;
		}

		/* clear the environment after successful chroot */
		environ = NULL;
	}

	if (data->user) {
		Nprint_h("Changing to user \"%s\".", data->user);

		if (change_to_user(data->user) == -1)
			return -1;
	}

	return execute_children(element);
}

static char *stageinfo_data(const element_s * const element,
			    const handler_data_e data_requested)
{
	struct stageinfo_data *data = (struct stageinfo_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_SHELL:
		if (data->shell)
			return xstrdup(data->shell);
		break;
	case HDATA_BASE:
		if (data->base)
			return xstrdup(data->base);
		break;
	case HDATA_DISPLAY_DETAILS:
	{
		char *display = NULL;

		if (data->base)
			append_str_format(&display, "Base directory: %s\n", data->base);
		if (data->shell)
			append_str_format(&display, "Shell to use for commands: %s\n",
					  data->shell);
		if (data->root)
			append_str_format(&display, "Root directory: %s\n", data->root);
		if (data->user)
			append_str_format(&display, "User name: %s\n", data->user);

		return display;
	}
	default:
		break;
	}

	return NULL;
}


/* <environment> handler */

static int environment_valid_child(const element_s * const element,
				   const element_s * const child)
{
	(void) element;

	if (child->handler->main != variable_main) {
		return 0;
	}

	return 1;
}

static int environment_main(const element_s * const element)
{
	return execute_children(element);
}

/* <variable> handler */

enum {
	VARIABLE_NAME,
	VARIABLE_MODE,
};

static struct handler_attribute variable_attributes[] = {
	{ .name = "name", .private = VARIABLE_NAME },
	{ .name = "mode", .private = VARIABLE_MODE },
	{ .name = NULL }
};

enum variable_mode {
	VAR_SET,
	VAR_APPEND,
	VAR_PREPEND
};

struct variable_data {
	enum variable_mode mode;
	char *name;
	char *value;
};

static int variable_setup(element_s *element)
{
	struct variable_data *data;

	if ((data = xmalloc(sizeof(struct variable_data))) == NULL)
		return 1;

	data->name = NULL;
	data->value = NULL;
	data->mode = VAR_SET;
	element->handler_data = data;

	return 0;
}

static void variable_free(const element_s * const element)
{
	struct variable_data *data = (struct variable_data *) element->handler_data;
	
	xfree(data->name);
	xfree(data->value);
	xfree(element->handler_data);
}

static int variable_attribute(const element_s * const element,
			      const struct handler_attribute * const attr,
			      const char * const value)
{
	struct variable_data *data = (struct variable_data *) element->handler_data;

	switch (attr->private) {
	case VARIABLE_NAME:
		if (data->name) {
			Nprint_err("<variable>: cannot specify \"name\" more than once.");
			return 1;
		}
		data->name = xstrdup(value);
		return 0;
	case VARIABLE_MODE:
		if (data->mode != VAR_SET) {
			Nprint_err("<variable>: cannot specify \"mode\" more than once.");
			return 1;
		}
		if (strcmp(value, "append") == 0) {
			data->mode = VAR_APPEND;
			return 0;
		} else if (strcmp(value, "prepend") == 0) {
			data->mode = VAR_PREPEND;
			return 0;
		} else {
			Nprint_err("<variable>: unknown \"mode\" (%s)", value);
			return 1;
		}
	default:
		return 1;
	}
}

static int variable_content(const element_s * const element,
			    const char * const content)
{
	struct variable_data *data = (struct variable_data *) element->handler_data;

	if (strlen(content))
		data->value = xstrdup(content);

	return 0;
}

static int variable_valid_data(const element_s * const element)
{
	struct variable_data *data = (struct variable_data *) element->handler_data;

	if (!data->name) {
		Nprint_err("<variable>: \"name\" must be specified.");
		return 0;
	}

	if (!data->value &&
	    ((data->mode == VAR_APPEND) || (data->mode == VAR_PREPEND))) {
		Nprint_err("<variable>: content cannot be empty if mode is prepend or append.");
		return 0;
	}

	return 1;
}

static int variable_main(const element_s * const element)
{
	struct variable_data *data = (struct variable_data *) element->handler_data;

	switch (data->mode) {
	case VAR_SET:
		if (data->value) {
			return set_variable(data->name, data->value);
		} else {
			return unset_variable(data->name);
		}
	case VAR_APPEND:
		return append_to_variable(data->name, data->value);
	case VAR_PREPEND:
		return prepend_to_variable(data->name, data->value);
	}

	return 1;
}

static char *variable_data(const element_s * const element,
			   const handler_data_e data_requested)
{
	struct variable_data *data = (struct variable_data *) element->handler_data;

	switch (data_requested) {
	case HDATA_DISPLAY_NAME:
	{
		char *display = NULL;

		append_str_format(&display, "Variable: %s", data->name);

		return display;
	}
	case HDATA_DISPLAY_DETAILS:
	{
		char *display = NULL;

		append_str_format(&display, "Variable name: %s\n", data->name);
		switch (data->mode) {
		case VAR_SET:
			if (data->value) {
				append_str_format(&display, "New value: %s\n", data->value);
			} else {
				append_str(&display, "Variable will be cleared\n");
			}
			break;
		case VAR_APPEND:
			append_str_format(&display, "Prepend value: %s\n", data->value);
			break;
		case VAR_PREPEND:
			append_str_format(&display, "Append value: %s\n", data->value);
			break;
		}

		return display;
	}
	default:
		break;
	}

	return NULL;
}


/*
 * Handlers' information.
 */

handler_info_s HANDLER_SYMBOL(info)[] = {
#if HANDLER_SYNTAX_3_0
	{
		.name = "stage",
		.description = "Enter stage: ",
		.syntax_version = "3.0",
		.type = HTYPE_NORMAL | HTYPE_STAGE,
		.main = stage_main,
		.data = HDATA_BASE | HDATA_DISPLAY_NAME,
		.alloc_data = stage_data,
		.setup = stage_setup,
		.free = stage_free,
		.attributes = stage_attributes,
		.attribute = stage_attribute,
		.valid_child = stage_valid_child,
	},
	{
		.name = "stageinfo",
		.description = "stageinfo",
		.syntax_version = "3.0",
		.type = HTYPE_STAGEINFO,
		.main = stageinfo_main,
		.alloc_data = stageinfo_data,
		.setup = stageinfo_setup,
		.free = stageinfo_free,
		.parameters = stageinfo_parameters,
		.parameter = stageinfo_parameter,
		.valid_child = stageinfo_valid_child,
		.data =  HDATA_DISPLAY_DETAILS,
	},
	{
		.name = "environment",
		.description = "environment",
		.syntax_version = "3.0",
		.type = HTYPE_NORMAL,
		.main = environment_main,
		.valid_child = environment_valid_child,
	},
	{
		.name = "variable",
		.description = "variable",
		.syntax_version = "3.0",
		.type = HTYPE_NORMAL,
		.main = variable_main,
		.setup = variable_setup,
		.free = variable_free,
		.attributes = variable_attributes,
		.attribute = variable_attribute,
		.content = variable_content,
		.valid_data = variable_valid_data,
		.data = HDATA_DISPLAY_DETAILS | HDATA_DISPLAY_NAME,
		.alloc_data = variable_data,
	},
#endif
#if HANDLER_SYNTAX_3_1
	{
		.name = "stage",
		.description = "stage",
		.syntax_version = "3.1",
		.type = HTYPE_NORMAL | HTYPE_STAGE,
		.main = stage_main,
		.data = HDATA_BASE | HDATA_DISPLAY_NAME,
		.alloc_data = stage_data,
		.setup = stage_setup,
		.free = stage_free,
		.attributes = stage_attributes,
		.attribute = stage_attribute,
		.valid_child = stage_valid_child,
	},
	{
		.name = "then",
		.description = "then",
		.syntax_version = "3.1",
		.type = HTYPE_TRUE_RESULT | HTYPE_STAGE,
		.main = then_main,
		.data = HDATA_BASE,
		.alloc_data = stage_data,
		.setup = stage_setup,
		.free = stage_free,
		.attributes = stage_attributes,
		.attribute = stage_attribute,
		.valid_child = stage_valid_child,
	},
	{
		.name = "else",
		.description = "else",
		.syntax_version = "3.1",
		.type = HTYPE_FALSE_RESULT | HTYPE_STAGE,
		.main = else_main,
		.data = HDATA_BASE,
		.alloc_data = stage_data,
		.setup = stage_setup,
		.free = stage_free,
		.attributes = stage_attributes,
		.attribute = stage_attribute,
		.valid_child = stage_valid_child,
	},
	{
		.name = "stageinfo",
		.description = "stageinfo",
		.syntax_version = "3.1",
		.type = HTYPE_STAGEINFO,
		.main = stageinfo_main,
		.alloc_data = stageinfo_data,
		.setup = stageinfo_setup,
		.free = stageinfo_free,
		.parameters = stageinfo_parameters,
		.parameter = stageinfo_parameter,
		.valid_child = stageinfo_valid_child,
		.data =  HDATA_DISPLAY_DETAILS,
	},
	{
		.name = "environment",
		.description = "environment",
		.syntax_version = "3.1",
		.type = HTYPE_NORMAL,
		.main = environment_main,
		.valid_child = environment_valid_child,
	},
	{
		.name = "variable",
		.description = "variable",
		.syntax_version = "3.1",
		.type = HTYPE_NORMAL,
		.main = variable_main,
		.setup = variable_setup,
		.free = variable_free,
		.attributes = variable_attributes,
		.attribute = variable_attribute,
		.content = variable_content,
		.valid_data = variable_valid_data,
		.data = HDATA_DISPLAY_DETAILS | HDATA_DISPLAY_NAME,
		.alloc_data = variable_data,
	},
#endif
#if HANDLER_SYNTAX_3_2
	{
		.name = "stage",
		.description = "Enter stage: ",
		.syntax_version = "3.2",
		.type = HTYPE_NORMAL | HTYPE_STAGE,
		.main = stage_main,
		.data = HDATA_BASE | HDATA_SHELL | HDATA_DISPLAY_NAME,
		.alloc_data = stage_data,
		.setup = stage_setup,
		.free = stage_free,
		.attributes = stage_attributes,
		.attribute = stage_attribute,
		.valid_child = stage_valid_child,
	},
	{
		.name = "then",
		.description = "then",
		.syntax_version = "3.2",
		.type = HTYPE_TRUE_RESULT | HTYPE_STAGE,
		.main = then_main,
		.data = HDATA_BASE | HDATA_SHELL,
		.alloc_data = stage_data,
		.setup = stage_setup,
		.free = stage_free,
		.attributes = stage_attributes,
		.attribute = stage_attribute,
		.valid_child = stage_valid_child,
	},
	{
		.name = "else",
		.description = "else",
		.syntax_version = "3.2",
		.type = HTYPE_FALSE_RESULT | HTYPE_STAGE,
		.main = else_main,
		.data = HDATA_BASE | HDATA_SHELL,
		.alloc_data = stage_data,
		.setup = stage_setup,
		.free = stage_free,
		.attributes = stage_attributes,
		.attribute = stage_attribute,
		.valid_child = stage_valid_child,
	},
	{
		.name = "stageinfo",
		.description = "stageinfo",
		.syntax_version = "3.2",
		.type = HTYPE_STAGEINFO,
		.main = stageinfo_main,
		.setup = stageinfo_setup,
		.free = stageinfo_free,
		.alloc_data = stageinfo_data,
		.parameters = stageinfo_parameters_3_2,
		.parameter = stageinfo_parameter,
		.valid_child = stageinfo_valid_child,
		.data =  HDATA_DISPLAY_DETAILS,
	},
	{
		.name = "environment",
		.description = "environment",
		.syntax_version = "3.2",
		.type = HTYPE_NORMAL,
		.main = environment_main,
		.valid_child = environment_valid_child,
	},
	{
		.name = "variable",
		.description = "variable",
		.syntax_version = "3.2",
		.type = HTYPE_NORMAL,
		.main = variable_main,
		.setup = variable_setup,
		.free = variable_free,
		.attributes = variable_attributes,
		.attribute = variable_attribute,
		.content = variable_content,
		.valid_data = variable_valid_data,
		.data = HDATA_DISPLAY_DETAILS | HDATA_DISPLAY_NAME,
		.alloc_data = variable_data,
	},
#endif
	{
		.name = NULL
	}
};
