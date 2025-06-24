/**
 * @file hsh.c
 * @author Zack Bostock
 * @brief Main shell of Horizon64
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stddef.h>
#include <stdint.h>

#include <libc/stdio.h>
#include <libc/string.h>
#include <libc/sys.h>

#include <hsh.h>

/**
 * @brief Creates a new exec cmd
 */
#define CREATE_EXEC_CMD(_name)                                              \
    EXEC_CMD *_name = (EXEC_CMD *)(malloc(sizeof(EXEC_CMD)));               \
    if (!_name) {                                                           \
        panic("hsh: Failed to allocate memory for new EXEC_CMD\n");         \
    }                                                                       \
    memset(_name, 0, sizeof(EXEC_CMD));                                     \
    _name->type = EXEC;


/**
 * @brief Creates a new redir cmd
 */
#define CREATE_REDIR_CMD(_name, file, efile, mode, fd)                      \
    REDIR_CMD *_name = (REDIR_CMD *)(malloc(sizeof(REDIR_CMD)));            \
    if (!_name) {                                                           \
        panic("hsh: Failed to allocate memory for new REDIR_CMD\n");        \
    }                                                                       \
    memset(_name, 0, sizeof(REDIR_CMD));                                    \
    _name->type = REDIR;                                                    \
    _name->cmd = subcmd;                                                    \
    _name->file = file;                                                     \
    _name->efile = efile;                                                   \
    _name->mode = mode;                                                     \
    _name->fd = fd;


/**
 * @brief Creates a new pipe cmd
 */
#define CREATE_PIPE_CMD(_name, left, right)                                 \
    PIPE_CMD *_name = (PIPE_CMD *)(malloc(sizeof(PIPE_CMD)));               \
    if (!_name) {                                                           \
        panic("hsh: Failed to allocate memory for new PIPE_CMD\n");         \
    }                                                                       \
    memset(_name, 0, sizeof(PIPE_CMD));                                     \
    _name->type = PIPE;                                                     \
    _name->left = left;                                                     \
    _name->right = right;

/**
 * @brief Creates a new list cmd
 */
#define CREATE_LIST_CMD(_name, left, right)                                 \
    LIST_CMD *_name = (LIST_CMD *)(malloc(sizeof(LIST_CMD)));               \
    if (!_name) {                                                           \
        panic("hsh: Failed to allocate memory for new LIST_CMD\n");         \
    }                                                                       \
    memset(_name, 0, sizeof(LIST_CMD));                                     \
    _name->type = LIST;                                                     \
    _name->left = left;                                                     \
    _name->right = right;

/**
 * @brief Creates a new back cmd
 */
#define CREATE_BACK_CMD(_name, subcmd)                                      \
    BACK_CMD *_name = (BACK_CMD *)(malloc(sizeof(BACK_CMD)));               \
    if (!_name) {                                                           \
        panic("hsh: Failed to allocate memory for new BACK_CMD\n");         \
    }                                                                       \
    memset(_name, 0, sizeof(BACK_CMD));                                     \
    _name->type = BACK;                                                     \
    _name->cmd = subcmd;

/**
 * @brief Makes an exec cmd
 *
 * @return CMD* New exec cmd
 */
static inline CMD *exec_cmd() {
    EXEC_CMD *cmd = (EXEC_CMD *)(malloc(sizeof(EXEC_CMD)));
    memset(cmd, 0, sizeof(EXEC_CMD));
    cmd->type = EXEC;
    return (CMD *) cmd;
}

/**
 * @brief Construct a new redir cmd
 *
 * @param subcmd Subcommand
 * @param file File to redirect into
 * @param efile External file
 * @param mode Mode to redirect
 * @param fd File descriptor
 * @param CMD* new command
 */
static inline CMD *redir_cmd(CMD *subcmd, char *file, char *efile, int mode,
                             int fd) {
    REDIR_CMD *cmd = (REDIR_CMD *)(malloc(sizeof(REDIR_CMD)));
    memset(cmd, 0, sizeof(REDIR_CMD));
    cmd->type = REDIR;
    cmd->cmd = subcmd;
    cmd->file = file;
    cmd->efile = efile;
    cmd->mode = mode;
    cmd->fd = fd;
    return (CMD *) cmd;
}

/**
 * @brief Construct new pipe cmd
 *
 * @param left Left binary of the pipe
 * @param right Right binary of the pipe
 * @return CMD* new command
 */
static inline CMD *pipe_cmd(CMD *left, CMD *right) {
    PIPE_CMD *cmd = (PIPE_CMD *)(malloc(sizeof(PIPE_CMD)));
    memset(cmd, 0, sizeof(PIPE_CMD));
    cmd->type = PIPE;
    cmd->left = left;
    cmd->right = right;
    return (CMD *) cmd;
}

/**
 * @brief Constructs new list cmd
 *
 * @param left Left list command
 * @param right Right list command
 * @return CMD* new command
 */
static inline CMD *list_cmd(CMD *left, CMD *right) {
    LIST_CMD *cmd = (LIST_CMD *)(malloc(sizeof(LIST_CMD)));
    memset(cmd, 0, sizeof(LIST_CMD));
    cmd->type = LIST;
    cmd->left = left;
    cmd->right = right;
    return (CMD *) cmd;
}

/**
 * @brief Constructs new back cmd
 *
 * @param subcmd Subcommand of this back command
 * @return CMD* new command
 */
static inline CMD *back_cmd(CMD *subcmd) {
    BACK_CMD *cmd = (BACK_CMD *)(malloc(sizeof(BACK_CMD)));
    memset(cmd, 0, sizeof(BACK_CMD));
    cmd->type = BACK;
    cmd->cmd = subcmd;
    return (CMD *) cmd;
}

/**
 * @brief Runs a command
 *
 * @param cmd Command type to run
 */
void run_cmd(CMD *cmd) {
    int p[2] = {0};
    char pathname[CMD_MAX_LEN] = {0};
    // BACK_CMD *bcmd;
    EXEC_CMD *ecmd;
    LIST_CMD *lcmd;
    PIPE_CMD *pcmd;
    // REDIR_CMD *rcmd;

    if (cmd == 0) {
        exit(1);
    }

    switch (cmd->type) {
        default:
            panic("runcmd");

        case EXEC:
            ecmd = (EXEC_CMD *) cmd;
            if (ecmd->argv[0] == 0) {
                exit(1);
            }
            strcpy(pathname, "");
            if (pathname[0] != '/') {
                strcpy(pathname, "/bin/");
            }

            strcat(pathname, ecmd->argv[0]);
            libc_log("hsh: start to execute process for current task\n");
            if (execv(pathname, ecmd->argv) < 0) {
                fprintf(STDERR, "exec \"%s\" has failed\n", ecmd->argv[0]);
            }
            break;
        case PIPE:
            pcmd = (PIPE_CMD *) cmd;
            if (pipe(p) < 0) {
                perror("pipe");
                panic("pipe");
            }
            libc_log("hsh: start to fork pipe process for left and right tasks\n");
            if (fork() == 0){
                /* Child process */
                close(p[1]);
                dup3(STDIN, 0, p[0]);
                run_cmd(pcmd->left);
                close(p[0]);

                /* Should never reach here */
                exit(1);
            }
            if (fork() == 0){
                /* Child process */
                close(p[1]);
                dup3(STDIN, 0, p[0]);
                run_cmd(pcmd->right);
                close(p[0]);

                /* Should never reach here */
                exit(1);
            }
            /* Parents should reach here */
            wait(-1);
            wait(-1);
            close(p[0]);
            close(p[1]);
            break;
        case LIST:
            lcmd = (LIST_CMD *) cmd;
            if (fork() == 0) {
                /* Child process */
                run_cmd(lcmd->left);
            }
            wait(-1);
            run_cmd(lcmd->right);
            break;
    }
    exit(0);
}

/**
 * @brief Get the command from the command line
 *
 * @param buf Buffer which houses the command
 * @param count Number of commands
 * @return int 0 if success, -1 if failure
 */
int get_cmd(char *buf, int count) {
    write(STDOUT, CMD_PROMPT, strlen(CMD_PROMPT));
    memset(buf, 0, count);
    for (int i = 0;;) {
        if (read(STDIN, &buf[i], 1) != 1) {
            break;
        }

        if (buf[i] == '\b') {
            if (i > 0) {
                buf[i - 1] = '\0';
                i--;
            }
            buf[i] = '\0';
            continue;
        }

        if (i >= count - 1 || buf[i] == (char)(EOF)) {
            break;
        } else if (buf[i] == '\n') {
            buf[i] = '\0';
            break;
        }
        i++;
    }
    if (buf[0] == (char)(EOF)) {
        return -1;
    }
    return 0;
}

/**
 * @brief Helper to write an error message
 *
 * @param msg Message to write
 */
void hsh_error(const char *msg) {
    char buf[128] = {0};
    strcpy(buf, "hsh: ");
    strcat(buf, msg);
    fprintf(STDERR, buf);
}


int get_token(char **ps, char *es, char **q, char **eq) {
    char *s;
    int ret;

    s = *ps;

    while (s < es && strchr(whitespace, *s)) {
        s++;
    }
    if (1) {
        *q = s;
    }
    ret = *s;
    switch (*s) {
        case 0:
            break;
        case '|':
        case '(':
        case ')':
        case ';':
        case '&':
        case '<':
            s++;
            break;
        case '>':
            s++;
            if (*s == '>') {
                ret = '+';
                s++;
            }
            break;
        default:
            ret = 'a';
            while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s)) {
                s++;
            }
            break;
    }

    if (eq) {
        *eq = s;
    }

    while (s < es && strchr(whitespace, *s)) {
        s++;
    }

    *ps = s;
    return ret;
}

int peek(char **ps, char *es, char *toks) {
    char *s;

    s = *ps;
    while (s < es && strchr(whitespace, *s)) {
        s++;
    }

    *ps = s;
    return *s && strchr(toks, *s);
}

/**
 * @brief Nul-terminate all strings
 *
 * @param cmd Command to nul-terminate
 * @return CMD* Nul-terminated command
 */
CMD *nul_terminate(CMD *cmd) {
    BACK_CMD *bcmd;
    EXEC_CMD *ecmd;
    LIST_CMD *lcmd;
    PIPE_CMD *pcmd;
    REDIR_CMD *rcmd;

    if (!cmd) {
        return 0;
    }

    switch (cmd->type) {
        case EXEC:
            ecmd = (EXEC_CMD *) cmd;
            int i = 0;
            while (ecmd->eargv[i]) {
                ecmd->eargv[i] = NULL;
                i++;
            }
            break;
        case REDIR:
            rcmd = (REDIR_CMD *) cmd;
            nul_terminate(rcmd->cmd);
            rcmd->efile = NULL;
            break;
        case PIPE:
            pcmd = (PIPE_CMD *) cmd;
            nul_terminate(pcmd->left);
            nul_terminate(pcmd->right);
            break;
        case LIST:
            lcmd = (LIST_CMD *) cmd;
            nul_terminate(lcmd->left);
            nul_terminate(lcmd->right);
            break;
        case BACK:
            bcmd = (BACK_CMD *) cmd;
            nul_terminate(bcmd->cmd);
            break;
    }
    return cmd;
}

CMD *parse_exec(char **ps, char *es) {
    char *q;
    char *eq;
    int tok;
    int argc;
    EXEC_CMD *cmd;
    CMD *ret;

    if (peek(ps, es, "(")) {
        return parse_block(ps, es);
    }

    ret = exec_cmd();
    cmd = (EXEC_CMD *) ret;

    argc = 0;
    ret = parse_redir(ret, ps, es);
    while (!peek(ps, es, "|)&;")) {
        if ((tok = get_token(ps, es, &q, &eq)) == 0) {
            break;
        }
        if (tok != 'a') {
            hsh_error("parse_exec: syntax error\n");
        }

        cmd->argv[argc] = q;
        cmd->eargv[argc] = eq;
        argc++;
        if (argc >= MAXARGS) {
            hsh_error("parse_exec: too many arguments\n");
        }
        ret = parse_redir(ret, ps, es);
    }

    cmd->argv[argc] = NULL;
    cmd->eargv[argc] = NULL;
    return ret;
}

CMD *parse_block(char **ps, char *es) {
    CMD *cmd;
    if (!peek(ps, es, "(")) {
        hsh_error("parse_block\n");
    }
    get_token(ps, es, 0, 0);
    cmd = parse_line(ps, es);
    if (!peek(ps, es, ")")) {
        panic("parse_block: syntax error: missing )\n");
    }
    get_token(ps, es, 0, 0);
    cmd = parse_redir(cmd, ps, es);
    return cmd;
}

CMD *parse_redir(CMD *cmd, char **ps, char *es) {
    int tok;
    char *q;
    char *eq;
    while (peek(ps, es, "<>")) {
        tok = get_token(ps, es, 0, 0);
        if (get_token(ps, es, &q, &eq) != 'a') {
            hsh_error("redir: missing file for redirection\n");
        }

        /* TODO: do file redirection */
        switch (tok) {
            case '<':
                break;
            case '>':
                break;
            case '+':
                break;
        }
    }
    return cmd;
}

CMD *parse_pipe(char **ps, char *es) {
    CMD *cmd = parse_exec(ps, es);
    if (peek(ps, es, "|")) {
        get_token(ps, es, 0, 0);
        cmd = pipe_cmd(cmd, parse_pipe(ps, es));
    }

    return cmd;
}

CMD *parse_line(char **ps, char *es) {
    CMD *cmd = parse_pipe(ps, es);
    while (peek(ps, es, "&")) {
        get_token(ps, es, 0, 0);
        cmd = back_cmd(cmd);
    }
    if (peek(ps, es, ";")) {
        get_token(ps, es, 0, 0);
        cmd = list_cmd(cmd, parse_line(ps, es));
    }
    return cmd;
}

CMD *parse_cmd(char *s) {
    char *es;
    CMD *cmd;

    es = s + strlen(s);
    cmd = parse_line(&s, es);
    peek(&s, es, "");
    if (s != es) {
        hsh_error("parse_cmd: syntax error\n");
    }
    nul_terminate(cmd);
    return cmd;
}

int main() {
    printf("Shell started...\n");
    char *buf = (char *)(malloc(CMD_MAX_LEN));
    if (!buf) {
        panic("hsh: failed to allocate memory for the command buffer\n");
    }

    /* TODO: Need to make sure three file descriptors are open */

    /* Read and run the inputted commands */
    while (get_cmd(buf, CMD_MAX_LEN) >= 0) {
        if (!strncmp(buf, "cd", sizeof("cd"))) {
            /* Change directory must be called by the parent */
            if (buf[strlen(buf) - 1] == '\n') {
                /* remove the \n */
                buf[strlen(buf) - 1] = '\0';
            }
            if (chdir(buf + 3) < 0) {
                fprintf(STDERR, "hsh: cd: %s: no such file or directory\n", buf + 3);
            }
            continue;
        } else if (!strncmp(buf, "mem", sizeof("mem"))) {
            if (meminfo() < 0) {
                hsh_error("mem: cannot display memory usage information\n");
            }
            continue;
        } else if (!strncmp(buf, "lspci", sizeof("lspci"))) {
            if (runcmd(buf) < 0) {
                fprintf(STDERR, "lspci: cannot list pci devices\n");
            }
            continue;
        }

        if (buf[0] == 0) {
            continue;
        }

        if (fork() == 0) {
            /* Child process */
            run_cmd(parse_cmd(buf));
            exit(0);
        }
        wait(-1);
    }
    hsh_error("exit: ending shell\n");
    exit(0);

    /* Should never reach here */
    return 0;
}