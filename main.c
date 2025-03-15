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

#define _XOPEN_SOURCE 800
#include <dirent.h>
#ifdef __linux__
#include <ftw.h>
#endif
#include <linux/limits.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define STR1(x) #x
#define STR(x) STR1(x)

#define EVENT_SIZE            (sizeof(struct inotify_event))
#define BUF_LEN               (1024 * (EVENT_SIZE + 16))
#define MAX_CMD_LEN           70
#define IGNORED_FILE_EXTS_LEN 12
#define IGNORED_DIRS_LEN      3
#define CMD_BASE              "make "

#define USAGE                 \
    "usage: %s [-vh]\n"       \
    "  -v          version\n" \
    "  -h          help\n"    \
    "  -t          target\n"


static char*
ignored_file_exts[IGNORED_FILE_EXTS_LEN] = {
    "git",
    "md",
    "json", "yml", "yaml",
    "txt", "doc", "docx",
    "xls", "xlsx",
    "ppt", "pptx"
};

static char*
ignored_dirs[IGNORED_DIRS_LEN] = {
    "./.git", "./.github", "./bin"
};

int i_fd, wd;

bool
ignored_file_type(const char *filename)
{
    if (filename == NULL) {
        return false;
    }

    const char *dot = strrchr(filename, '.');
    if (dot == NULL || dot == filename) {
        return false;
    }

    for (uint8_t i = 0; i < IGNORED_FILE_EXTS_LEN; i++) {
        if (strcmp(dot+1, ignored_file_exts[i]) == 0) {
            return true;
        }
    }

    return false;
}

bool
ignored_directory(const char *name)
{
    for (uint8_t i = 0; i < IGNORED_DIRS_LEN; i++) {
        if (strcmp(name, ignored_dirs[i]) == 0) {
            return true;
        }
    }

    return false;
}

static int
mon_dir(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    if (typeflag == FTW_D) {
        if (ignored_directory(fpath)) {
            return 1;
        }

        wd = inotify_add_watch(i_fd, fpath, IN_CREATE | IN_DELETE | IN_MODIFY);
        if (wd == -1) {
            perror("inotify_add_watch");
            return 1;
        }
        printf("Watching: %s\n", fpath);
    }

    return 0;
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

    char cmd[MAX_CMD_LEN] = CMD_BASE;
    if (target[0] != '\0') {
        strncat(cmd, target, MAX_CMD_LEN-6);
    }

    char buffer[BUF_LEN];

    i_fd = inotify_init();
    if (i_fd < 0) {
        fprintf(stderr, "failed to initialize inotify\n");
        return 1;
    }

    if (nftw(".", mon_dir, 20, FTW_PHYS) == -1) {
        perror("nftw");
        close(i_fd);
        return 1;
    }

    ssize_t i = 0;
    int ret = 0;

    while (1) {
        ret = read(i_fd, buffer, BUF_LEN);
        if (ret < 0) {
            fprintf(stderr, "failed to read buffer\n");
            return 1;
        }
    
        struct inotify_event *event = (struct inotify_event *)&buffer[i];
        if (event->len) {
            // ignore events to directories
            if (event->mask & IN_ISDIR) {
                continue;
            }

            // ignore non code files
            if (ignored_file_type(event->name)) {
                continue;
            }

            if ((event->mask & IN_CREATE) || (event->mask & IN_MODIFY) ||
                (event->mask & IN_DELETE)) {

                system(cmd);
            }
        }
    }

    inotify_rm_watch(i_fd, wd);
    close(i_fd);

    return 0;
}
