/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
		 Aasheesh Kolli <akolli@umich.edu>

This file declares the tpcc database and the accesor transactions.
*/

#include "table_entries.h"
#include <atomic>
#include "simple_queue.h"
#include <pthread.h>
#include <cstdlib>

class TPCC_DB;
#ifdef _ENABLE_LIBPMEMOBJ
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/tx_base.h>
#include <libpmemobj++/transaction.hpp>
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)
extern pmem::obj::pool<TPCC_DB> pool;
extern pmem::obj::persistent_ptr<TPCC_DB> table;
#define PMDK_POOL_FILE "/mnt/ramdisk/tpcc.pmdk.pool"
#define PMDK_POOL_SIZE (1UL << 32)
#endif

#define N_DISTRICT_PER_WAREHOUSE 10
#define N_CUSTOMER_PER_DISTRICT 3000
#define N_ORDER_PER_DISTRICT 3000
#define N_ORDER_LINE_PER_ORDER 15
#define N_NEW_ORDER_PER_DISTRICT 900

typedef simple_queue queue_t;

struct backUpLog
{
	struct district_entry district_back;

	// fill_new_order_entry

	struct new_order_entry new_order_entry_back;

	// update_order_entry

	struct order_entry order_entry_back;

	// update_stock_entry

	struct stock_entry stock_entry_back[15];

	int fill_new_order_entry_indx = 0;
	int update_order_entry_indx = 0;
	int update_stock_entry_indx[16];

	uint64_t district_back_valid;
	uint64_t fill_new_order_entry_back_valid;
	uint64_t update_order_entry_back_valid;
	uint64_t update_stock_entry_num_valid;
	uint64_t fill_new_order_line_entry_valid;

	// global log valid
	uint64_t log_valid;
};

class TPCC_DB
{

private:
	// Tables with size dependent on num warehouses
	short num_warehouses;
	uint64_t num_items;
	short random_3000[3000];
#ifdef _ENABLE_LIBPMEMOBJ
	pmem::obj::persistent_ptr<warehouse_entry[]> warehouse;
	pmem::obj::persistent_ptr<district_entry[]> district;
	pmem::obj::persistent_ptr<customer_entry[]> customer;
	pmem::obj::persistent_ptr<stock_entry[]> stock;
	pmem::obj::persistent_ptr<history_entry[]> history;
	pmem::obj::persistent_ptr<order_entry[]> order;
	pmem::obj::persistent_ptr<new_order_entry[]> new_order;
	pmem::obj::persistent_ptr<order_line_entry[]> order_line;
	pmem::obj::persistent_ptr<item_entry[]> item;
#else
	warehouse_entry *warehouse;
	district_entry *district;
	customer_entry *customer;
	stock_entry *stock;

	// Tables with slight variation in sizes (due to inserts/deletes etc.)
	history_entry *history;
	order_entry *order;
	new_order_entry *new_order;
	order_line_entry *order_line;

	// Fixed size table
	item_entry *item;
#endif
	unsigned long *rndm_seeds;

	queue_t *perTxLocks;		   // Array of queues of locks held by active Tx
	pthread_mutex_t *locks;		   // Array of locks held by the TxEngn. RDSs acquire locks through the TxEngn
	struct backUpLog **backUpInst; // Pointer to per-thread log location
	unsigned g_seed;

public:
	TPCC_DB(uint64_t nwarehouse, uint64_t nitems);
	~TPCC_DB();

	void initialize(uint64_t nthreads, uint64_t nwarehouse, uint64_t nitems);
	void deinitialize();

	/* Populate table */
	void populate_tables();
	void fill_item_entry(int _i_id);
	void fill_warehouse_entry(int _w_id);
	void fill_stock_entry(int _s_w_id, int s_i_id);
	void fill_district_entry(int _d_w_id, int _d_id);
	void fill_customer_entry(int _c_w_id, int _c_d_id, int _c_id);
	void fill_history_entry(int _h_c_w_id, int _h_c_d_id, int _h_c_id);
	void fill_order_entry(int _o_w_id, int _o_d_id, int _o_id);
	void fill_order_line_entry(int _ol_w_id, int _ol_d_id, int _ol_o_id, int _o_ol_cnt, long long _o_entry_d);
	void fill_new_order_entry(int _no_w_id, int _no_d_id, int _no_o_id, int tid);
	void fill_new_order_line_entry(int tid, int _ol_w_id, int _ol_d_id, int _ol_o_id, int ol_num, int ol_i_id);
	void fill_time(long long &time_slot);

	/* Random related */
	int rand_local(int min, int max);
	void random_a_string(int min, int max, char *string_ptr);
	void random_a_original_string(int min, int max, int probability, char *string_ptr);
	void random_n_string(int min, int max, char *string_ptr);
	void random_zip(char *string_ptr);
	unsigned fastrand();
	unsigned long get_random(int thread_id, int min, int max);
	unsigned long get_random(int thread_id);

	/* Memcpy wrappers */
	void copy_district_info(district_entry &dest, district_entry &source);
	void copy_customer_info(customer_entry &dest, customer_entry &source);
	void copy_new_order_info(new_order_entry &dest, new_order_entry &source);
	void copy_order_info(order_entry &dest, order_entry &source);
	void copy_stock_info(stock_entry &dest, stock_entry &source);
	void copy_order_line_info(order_line_entry &dest, order_line_entry &source);

	/* Actual Transactions */
	void new_order_tx(int tid, int w_id, int d_id, int c_id, int *item_ids, int ol_cnt);
	void update_order_entry(int tid, int _w_id, short _d_id, int _o_id, int _c_id, int _ol_cnt);
	void update_stock_entry(int tid, int _w_id, int _i_id, int _d_id, float &amount, int itr);

	/* Multi-threading */
	void acquire_locks(int thread_id, queue_t &reqLocks);
	void release_locks(int thread_id);

	/* Debug Related */
	void printStackPointer(int *sp, int thread_id);
};
