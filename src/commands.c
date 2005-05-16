/* $Id$
 * -------------------------------------------------------
 * Copyright (C) 2003-2005 Tommi Saviranta <wnd@iki.fi>
 *	(C) 2002 Lee Hardy <lee@leeh.co.uk>
 * -------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <string.h>
#include <stdio.h>
#include "commands.h"
#include "common.h"



static struct commandhash *cmd_hash[MAX_CMD];
static int command_hash(const char *p);
void command_add(const char *p, int cmdvalue);



/* The struct for the command hash table. */
struct commandhash {
	const char		*cmd;
	int			cmdvalue;
	struct commandhash	*next;
};



/* The struct for the table below. */
struct commandaddstruct {
	char	*cmd;
	int	mask;
};



/*
 * The table of commands that we need to add at startup, and their relevant
 * bitmasks, as defined in commands.h.
 */
static struct commandaddstruct cmd_add_table[] =
{
	/* command,		bitmask,	*/
	{"PING",		CMD_PING,	},
	{"PONG",		CMD_PONG,	},
	{"MODE",		CMD_MODE,	},
	{"NICK",		CMD_NICK,	},
	{"NOTICE",		CMD_NOTICE,	},
	{"KICK",		CMD_KICK,	},
	{"JOIN",		CMD_JOIN,	},
	{"PART",		CMD_PART,	},
	{"TOPIC",		CMD_TOPIC,	},
	{"KILL",		CMD_KILL,	},
	{"PRIVMSG",		CMD_PRIVMSG,	},
	{"QUIT",		CMD_QUIT,	},
	{NULL,			CMD_NONE,	}
};



/*
 * Hashes a command to a value.
 */
static int
command_hash(const char *p)
{
	int	hash_val = 0;

	while (*p) {
		hash_val += ((int) (*p) & 0xDF);
		p++;
	}

	return (hash_val % MAX_CMD);
} /* static int command_hash(const char *p) */



/*
 * Searches for a command in the command hash table.
 */
int
command_find(char *p)
{
	struct commandhash	*ptr;
	int			cmdindex;
	
	cmdindex = command_hash(p);
	
	for (ptr = cmd_hash[cmdindex]; ptr; ptr = ptr->next) {
		if (xstrcasecmp(p, ptr->cmd) == 0) {
			return ptr->cmdvalue;
		}
	}

	return 0;
} /* int command_find(char *p) */



/*
 * Adds a command to the command hash table.
 */
void
command_add(const char *cmd, int cmdvalue)
{
	struct commandhash	*ptr;
	struct commandhash	*temp_ptr;
	struct commandhash	*last_ptr = NULL;
	int			cmdindex;

	cmdindex = command_hash(cmd);

	/* command exists */
	for (temp_ptr = cmd_hash[cmdindex]; temp_ptr;
			temp_ptr = temp_ptr->next) {
		if (xstrcasecmp(cmd, temp_ptr->cmd) == 0) {
			return;
		}

		last_ptr = temp_ptr;
	}

	ptr = (struct commandhash *) xcalloc(1, sizeof(struct commandhash));

	ptr->cmd = cmd;
	ptr->cmdvalue = cmdvalue;

	if (last_ptr) {
		last_ptr->next = ptr;
	} else {
		cmd_hash[cmdindex] = ptr;
	}
} /* void command_add(const char *cmd, int cmdvalue) */



/*
 * Walks the commandsadd table and adds all the entries.
 */
void
command_setup(void)
{
	int	i;

	for (i = 0; i < MAX_CMD; i++) {
		cmd_hash[i] = NULL;
	}
	
	for (i = 0; cmd_add_table[i].cmd; i++) {
		command_add(cmd_add_table[i].cmd, cmd_add_table[i].mask);
	}
} /* void command_setup(void) */



/*
 * Free memore allocated by command-hash.
 */
void
command_free(void)
{
	int	i;
	struct commandhash	*ptr;
	struct commandhash	*next = NULL;

	for (i = 0; i < MAX_CMD; i++) {
		for (ptr = cmd_hash[i]; ptr != NULL; ptr = next) {
			next = ptr->next;
			xfree(ptr);
		}
	}
} /* void command_free(void) */
