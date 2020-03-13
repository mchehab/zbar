/*------------------------------------------------------------------------
 *  Copyright 2019 (c) Mauro Carvalho Chehab <mchehab+samsung@kernel.org>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *------------------------------------------------------------------------*/

#include <argp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dbus/dbus.h>

#define ZBAR_INTERFACE "org.linuxtv.Zbar1.Code"
#define ZBAR_SIGNAL_CODE "Code"
#define ZBAR_SIGNAL_TYPE "Type"
#define ZBAR_SIGNAL_DATA "Data"
#define ZBAR_SIGNAL_BINARY_DATA "BinaryData"

#define PROGRAM_NAME	"test_dbus"

static const char doc[] = "\nTest if ZBar is sending codes via D-Bus\n";

static const struct argp_option options[] = {
    {"count",   'c', "#codes",   0, "Stop after received #codes", 0},
    {"time",    't', "#seconds", 0, "Stop after #seconds",        0},
    {"log",     'l', "#file",    0, "Write log to #file",         0},
    {"bin-log", 'b', "#file",    0, "Write binary log to #file",  0},
    {"help",    '?', 0,          0, "Give this help list",       -1},
    {"usage",   -3,  0,          0, "Give a short usage message", 0},
    { 0 }
};

static int max_msg = 0;
static int timeout = 0;
static FILE *log   = NULL;
static FILE *bin_log  = NULL;

static error_t parse_opt(int k, char *optarg, struct argp_state *state)
{
    switch (k) {
    case 'c':
        max_msg = strtoul(optarg, NULL, 0);
        break;
    case 't':
        timeout = strtoul(optarg, NULL, 0);
        break;
    case 'l':
        log = fopen(optarg, "wb");
        break;
    case 'b':
        bin_log = fopen(optarg, "wb");
        break;
    case '?':
        argp_state_help(state, state->out_stream,
                        ARGP_HELP_SHORT_USAGE | ARGP_HELP_LONG |
                        ARGP_HELP_DOC);
        exit(0);
    case -3:
        argp_state_help(state, state->out_stream, ARGP_HELP_USAGE);
        exit(0);
    default:
        return ARGP_ERR_UNKNOWN;
    };
    return 0;
}

static const struct argp argp = {
	.options = options,
	.parser = parse_opt,
	.doc = doc,
};

int main(int argc, char *argv[])
{
    DBusMessage* msg;
    DBusMessageIter args, entry, dict, val;
    DBusConnection* conn;
    DBusError err;
    char *str, *property;
    int count = 0, length = 0;

    if (argp_parse(&argp, argc, argv, ARGP_NO_HELP | ARGP_NO_EXIT, 0, 0)) {
        argp_help(&argp, stderr, ARGP_HELP_SHORT_USAGE, PROGRAM_NAME);
        return -1;
    }

    if (!log)
	log = fdopen(dup(fileno(stderr)), "w+");

    if (!bin_log)
	bin_log = fdopen(dup(fileno(stderr)), "w+");

    // initialise the error value
    dbus_error_init(&err);

    // connect to the DBUS system bus, and check for errors
    conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
    }
    if (!conn) {
        fprintf(stderr, "Connection Null\n");
        return -1;
    }

    dbus_bus_add_match(conn,
		       "type='signal',interface='" ZBAR_INTERFACE "'",
		       &err);
   dbus_connection_flush(conn);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Match Error (%s)\n", err.message);
      exit(1);
   }

   if (timeout)
       alarm(timeout);

   /* loop listening for signals being emitted */
   fprintf(stderr, "Waiting for Zbar events\n");
   while (true) {
      // non blocking read of the next available message
      dbus_connection_read_write(conn, 0);
      msg = dbus_connection_pop_message(conn);

      // loop again if we haven't read a message
      if (NULL == msg) {
         sleep(1);
         continue;
      }

      // check if the message is a signal from the correct interface and with the correct name
      if (dbus_message_is_signal(msg, ZBAR_INTERFACE, ZBAR_SIGNAL_CODE)) {
         // read the parameters
         if (!dbus_message_iter_init(msg, &args))
            fprintf(stderr, "Message has no arguments!\n");
         else if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&args))
            fprintf(stderr, "Argument is not array!\n");
         else {
            while (dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_INVALID) {
              dbus_message_iter_recurse(&args, &entry);
              if (DBUS_TYPE_DICT_ENTRY != dbus_message_iter_get_arg_type(&entry)) {
                fprintf(stderr, "Element is not dict entry!\n");
              } else {
                while (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_INVALID) {
                  dbus_message_iter_recurse(&entry, &dict);
                  if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&dict)) {
                    fprintf(stderr, "Dict Entry key is not string!\n");
                  } else {
                    dbus_message_iter_get_basic(&dict, &property);
                    dbus_message_iter_next(&dict);
                    if (DBUS_TYPE_VARIANT != dbus_message_iter_get_arg_type(&dict)) {
                      fprintf(stderr, "Dict Entry value is not variant!\n");
                    } else {
                      dbus_message_iter_recurse(&dict, &val);
                      if (strcmp(property, ZBAR_SIGNAL_TYPE) == 0) {
                        if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&val)) {
                          fprintf(stderr, "Dict Entry value for barcode type is not string!\n");
                        } else {
                          dbus_message_iter_get_basic(&val, &str);
                          fprintf(stderr, "Type = %s\n", str);
                        }
                      } else if (strcmp(property, ZBAR_SIGNAL_DATA) == 0) {
                        if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&val)) {
                          fprintf(stderr, "Dict Entry value for barcode text data is not string!\n");
                        } else {
                          dbus_message_iter_get_basic(&val, &str);
                          fprintf(stderr, "Value = %s\n", str);
                          fprintf(log, "%s\n", str);
                        }
                      } else if (strcmp(property, ZBAR_SIGNAL_BINARY_DATA) == 0) {
                        if (DBUS_TYPE_ARRAY != dbus_message_iter_get_arg_type(&val)) {
                          fprintf(stderr, "Dict Entry value for barcode binary data is not array!\n");
                        } else {
                          dbus_message_iter_recurse(&val, &val);
                          if (DBUS_TYPE_BYTE != dbus_message_iter_get_arg_type(&val)) {
                            fprintf(stderr, "Dict Entry value for barcode binary data is not array of bytes!\n");
                          } else {
                            dbus_message_iter_get_fixed_array(&val, &str, &length);
                            fprintf(stderr, "BinaryData[%d]\n", length);
                            fwrite(str, sizeof(*str), length, bin_log);
                          }
                        }
                      }
                    }
                  }
                  dbus_message_iter_next(&entry);
                }
                /* If max_msg > 0, stops after receiving 'count' messages */
                if (++count == max_msg) {
                    dbus_message_unref(msg);
                    return 0;
                }
              }
              dbus_message_iter_next(&args);
            }
         }
      }
      // free the message
      dbus_message_unref(msg);
   }

   fclose(log);
   fclose(bin_log);

   return 0;
}
