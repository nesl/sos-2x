/**
 * @file setup_db.c
 * @breif sets up surge SQLite3 database tables for surge_db.c.
 * @author Peter Mawhorter (pmawhorter@cs.hmc.edu)
 */


#include <stdlib.h>
#include <stdio.h>

#include <sqlite3.h>

#define SQLITE_PROBLEM 1

const char* FILENAME = "messages.sql";

/*
 * Sets up the sqlite database complete with tables, etc.
 */
int main(int argc, char **argv) {
  int ret; // return value
  char *err; // error value
  // Command to set up the tables:
  const char *setup = "CREATE TABLE messages (id INTEGER PRIMARY KEY, origin_address INTEGER, routing_sequence_number INTEGER, hop_count INTEGER, origin_hop_count INTEGER, parent_address INTEGER, surge_message_type INTEGER, surge_sequence_number INTEGER, reading INTEGER); CREATE TABLE motes (address INTEGER, start_time INTEGER, last_time INTEGER)";
  sqlite3 *connection; // The connection to the database.
  
  // Open the database:
  ret = sqlite3_open(FILENAME, &connection);

  if (ret != 0) {
    printf(sqlite3_errmsg(connection));
    sqlite3_close(connection);
    exit(SQLITE_PROBLEM);
  }

  err = NULL;
  ret = sqlite3_exec(connection, setup, NULL, NULL, &err);

  if (err != NULL) {
    printf("SQLite database error: %s\n", err);
    sqlite3_close(connection);
    exit(SQLITE_PROBLEM);
  } else if (ret != 0) {
    printf(sqlite3_errmsg(connection));
    sqlite3_close(connection);
    exit(SQLITE_PROBLEM);
  }

  sqlite3_close(connection);

  return 0;
}
