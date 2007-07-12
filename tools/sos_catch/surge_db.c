/**
 * @file surge_db.c
 * @breif dumps surge messages into an sqlite3 database.
 * @author Peter Mawhorter (pmawhorter@cs.hmc.edu)
 */

#include <stdio.h>

#include <sqlite3.h>

#include <message_types.h>
#include <tree_routing.h>
#include <surge.h>

#define BAD_ORIGIN_ADDRESSES 1
#define SQLITE_PROBLEM 2
#define OVERFLOW_ERROR 3

const char* FILENAME = "messages.sql";

/*
 * An sqlite3 callback function that sets the given variable to 1 when executed.
 * This is for use as a field existence detector: use it as the callback for
 * a select execution and pass it a variable set to 0. Afterwards, if(that
 * variable) corresponds to the existence of one or more fields matching the
 * select statement.
 */
int field_found(void* out, int cols, char** values, char** titles) {
  *(int*)out = 1;
  return 0;
}

/*
 * Uses the given connection to check for the existence of a mote with the
 * given id. The mote is assumed to exist if it has an entry in the motes
 * table. If there is no entry in the motes table, then it is assumed that
 * the mote does not exist and returns 0.
 */
int mote_exists(int mote_id, sqlite3* connection) {
  int ret; // SQLite3 return value.
  int exists = 0; // Whether the mote exists or not.
  int check=0; // Buffer overflow?
  const int QUERY_SIZE = 50; // How many characters could the querry be?
  char query[QUERY_SIZE]; // SQL querry for the database.
  char *err = NULL; // SQLite3 error message.

  check = sprintf(query, "SELECT * FROM motes WHERE address=%d", mote_id);
  if (check >= QUERY_SIZE) {
    printf("Query string overflow: %d characters written to buffer of size %d.",
           check, QUERY_SIZE);
    sqlite3_close(connection);
    exit(OVERFLOW_ERROR);
  }

  ret = sqlite3_exec(connection, query, &field_found, &exists, &err);

  if (err != NULL) {
    printf("SQLite database error: %s\n", err);
    sqlite3_close(connection);
    exit(SQLITE_PROBLEM);
  } else if (ret != 0) {
    printf(sqlite3_errmsg(connection));
    sqlite3_close(connection);
    exit(SQLITE_PROBLEM);
  }

  return exists;
}

/*
 * Catches surge messages and dumps them into messages.sql:
 */
int catch(Message* msg) {
  int i; // Loop variable.
  int ret; // Sqlite 3 return value.
  int check; // Check on the size of the sprintf into command.
  int tm = time(NULL); // Timestamp.
  const int COMMAND_SIZE=150; // Size of sqlite command.
  char tree_routing_header[sizeof(tr_hdr_t)];
  char surge_message[sizeof(tr_hdr_t)];
  char command[COMMAND_SIZE]; // The sqlite 3 command we'll be using.
  char *err; // Place for sqlite error messages.
  sqlite3* connection; // The sqlite 3 database object.

  // We only care about tree routing messages:
  if (msg->type == MSG_TR_DATA_PKT) {
    // Split off the tree routing header:
    for(i=0; i < sizeof(tr_hdr_t); ++i) {
      tree_routing_header[i] = msg->data[i];
    }
    tr_hdr_t* tr_hdr = (tr_hdr_t*)(tree_routing_header);

    // We only care about messages for the Surge module:
    if (tr_hdr->dst_pid == SURGE_MOD_PID) {
      // Get the surge message:
      for(i=0; i < sizeof(SurgeMsg); ++i) {
        surge_message[i] = msg->data[sizeof(tr_hdr_t)+i];
      }
      SurgeMsg* sg_msg = (SurgeMsg*)(surge_message);

      // Flip the endiannesses of the relevant fields:
      tr_hdr->originaddr = entohs(tr_hdr->originaddr);
      tr_hdr->seqno = entohs(tr_hdr->seqno);
      tr_hdr->parentaddr = entohs(tr_hdr->parentaddr);
      sg_msg->originaddr = entohs(sg_msg->originaddr);
      sg_msg->reading = entohs(sg_msg->reading);
      sg_msg->seq_no = entohl(sg_msg->seq_no);

      // Sanity checking:
      if (tr_hdr->originaddr != sg_msg->originaddr) {
        printf("Tragic loss of coherence: origin addresses don't agree:\n");
        printf("Tree Routing: %d\nSurge: %d\n", tr_hdr->originaddr,
               sg_msg->originaddr);
        exit(BAD_ORIGIN_ADDRESSES);
      }

      // Open the database:
      ret = sqlite3_open(FILENAME, &connection);

      if (ret != 0) {
        printf(sqlite3_errmsg(connection));
        sqlite3_close(connection);
        exit(SQLITE_PROBLEM);
      }

      /*
       * Check that the mote we're adding data for is already listed in the
       * motes table. If it isn't we need to add it, along with a start time
       * and last time.
       */
      if (!mote_exists(tr_hdr->originaddr, connection)) {
        check = sprintf(command, "INSERT INTO motes VALUES(%d, %d, %d)",
                        tr_hdr->originaddr, tm, tm);

        if (check >= COMMAND_SIZE) {
          printf(
           "Query string overflow: %d characters written to buffer of size %d.",
           check, COMMAND_SIZE);
          sqlite3_close(connection);
          exit(OVERFLOW_ERROR);
        }

        ret = sqlite3_exec(connection, command, NULL, NULL, &err);
   
        if (err != NULL) {
          printf("SQLite database error: %s\n", err);
          sqlite3_close(connection);
          exit(SQLITE_PROBLEM);
        } else if (ret != 0) {
          printf(sqlite3_errmsg(connection));
          sqlite3_close(connection);
          exit(SQLITE_PROBLEM);
        }

      }

      /*
       * Construct a command for inserting the message into the database.
       * The surge origin address is dropped because it's the same as the
       * tree routing origin address.
       */
      check = sprintf(command,
            "INSERT INTO messages VALUES(NULL, %d, %d, %d, %d, %d, %d, %d, %d)",
            tr_hdr->originaddr,
            tr_hdr->seqno,
            tr_hdr->hopcount,
            tr_hdr->originhopcount,
            tr_hdr->parentaddr,
            sg_msg->type,
            sg_msg->seq_no,
            sg_msg->reading);

      if (check >= COMMAND_SIZE) {
        printf(
          "Query string overflow: %d characters written to buffer of size %d.",
          check, COMMAND_SIZE);
        sqlite3_close(connection);
        exit(OVERFLOW_ERROR);
      }

      err = NULL;
      ret = sqlite3_exec(connection, command, NULL, NULL, &err);

      if (err != NULL) {
        printf("SQLite database error: %s\n", err);
        sqlite3_close(connection);
        exit(SQLITE_PROBLEM);
      } else if (ret != 0) {
        printf(sqlite3_errmsg(connection));
        sqlite3_close(connection);
        exit(SQLITE_PROBLEM);
      }

      /*
       * We also need to update the last_time field for this mote:
       */
      check = sprintf(command, "UPDATE motes SET last_time=%d WHERE address=%d",
                      tm, tr_hdr->originaddr);

      if (check >= COMMAND_SIZE) {
        printf(
         "Query string overflow: %d characters written to buffer of size %d.",
         check, COMMAND_SIZE);
        sqlite3_close(connection);
        exit(OVERFLOW_ERROR);
      }

      ret = sqlite3_exec(connection, command, NULL, NULL, &err);
   
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
    }
  }
  /*
   * There are no else cases: messages which aren't Surge messages just go
   * right on by without being touched. If you want to be able to see what
   * else is going on, uncomment the following lines and comment out the
   * closing brace on the line above this comment.
   */
/*
    else {
      printf("Message destination (%d) is not the Surge module (%d).\n",
             tr_hdr->dst_pid, SURGE_MOD_PID);
    }
  }
  else {
    printf("Message (type %d) is not a tree routing message (type %d)\n",
           msg->type, MSG_TR_DATA_PKT);
  }
*/
}

