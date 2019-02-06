#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <dbus/dbus.h>

#define ZBAR_INTERFACE "org.linuxtv.Zbar1.Code"
#define ZBAR_SIGNAL_TYPE "Type"
#define ZBAR_SIGNAL_CODE "Code"

int main(void)
{
    DBusMessage* msg;
    DBusMessageIter args;
    DBusConnection* conn;
    DBusError err;
    char *str;

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
// loop listening for signals being emmitted
   printf("Waiting for Zbar events\n");
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
      if (dbus_message_is_signal(msg, ZBAR_INTERFACE, ZBAR_SIGNAL_TYPE)) {
         // read the parameters
         if (!dbus_message_iter_init(msg, &args))
            fprintf(stderr, "Message has no arguments!\n");
         else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
            fprintf(stderr, "Argument is not string!\n");
         else {
            dbus_message_iter_get_basic(&args, &str);
	    printf("Type = %s\n", str);
         }
      } else if (dbus_message_is_signal(msg, ZBAR_INTERFACE, ZBAR_SIGNAL_CODE)) {
         // read the parameters
         if (!dbus_message_iter_init(msg, &args))
            fprintf(stderr, "Message has no arguments!\n");
         else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
            fprintf(stderr, "Argument is not string!\n");
         else {
            dbus_message_iter_get_basic(&args, &str);
	    printf("Value = %s\n", str);
         }
      }


      // free the message
      dbus_message_unref(msg);
   }
   return 0;
}
