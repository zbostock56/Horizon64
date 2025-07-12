/**
 * @file hsh.h
 * @author Zack Bostock
 * @brief Information pertaining to Horizon64's shell
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

/* ---------------------------- LITERAL CONSTANTS --------------------------- */
#define EXEC        (1)
#define REDIR       (2)
#define PIPE        (3)
#define LIST        (4)
#define BACK        (5)

#define MAXARGS     (10)
#define CMD_MAX_LEN (100)
#define CMD_PROMPT  " \033[32m$ \033[0m"

static char whitespace[] = " \t\r\b\v";
static char symbols[] = "<|<&;()";

typedef struct {
    int type;
} CMD;

typedef struct {
    int type;
    char *argv[MAXARGS];
    char *eargv[MAXARGS];
} EXEC_CMD;

typedef struct {
    int type;
    CMD *cmd;
    char *file;
    char *efile;
    int mode;
    int fd;
} REDIR_CMD;

typedef struct {
    int type;
    CMD *left;
    CMD *right;
} PIPE_CMD;

typedef struct {
    int type;
    CMD *left;
    CMD *right;
} LIST_CMD;

typedef struct {
    int type;
    CMD *cmd;
} BACK_CMD;


/* -------------------------------- GLOBALS --------------------------------- */

/* --------------------------------- MACROS --------------------------------- */

/* --------------------------- INTERNALLY DEFINED --------------------------- */
void run_cmd(CMD *cmd);
int get_cmd(char *buf, int count);
void hsh_error(const char *msg);
int get_token(char **ps, char *es, char **q, char **eq);
int peek(char **ps, char *es, char *toks);
CMD *nul_terminate(CMD *cmd);
CMD *parse_exec(char **ps, char *es);
CMD *parse_block(char **ps, char *es);
CMD *parse_redir(CMD *cmd, char **ps, char *es);
CMD *parse_pipe(char **ps, char *es);
CMD *parse_line(char **ps, char *es);
CMD *parse_cmd(char *s);
