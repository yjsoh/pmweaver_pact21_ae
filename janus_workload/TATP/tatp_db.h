/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
         Aasheesh Kolli <akolli@umich.edu>


This file declares the TATP data base and the different transactions supported by
the database.
*/

#ifndef _TATP_DB_
#define _TATP_DB_
#include <pthread.h>
#include <atomic>
#include <string.h>
#include "tableEntries.h"

typedef uint64_t VALID_BIT_TYPE;
#define LOAD_THREADS 1

class TATP_DB
{

private:
  long total_subscribers; // Holds the number of subscribers
  int num_threads;
  subscriber_entry *subscriber_table;             // Pointer to the subscriber table
  access_info_entry *access_info_table;           // Pointer to the access info table
  special_facility_entry *special_facility_table; // Pointer to the special facility table
  call_forwarding_entry *call_forwarding_table;   // Pointer to the call forwarding table
  subscriber_entry *backup;
  VALID_BIT_TYPE *valid;
  unsigned long *subscriber_rndm_seeds;
  unsigned long *vlr_rndm_seeds;
  unsigned long *rndm_seeds;

public:
  pthread_mutex_t *lock_;            // Lock per subscriber to protect the update
  TATP_DB(unsigned num_subscribers); // Constructs and sizes tables as per num_subscribers
  ~TATP_DB();

  void initialize(unsigned num_subscribers, int n);

  void populate_tables(unsigned num_subscribers);   // Populates the various tables
  void populate_tables(unsigned from, unsigned to); // Populates the various tables

  // Fills subscriber table entry given subscriber id
  void fill_subscriber_entry(unsigned _s_id);
  // Fills access info table entry given subscriber id and ai_type
  void fill_access_info_entry(unsigned _s_id, short _ai_type);
  // Fills special facility table entry given subscriber id and sf_type
  void fill_special_facility_entry(unsigned _s_id, short _sf_type);
  // Fills call forwarding table entry given subscriber id, sf_type and start type
  void fill_call_forwarding_entry(unsigned _s_id, short _sf_type, short _start_time);

  void convert_to_string(unsigned number, int num_digits, char *string_ptr);
  void make_upper_case_string(char *string_ptr, int num_chars);

  // Tx: updates a random subscriber data
  void update_subscriber_data(int thread_id);
  void backup_location(int thread_id, long subId);
  void discard_backup(int thread_id, long subId);
  // Tx: updates location for a random subscriber
  void update_location(long subId, uint64_t vlr);
  // Tx: Inserts into call forwarding table for a random user
  void insert_call_forwarding(int thread_id);
  // Tx: Deletes call forwarding for a random user
  void delete_call_forwarding(int thread_id);

  unsigned long get_random(int thread_id, int min, int max);
  unsigned long get_random(int thread_id);
  unsigned long get_random_s_id(int thread_id);
  unsigned long get_random_vlr(int thread_id);

  void print_results();
};

#endif