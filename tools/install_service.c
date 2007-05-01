/* Initng, a next generation sysvinit replacement.
 * Copyright (C) 2005 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */
#define TRUE 1
#define FALSE 0
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>

#include <initng-paths.h>

/*#define DEBUG */

#undef D_
#ifdef DEBUG
#define D_(fmt,...) printf(fmt, ## __VA_ARGS__)
#else
#define D_(fmt,...)
#endif

/*the string "blah" must be the FIRST parameter, but his improves perf a little bit */
#define MATCH(PATTERN, LINE) (strncmp(PATTERN, skip_spaces(LINE), sizeof(PATTERN) - 1) == 0)
#define LEN(STR) (sizeof(STR) - 1)

#ifndef INITNG_ROOT
#define INITNG_ROOT "/etc/initng"
#endif

static char **path;

/* for the latter paths, a trailing '/' is required ! */
static const char *additional_paths[] = {
    "/bin/",
    "/sbin/",
    "/usr/bin/",
    "/usr/sbin/",
    "/usr/local/bin/",
    "/usr/local/sbin/",
    NULL
};

struct s_overlay
{
    char const *opt;
    char const *value;
} overlay[1001];

static struct s_overlay const STATIC_OVERLAYS[] = {
    {"/lib/initng", INITNG_PLUGIN_DIR},
    {"/etc/initng", INITNG_ROOT},
    {0, 0}
};

static char *at_default_path = NULL;

/* #define MAX_AT_DEF_PATHS 100 */
static char **at_default_paths = NULL;

/* this program will parse all .i files in installation
   setting distribution dependent values on install time */

#define LINE_LEN 1000

#define TRIM_END(x) { \
	/* skip ending spaces */ \
	int t=strlen(x); \
	while((x)[t-1]==' ') \
	    (x)[t-1]='\0'; }

#define TRIM_BEGINNING(x) { \
	while((x)[0]==' ' || (x)[0]=='\t') \
	    (x)++; }

static void overlay_add(char *opt, char *value)
{
    int i = 0;


    /* step the filled in */
    while (i < 1000 && overlay[i].opt)
        i++;

    TRIM_END(opt);
    TRIM_END(value);

    D_("adding \"%s\" == \"%s\" on %i\n", opt, value, i);

    overlay[i].opt = opt;
    overlay[i].value = value;
}

static void load_overlay_file(const char *filename)
{
    char line[LINE_LEN];
    FILE *f;

    f = fopen(filename, "r");

    if (!f)
    {
        D_("overlay file \"%s\" not found.\n", filename);
        return;
    }

    /* endless loop */
    while (1)
    {
        int opt_len = 0;
        char *opt = NULL;
        char *value = NULL;
        char *tmp;

        /* get the line */
        /* make sure this is not end of file */
        if (fgets(line, LINE_LEN, f) == 0 || feof(f))
            break;

        tmp = line;

        /* skip leading spaces */
        TRIM_BEGINNING(tmp);

        /* skip these lines */
        if (!tmp[0] || tmp[0] == '#' || tmp[0] == '\n')
            continue;

        /* count the lenght to the '=' char */
        while (tmp[opt_len] && tmp[opt_len] != '=' && tmp[opt_len] != '\n')
            opt_len++;

        /* make sure we did not reach end */
        if (!tmp[opt_len] || tmp[opt_len] != '=')
            continue;

        /*D_("option len found = %i : %s\n", opt_len, tmp); */

        opt = strndup(tmp, opt_len);
        tmp = tmp + opt_len + 1;
        opt_len = 0;


        /* skip spaces */
        TRIM_BEGINNING(tmp);
        if (!tmp[0])
            continue;

        /* count the value lenght */
        while (tmp[opt_len] && tmp[opt_len] != '\n')
            opt_len++;

        value = strndup(tmp, opt_len);

        if (opt && value)
            overlay_add(opt, value);
    }

    fclose(f);
}

static int _is_file(const char *filename, int executable)
{
    struct stat tmp;

    D_("Trying, with file \"%s\":\n", filename);
    if (stat(filename, &tmp) == 0 && S_ISREG(tmp.st_mode))
    {
        if (executable)
        {
            if (!
                (tmp.
                 st_mode & (S_IXUSR | S_IXGRP | S_IXOTH | S_ISUID | S_ISGID |
                            S_ISVTX)))
            {
                D_("Found it, but not an executable\n");
                return (FALSE);
            }
        }
        D_("Found it.\n");
        return (TRUE);
    }
    D_("Did not find it.\n");
    return (FALSE);
}

#define is_file(FILENAME) _is_file(FILENAME, 0)
#define is_exec(FILENAME) _is_file(FILENAME, 1)

static const char *probe_distribution(void)
{
    D_("probe_distribution\n");

    /* Give Linspire precedence over Debian because Linspire has
     * the /etc/debian_version file also.
     */

    if (is_file("/etc/linspire-version"))
        return ("linspire");
    if (is_file("/etc/debian_version"))
        return ("debian");
    if (is_file("/etc/gentoo-release") || is_file("/etc/pardus-release"))
        return ("gentoo");
    if (is_file("/etc/fedora-release"))
        return ("fedora");
    if (is_file("/etc/pld-release"))
        return ("pld");
    if (is_file("/etc/pingwinek-release"))
        return ("pingwinek");
    if (is_file("/etc/altlinux-release"))
        return ("altlinux");
    if (is_file("/etc/mandriva-release") || is_file("/etc/pclinuxos-release"))
        return ("mandriva");
    if (is_file("/etc/arch-release"))
        return ("arch");
    if (is_file("/etc/slackware-version"))
        return ("slackware");
    if (is_file("/etc/SuSE-release"))
        return ("suse");
    if (is_file("/etc/kanotix-version"))
        return ("kanotix");
    if (is_file("/etc/lfs-release"))
        return ("lfs");
    if (is_file("/etc/sourcemage-release"))
        return ("sourcemage");
    if (is_file("/etc/enlisy-release"))
        return ("enlisy");

    D_("Probe failed\n");
    return ("unknown");
}

/* this should split a string into an "argv" like array using char delim */
/* dont use this any more, due to some obscure memory mgmt bug! 
   struct my_ll
   {
   char *data;
   struct my_ll *next;
   }; */

/* libc provides strtok, but they said, i should not use it */
static char **str_split(const char *in, char delim)
{
    /*struct my_ll *head;
       struct my_ll *current; */
    int num_tokens = 1;
    char *tmp;
    char **retval;

    /* count the number of tokens. */
    /* there is at least one token plus one for each delim char found */
    /* it doesn't matter if we allocate a bit more mem than required, eg for :: */
    for (tmp = (char *) in; *tmp; tmp++)
        if (*tmp == ':')
            num_tokens++;

    retval = malloc(sizeof(char *) * (num_tokens + 1));

    /* there may be delims in front of in */
    while (*in == delim)
        in++;

    tmp = (*retval = strdup(in));

    num_tokens = 1;

    while ((tmp = strchr(tmp, delim)))
    {
        *tmp = '\0';
        tmp++;
        while (*tmp == delim)
            tmp++;                          /*skip additional delims */
        if (*tmp == '\0')
            break;                          /*in case a delim was at the end of in */
        /* ok, here we found a new token */
        retval[num_tokens++] = tmp;
    }
    retval[num_tokens] = NULL;

    D_("got %d tokens\n", num_tokens);
    return retval;
}

/* This should find string like @mknod@ and return a pointer to it if found,
 * otherwise return NULL */

static const char *find_at_phrases(const char *in)
{
    const char *first;
    const char *second;
    const char *tmp;

    if (!(first = strchr(in, '@')))
        return NULL;                        /* no '@' */
    while (1)
    {
        if (!(second = strchr(first + 1, '@')))
            return NULL;                    /* only one '@' */

        for (tmp = first + 1; tmp < second; tmp++)
            if (*tmp == ' ' || *tmp == '\t')
            {
                /* whitespace in between: no match */
                first = second;
                continue;
            }
        return first;
    }
}

static int parse_phrase_no_colon(FILE * stream, const char *fullphrase,
                                 int def_path_no)
{
#define FILENAME_LEN 1000
    char filename[FILENAME_LEN];
    const char *tmp;
    const char *phrase;
    int i;
    int len;

    D_("ppnc: fp=\"%s\", i=%d\n", fullphrase, def_path_no);
    if (!*fullphrase)
    {
        fputc('@', stream);
        return 0;                           /* @@ => @ translation, no looping required */
    }

    /* handle some special phrases */
    for (i = 0; STATIC_OVERLAYS[i].opt; ++i)
    {
        if (strcmp(fullphrase, STATIC_OVERLAYS[i].opt) == 0)
        {
            printf("  **\t\"@%s@\" set by overlay to \"%s\".\n", fullphrase,
                   STATIC_OVERLAYS[i].value);
            fputs(STATIC_OVERLAYS[i].value, stream);
            return (0);
        }
    }

    /*    if (!path)
       return; */
    if (!(phrase = strrchr(fullphrase, '/')))
        phrase = fullphrase;
    else
        phrase++;

    if (phrase != fullphrase)
    {
        /* this means fullphrase contains an / */
        if (is_exec(fullphrase))
        {
            fputs(fullphrase, stream);
            return 0;                       /* found filename in supplied Path => no looping req'd */
        }
    }

    /* compare with all overlays */
    i = 0;
    while (overlay[i].opt)
    {
        if (strcasecmp(fullphrase, overlay[i].opt) == 0)
        {
            printf("  **\t\"@%s@\" set by overlay to \"%s\".\n", fullphrase,
                   overlay[i].value);
            fputs(overlay[i].value, stream);
            return (0);
        }
        i++;
    }

    /* first, search the path */
    i = 0;
    while (path[i])
    {
        memset(filename, 0, sizeof filename);
        strncpy(filename, path[i], FILENAME_LEN - 2);
        len = strlen(filename);
        if (filename[len - 1] != '/')
        {
            filename[len] = '/';
            filename[len + 1] = '\0';
        }
        strncat(filename, phrase, FILENAME_LEN - 1 - strlen(filename));
        if (is_exec(filename))
        {
            fputs(filename, stream);
            return 0;                       /* found filename in Path => no looping req'd */
        }
        i++;
    }
    /* then, search additional paths */
    i = 0;
    while (additional_paths[i])
    {
        strncpy(filename, additional_paths[i], FILENAME_LEN - 1);
        strncat(filename, phrase,
                FILENAME_LEN - 1 - strlen(additional_paths[i]));
        if (is_exec(filename))
        {
            fputs(filename, stream);
            return 0;                       /* found filename in Path => no looping req'd */
        }
        i++;
    }
    /* here we neither found it in the path, nor in the additional paths */
    if (def_path_no == -1)
        return 1;                           /* this is from a coloned search: the first bastard wasn't found, try the next */
    fprintf(stderr, "  **\tWARNING: No executable found for \"%s\", ",
            phrase);

    if (fullphrase != phrase)
    {
        fprintf(stderr, "  **\tusing full path \"%s\"\n", fullphrase);
        fputs(fullphrase, stream);
        /* return 0;  full path specified => no looping req'd */
    }
    else if (at_default_path)
    {
        tmp = at_default_paths[def_path_no];
        fprintf(stderr, "  **\tusing supplied default path \"%s\"\n", tmp);
        fputs(tmp, stream);
        if (tmp[strlen(tmp) - 1] != '/')
            fputc('/', stream);
        fputs(phrase, stream);
        if (at_default_paths[def_path_no + 1])
            return 1;                       /* next default path exits, so looping is required */
        /* return 0 */
    }
    else                                    /* at_default_path == NULL && fullphrase == phrase */
    {
        fprintf(stderr, "  **\tusing builtin default path \"/usr/sbin/\"\n");
        fputs("/usr/sbin/", stream);
        fputs(phrase, stream);
        /* no default path(s) so no looping */
    }
    /* exit(1) ??? */
    return 0;



#if 0
    tmp = path;
    while (1)
    {
        i = 0;
        while (*tmp != ':' && *tmp != '\0' && i < FILENAME_LEN - 1)
            filename[i++] = *(tmp++);
        if (*tmp)
            tmp++;                          /* skip ':' */
        if (filename[i] != '/')
            filename[i++] = '/';
        filename[i] = '\0';
        strncat(filename, phrase, FILENAME_LEN - 1 - i);
        /* TODO: check if executable */
        if (is_exec(filename))
        {
            fputs(filename, stream);
            return 0;                       /* found filename in Path => no looping req'd */
        }
        if (!*tmp)
        {
            /* here we passed all paths in PATH */
            i = 0;
            while (additional_paths[i])
            {
                strncpy(filename, additional_paths[i], FILENAME_LEN - 1);
                strncat(filename, phrase,
                        FILENAME_LEN - 1 - strlen(additional_paths[i]));
                if (is_exec(filename))
                {
                    fputs(filename, stream);
                    return 0;               /* found filename in Path => no looping req'd */
                }
                i++;
            }
            if (def_path_no == -1)
                return 1;                   /* this is from a coloned search: the first bastard wasn't found, try the next */
            fprintf(stderr, "  **\tWARNING: No executable found for \"%s\", ",
                    phrase);
            if (fullphrase != phrase)
            {
                fprintf(stderr, "  **\tusing full path \"%s\"\n", fullphrase);
                fputs(fullphrase, stream);
                /* return 0;  full path specified => no looping req'd */
            }
            else if (at_default_path)
            {
                tmp = at_default_paths[def_path_no];
                fprintf(stderr, "  **\tusing supplied default path \"%s\"\n",
                        tmp);
                fputs(tmp, stream);
                if (tmp[strlen(tmp) - 1] != '/')
                    fputc('/', stream);
                fputs(phrase, stream);
                if (at_default_paths[def_path_no + 1])
                    return 1;               /* next default path exits, so looping is required */
                /* return 0 */
            }
            else                            /* at_default_path == NULL && fullphrase == phrase */
            {
                fprintf(stderr,
                        "  **\tusing builtin default path \"/usr/sbin/\"\n");
                fputs("/usr/sbin/", stream);
                fputs(phrase, stream);
                /* no default path(s) so no looping */
            }
            /* exit(1) ??? */
            return 0;
        }
    }
#endif
}

static int parse_phrase(FILE * stream, const char *fullphrase,
                        int def_path_no)
{
    char **phrases;

    /* char* tmp; */
    int i = 0;
    int rv;

    D_("pp: i=%d; called with %s\n", def_path_no, fullphrase);
#if 0
    phrases[0] = strdup(fullphrase);

    while ((tmp = strchr(phrases[i], ':')))
    {
        *tmp = '\0';
        phrases[++i] = tmp + 1;
        if (i == 98)
            break;
    }
    phrases[i + 1] = NULL;
#endif
    phrases = str_split(fullphrase, ':');
    if (def_path_no == 0)
    {
        for (i = 0; phrases[i]; i++)
        {
            D_("doing ppnc(%s)\n", phrases[i]);
            if (parse_phrase_no_colon(stream, phrases[i], -1) == 0)
            {
                free(phrases[0]);
                return 0;                   /* found it */
            }
        }
        /*xxx */
        /* here = bad: all executables have not been found ! */
    }

    rv = parse_phrase_no_colon(stream, phrases[0], def_path_no);
    free(phrases[0]);
    free(phrases);
    return rv;
}

static int print_line(FILE * stream, const char *buf, int def_path_no)
{
    const char *tmp;
    const char *phrase;
    int i = 0;
    int retval = 0;

#define PHRASE_LEN 1000
    char phrase_name[PHRASE_LEN];

    while (1)
    {
        phrase_name[0] = '\0';
        if (!(phrase = find_at_phrases(buf)))
        {
            fputs(buf, stream);
            return retval;
        }

        /* write all chars up to the beginning '@' thingie to stream */
        for (tmp = buf; tmp < phrase; tmp++)
            fputc(*tmp, stream);

        /* copy Phrase to phrase_name */
        i = 0;
        for (tmp = phrase + 1; *tmp != '@' && i < PHRASE_LEN; tmp++)
        {
            phrase_name[i++] = *tmp;
        }
        phrase_name[i] = '\0';
        /* pasrse Phrase */
        retval |= parse_phrase(stream, phrase_name, def_path_no);
        /* re-adjust output buffer */
        buf = tmp + 1;
    }
}

#define do_print_line(STREAM, BUF) for(i = 0; print_line(STREAM, BUF, i); i++)

static void print_it(FILE * stream, const char *buf)
{
    char *line, *tmp;
    int i;

    D_("print_it: Got block \"%s\"\n", buf);
    while (1)
    {
        tmp = strchr(buf, '\n') + 1;
        if (tmp == (char *) 1)
        {
            do_print_line(stream, buf);
            return;
        }
        else
        {
            line = strndup(buf, tmp - buf);
            buf = tmp;
            D_("print_it: Got line \"%s\"\n", line);
            do_print_line(stream, line);
            free(line);
        }
        if (!*buf)
            return;
    }
}

static char *do_exec(char *data, int length)
{
    int pipe_fork[2], inpipe_fork[2];

    if (pipe(pipe_fork) == -1 || pipe(inpipe_fork) == -1)
    {
        perror("pipe()");
        return 0;
    }

    int status;
    int datalen = 0;
    int pid;
    char *return_data = NULL;

    D_("forking...\n");
    if ((pid = fork()) == 0)
    {
        const char *argv[] = { "/bin/bash", NULL, NULL };
        dup2(pipe_fork[0], 0);
        close(pipe_fork[1]);
        dup2(inpipe_fork[1], 1);
        dup2(inpipe_fork[1], 2);
        close(inpipe_fork[0]);
        execve("/bin/bash", (char **) argv, environ);
        printf("  **\texecve /bin/bash FAILED!\n");
        exit(1);
    }
    else
    {
        D_("This is not fork\n");
        return_data = malloc(1024);
        close(pipe_fork[0]);

        /*        printf("Writing data to pipe : (%d) \n",length);
           int x;
           for (x = 0; x <= length;x++)
           printf("%c",data[x]);
           printf("\n"); */

        D_("Data sent to fork (%i): \"%s\"\n", length, data);
        if (TEMP_FAILURE_RETRY(write(pipe_fork[1], data, length)) != length ||
            TEMP_FAILURE_RETRY(write(pipe_fork[1], "\n\nexit\n", 7)) != 7)
        {
            fprintf(stderr, "%s:%u: failed to write all data\n",
                    __FILE__, __LINE__);
        }
        close(pipe_fork[1]);

        wait(&status);

        int fd_flags;

        fd_flags = fcntl(inpipe_fork[0], F_GETFL, 0);
        fcntl(inpipe_fork[0], F_SETFL, fd_flags | O_NONBLOCK);
        close(inpipe_fork[1]);
        D_("Reading data from fork\n");
        datalen = read(inpipe_fork[0], return_data, 1023);
        close(inpipe_fork[0]);
        if (datalen > 0)
        {
            D_("Read %d bytes: \"%s\"\n", datalen, return_data);
            return_data[datalen] = '\0';
            return return_data;
        }
        printf("  **\tDid not got any data from #exec statement!\n");
        free(return_data);
        return NULL;
    }
    return NULL;
}

static inline const char *skip_spaces(const char *in)
{
    while (*in == ' ' || *in == '\t')
        in++;
    return in;
}

static inline int empty_elsed(const char *in)
{
    in = skip_spaces(in);
    in += LEN("#elsed");
    while (*in == ' ' || *in == '\t')
        in++;
    if (*in == '\n')
        return 1;
    else
        return 0;
}

/* this verifies that the string "distro" is found in the "in" string
 * and that the character before the string "distro" in "in" is either
 * ' ' or '\t'
 * the character after "distro" meight either be ' ', '\t' or '\n'
 */
static inline int match_distro(const char *in, const char *distro)
{
    const char *tmp;
    int len = strlen(distro);

    tmp = skip_spaces(in);
    while (1)
    {
        tmp = strstr(tmp, distro);
        if (!tmp)
            return 0;
        if ((tmp[-1] != ' ' && tmp[-1] != '\t')
            || (tmp[len] != ' ' && tmp[len] != '\t' && tmp[len] != '\n'))
            tmp++;                          /* this will make strstr not find the same part again */
        else
            return 1;
    }
}

typedef enum
{
    UNKNOWN_BLOCK = 0,
    IF_BLOCK = 1,
    EXEC_BLOCK = 2
} enum_block;

#define EXEC_BUFFER_LEN 100000

int main(int argc, char **argv)
{
    /*
     * VARIABLES
     */

    /* simple counter */
    int i = 1;

    /* getopt_long-options */
    static struct option long_options[] = {
        {"dist", required_argument, 0, 'd'},
        {"in", required_argument, 0, 'i'},
        {"out", required_argument, 0, 'o'},
        {0, 0, 0, 0}
    };

    /* Filepointers to in and outfile */
    FILE *in;
    FILE *out = NULL;

    /* a string containing the name of the distro we are working on */
    const char *distro = NULL;

    /* filenames, of in and outfiles */
    const char *infile = NULL;
    const char *outfile = NULL;

    /* temporary data storage */
    const char *data;

    /* space to get the line we are parsing */
    char line[LINE_LEN];

    /* if parsing an #exec, fill this string with #exec data */
    char exec_buffer[EXEC_BUFFER_LEN];

    /* The maximum exec_buffer lengt we have left to fill */
    int exec_buffer_free = EXEC_BUFFER_LEN - 1;

    /* Current line number in input */
    int lineno = 0;

    /* Stack - please use the macros defined below, if possible */
    struct
    {
        enum_block block_type;
        int process_elses;
        int print_out;
        int lineno;
    } stack[16];

    int stack_top = 0;

#define POP_STACK --stack_top
#define PUSH_STACK(type) ( stack[stack_top].process_elses = BLOCK_PRINTS_OUT, \
                           stack[stack_top].print_out = BLOCK_PRINTS_OUT, \
                           stack[stack_top].lineno = lineno, \
                           stack[stack_top++].block_type = type)
#define BLOCK_PRINTS_OUT ( stack_top ? stack[stack_top-1].print_out : 1 )
#define STACK_EMPTY ( stack_top == 0 )
#define STACK_TOP ( stack[stack_top-1] )
#define IN_BLOCK (  stack_top ? stack[stack_top-1].block_type : 0 )

    /*
     *  CODE
     */

    /* parse and fill path struct */
    path = str_split(getenv("PATH"), ':');
    if (!path)
    {
        fprintf(stderr, "  **\tERROR: could not get $PATH\n");
        exit(1);
    }

    /* Parse initial arguments */
    while (-1 != (i = getopt_long(argc, argv, "hi:o:d:", long_options, NULL)))
    {
        switch (i)
        {
                /* set the distro manually */
            case 'd':
                distro = optarg;
                break;
                /* set the file to parse */
            case 'i':
                infile = optarg;
                break;
                /* set the outfile manually */
            case 'o':
                outfile = optarg;
                break;
            default:
                fprintf(stdout, "Ups, that's that? <0%o>\n", i);
        }
    }

    /* if not set, probe it */
    if (!distro)
        distro = probe_distribution();
    if (!distro)
    {
        fprintf(stderr, "  **\tUnknown DISTRIBUTION\n");
        exit(1);
    }
    fprintf(stderr, "  **\tDistribution set: \"%s\"\n", distro);

    /* load the overlay */
    {
        int j;
        char file[200];

        /* reset the ovelay */
        for (j = 0; j < 1000; j++)
        {
            overlay[j].opt = NULL;
            overlay[j].value = NULL;
        }

        /* and distro dependent overlay, found in workdir */
        sprintf(file, "%s.overlay", distro);
        load_overlay_file(file);

        /* a standard path for overlay */
		strcpy(file, INITNG_ROOT);
		strcat(file, "/initng.overlay");
        load_overlay_file(file);

        /*printf("Overlays found:\n");
           for(j=0;j<1000 && overlay[j].opt;j++)
           {
           printf(" \"%s\" == \"%s\"\n", overlay[j].opt, overlay[j].value);
           } */

    }

    /* open input for writing */
    if (!infile)
    {
        fprintf(stderr, "  **\tYou have to set input file with -i\n");
        exit(1);
    }
    if (!(in = fopen(infile, "r")))
    {
        fprintf(stderr, "  **\tcould not open input file!\n");
        exit(1);
    }

    /* open output for writing */
    if (!outfile)
    {
        if (strstr(infile, ".ii"))
        {
            outfile = strndup(infile, strlen(infile) - 1);
        }
        else
        {
            fprintf(stderr,
                    "  **\tout with -o not set, defaulting to stdout.\n");
            out = stdout;
        }
    }
    if (!out && !(out = fopen(outfile, "w")))
    {
        fprintf(stderr, "  **\tcould not open output file!\n");
        exit(1);
    }

	/* Print header */
	fprintf(out, "#!/sbin/itype\n# This is a i file, used by initng parsed by install_service\n\n");

    /* Okay, go parse every line */
    while (fgets(line, LINE_LEN, in))
    {
        lineno++;

        /*
         * This is an internal set, to force an standard default path,
         * please us an direct @/usr/sbin/gdm@ instead.
         */
        if (MATCH("#atdefpath", line))
        {
            D_("found #atdefpath. updating at default path\n");

            /* get the data after #atdefpath data */
            data = skip_spaces(line + LEN("#atdefpath"));

            /* free previous fore path */
            if (at_default_path)
                free(at_default_path);

            /* allocate an new one */
            at_default_path = malloc(strlen(data) + 1);

            /* copy the data */
            i = 0;
            while (data[i] && data[i] != ' ' && data[i] != '\t'
                   && data[i] != '\n')
            {
                at_default_path[i] = data[i];
                i++;
            }
            at_default_path[i] = '\0';

            D_("at_default_path updated to %s\n", at_default_path);

            /* explain */
            at_default_paths = str_split(at_default_path, ':');
            continue;
        }

        /* If this is an ordinary line, not starting with an # */
        if (STACK_EMPTY && line[0] != '#')
        {
            /* youst print it to outfile */
            print_it(out, line);
            continue;                       /* read next line */
        }

        /* either in_block != 0 or line started with '#' */
        if (IN_BLOCK == EXEC_BLOCK)
        {
            /* in_block == EXEC_BLOCK */
            if (MATCH("#endexec", line))
            {
                D_("found #endexec, line %i\n", lineno);
                /* Do the exec shit */
                if (BLOCK_PRINTS_OUT)
                {
                    data = do_exec(exec_buffer, strlen(exec_buffer));
                    if (data)
                    {
                        print_it(out, data);
                        free((void *) data);
                    }
                }
                POP_STACK;
                continue;
            }
            else
            {
                strncat(exec_buffer, line, exec_buffer_free);
                exec_buffer_free -= strlen(line);
                continue;
            }

        }
        else
        {

            /* #exec is an internal run, and output will be outputed to outfile */
            if (MATCH("#exec", line))
            {
                D_("found #exec, line %i\n", lineno);
                exec_buffer[0] = '\0';
                exec_buffer_free = EXEC_BUFFER_LEN - 1;
                PUSH_STACK(EXEC_BLOCK);
                continue;
            }

            else if (MATCH("#endexec", line))
            {
                fprintf(stderr, "ERROR: #endexec without #exec, line %i\n",
                        lineno);
                fclose(out);
                if (outfile)
                    unlink(outfile);
                exit(1);
            }

            /* #ifd debian, meens utput this to outfile if distro == debian */
            else if (MATCH("#ifd", line))
            {
                D_("found #ifd, line %i\n", lineno);
                PUSH_STACK(IF_BLOCK);
                STACK_TOP.print_out = 0;
                if (STACK_TOP.process_elses && match_distro(line, distro))
                {
                    D_("actual distro %s matches line %i: %s\n", distro,
                       lineno, line);
                    STACK_TOP.print_out = 1;
                    STACK_TOP.process_elses = 0;
                }
                continue;
            }

            else if (MATCH("#elsed", line))
            {
                if (IN_BLOCK != IF_BLOCK)
                {
                    fprintf(stderr, "ERROR: unexpected #elsed, line %i\n",
                            lineno);
                    fclose(out);
                    if (outfile)
                        unlink(outfile);
                    exit(1);
                }
                if (STACK_TOP.process_elses)
                {
                    if (empty_elsed(line) || match_distro(line, distro))
                    {
                        STACK_TOP.print_out = 1;
                        STACK_TOP.process_elses = 0;
                    }
                }
                else
                {
                    STACK_TOP.print_out = 0;
                }
            }

            else if (MATCH("#endd", line))
            {
                if (IN_BLOCK != IF_BLOCK)
                {
                    fprintf(stderr, "ERROR: unexpected #endd, line %i\n",
                            lineno);
                    fclose(out);
                    if (outfile)
                        unlink(outfile);
                    exit(1);
                }
                POP_STACK;
            }
            else
            {
                /* it's just an ordinary line */
                if (BLOCK_PRINTS_OUT)
                    print_it(out, line);
                continue;                   /* read next line */
            }
        }
    }

    if (!STACK_EMPTY)
    {
        while (!STACK_EMPTY)
        {
            if (IN_BLOCK == EXEC_BLOCK)
                fprintf(stderr,
                        "ERROR: missing #endexec for #exec on line %i\n",
                        STACK_TOP.lineno);
            else
                fprintf(stderr, "ERROR: missing #endd for #ifd on line %i\n",
                        STACK_TOP.lineno);
            POP_STACK;
        }
        fclose(out);
        if (outfile)
            unlink(outfile);
        exit(1);
    }

    /* free the overlay table */
    for (i = 0; i < 1000 && overlay[i].opt; i++)
    {
        free((char *) overlay[i].opt);      /* const-cast */
        overlay[i].opt = NULL;
        if (overlay[i].value)
        {
            free((char *) overlay[i].value);    /* const-cast */
            overlay[i].value = NULL;
        }
    }

    /* close bouth input and output file */
    fclose(in);
    fclose(out);
    exit(0);
}
