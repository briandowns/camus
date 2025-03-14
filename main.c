/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2025 Brian J. Downs
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>

#define STR1(x) #x
#define STR(x) STR1(x)

#define EVENT_SIZE  (sizeof(struct inotify_event))
#define BUF_LEN     (1024 * (EVENT_SIZE + 16))
#define MAX_CMD_LEN 70

#define USAGE                 \
    "usage: %s [-vh]\n"       \
    "  -v          version\n" \
    "  -h          help\n"    \
    "  -t          target\n"

bool
valid_file_type(const char *filename)
{
    if (filename == NULL) {
        return false;
    }

    const char *dot = strrchr(filename, '.');
    if (dot == NULL || dot == filename) {
        return false;
    }

    return (strcmp(dot + 1, "c") == 0) || (strcmp(dot + 1, "h") == 0);
}

void
handle_signal(int sig)
{
    if (sig == SIGINT) {

    }
    switch (sig) {
    case SIGINT:
        printf("\nshutting down camus...\n");
    }
    exit(0);
}

int
main(int argc, char **argv)
{
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        return 1;
    }

    char target[64];

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            printf("version: %s - git: %s\n", STR(camus_version),
                   STR(git_sha));
            return 0;
        }
        if (strcmp(argv[i], "-h") == 0) {
            printf(USAGE, STR(bin_name));
            return 0;
        }
        if (strcmp(argv[i], "-t") == 0) {
            if (argv[i+1] != NULL || argv[i+1][0] != '\0') {
                strncpy(target, argv[i+1], 64);
            }
            break;
        }
    }

    char buffer[BUF_LEN];

    int fd = inotify_init();
    if (fd < 0) {
        fprintf(stderr, "failed to initialize inotify\n");
        return 1;
    }

    int wd;
    int i = 0;
    int ret = 0;

    while (1) {
        wd = inotify_add_watch(fd, ".", IN_MODIFY | IN_CREATE | IN_DELETE);

        ret = read(fd, buffer, BUF_LEN);
        if (ret < 0) {
            fprintf(stderr, "failed to read buffer\n");
            return 1;
        }
    
        struct inotify_event *event = (struct inotify_event *) &buffer[i];
        if (event->len) {
            if (!valid_file_type(event->name)) {
                continue;
            }

            if ((event->mask & IN_CREATE) || (event->mask & IN_MODIFY) ||
                (event->mask & IN_DELETE)) {
                char cmd[MAX_CMD_LEN] = "make ";

                if (target[0] != '\0') {
                    strncat(cmd, target, MAX_CMD_LEN-6);
                }

                int r = system(cmd);
                if (r != 0) {
                    fprintf(stderr, "it failed...");
                    return 1;
                }
            }
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);

    return 0;
}
