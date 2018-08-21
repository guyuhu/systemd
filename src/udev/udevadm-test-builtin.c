/* SPDX-License-Identifier: GPL-2.0+ */

#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "path-util.h"
#include "string-util.h"
#include "udev.h"
#include "udevadm.h"
#include "udevadm-util.h"

static void help(void) {
        printf("%s test-builtin [OPTIONS] COMMAND DEVPATH\n\n"
               "Test a built-in command.\n\n"
               "  -h --help     Print this message\n"
               "  -V --version  Print version of the program\n\n"
               "Commands:\n"
               , program_invocation_short_name);

        udev_builtin_list();
}

int builtin_main(int argc, char *argv[], void *userdata) {
        static const struct option options[] = {
                { "version", no_argument, NULL, 'V' },
                { "help",    no_argument, NULL, 'h' },
                {}
        };
        char *command = NULL;
        char *syspath = NULL;
        char filename[UTIL_PATH_SIZE];
        struct udev_device *dev = NULL;
        enum udev_builtin_cmd cmd;
        int rc = EXIT_SUCCESS, c;

        log_set_max_level(LOG_DEBUG);

        while ((c = getopt_long(argc, argv, "Vh", options, NULL)) >= 0)
                switch (c) {
                case 'V':
                        print_version();
                        goto out;
                case 'h':
                        help();
                        goto out;
                }

        command = argv[optind++];
        if (command == NULL) {
                fprintf(stderr, "command missing\n");
                help();
                rc = 2;
                goto out;
        }

        syspath = argv[optind++];
        if (syspath == NULL) {
                fprintf(stderr, "syspath missing\n");
                rc = 3;
                goto out;
        }

        udev_builtin_init();

        cmd = udev_builtin_lookup(command);
        if (cmd >= UDEV_BUILTIN_MAX) {
                fprintf(stderr, "unknown command '%s'\n", command);
                help();
                rc = 5;
                goto out;
        }

        /* add /sys if needed */
        if (!path_startswith(syspath, "/sys"))
                strscpyl(filename, sizeof(filename), "/sys", syspath, NULL);
        else
                strscpy(filename, sizeof(filename), syspath);
        delete_trailing_chars(filename, "/");

        dev = udev_device_new_from_syspath(NULL, filename);
        if (dev == NULL) {
                fprintf(stderr, "unable to open device '%s'\n\n", filename);
                rc = 4;
                goto out;
        }

        rc = udev_builtin_run(dev, cmd, command, true);
        if (rc < 0) {
                fprintf(stderr, "error executing '%s', exit code %i\n\n", command, rc);
                rc = 6;
        }
out:
        udev_device_unref(dev);
        udev_builtin_exit();
        return rc;
}
