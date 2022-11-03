/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
		 Aasheesh Kolli <akolli@umich.edu>


This file defines the various functions of the tpcc database
*/

#include <cstdlib>
#include <iostream>
#include <queue>
#include <cstring>	 // For memcpy
#include <algorithm> // for sort
#include "tpcc_db.h"
#include "common.h"
//#define NEW_ORDER_LOCK 10;
#define TPCC_DEBUG 0
#define NUM_RNDM_SEEDS 1280

TPCC_DB::TPCC_DB(uint64_t nwarehouse, uint64_t nitems) : num_warehouses(nwarehouse), num_items(nitems)
{
	// std::cout << "Entering " << __FUNCTION__ << std::endl;
	uint64_t district_back_valid = 0UL;
	uint64_t fill_new_order_entry_back_valid = 0UL;
	uint64_t update_order_entry_back_valid = 0UL;
	uint64_t update_stock_entry_num_valid = 0UL;
	uint64_t log_valid = 0UL;
	g_seed = 1312515;
}

void TPCC_DB::initialize(uint64_t nthreads, uint64_t nwarehouse, uint64_t nitems)
{
	uint64_t num_districts = N_DISTRICT_PER_WAREHOUSE * nwarehouse;
	uint64_t num_customers = N_CUSTOMER_PER_DISTRICT * num_districts;
	uint64_t num_stocks = nwarehouse * nitems;
	uint64_t num_histories = num_customers;
	uint64_t num_orders = N_ORDER_PER_DISTRICT * num_districts;
	uint64_t num_order_lines = N_ORDER_LINE_PER_ORDER * num_orders; // Max possible, average is 10*num_orders
	uint64_t num_new_orders = N_NEW_ORDER_PER_DISTRICT * num_districts;

	uint64_t total_mem = 0;
	total_mem += (nwarehouse * sizeof(warehouse_entry));
	total_mem += (num_districts * sizeof(district_entry));
	total_mem += (num_customers * sizeof(customer_entry));
	total_mem += (num_stocks * sizeof(stock_entry));
	total_mem += (nitems * sizeof(item_entry));
	total_mem += (num_histories * sizeof(history_entry));
	total_mem += (num_orders * sizeof(order_entry));
	total_mem += (num_new_orders * sizeof(new_order_entry));
	total_mem += (num_order_lines * sizeof(order_line_entry));
	total_mem += (nthreads * sizeof(struct backUpLog));
	for (uint64_t i = 0; i < nthreads; i++)
	{
		total_mem += (sizeof(struct backUpLog));
	}
	std::cout << "Total Allocated Memory: " << total_mem / (1UL << 20) << " MB \n";

	init_pmalloc(total_mem);
	warehouse = (warehouse_entry *)pmalloc(nwarehouse * sizeof(warehouse_entry));
	district = (district_entry *)pmalloc(num_districts * sizeof(district_entry));
	customer = (customer_entry *)pmalloc(num_customers * sizeof(customer_entry));
	stock = (stock_entry *)pmalloc(num_stocks * sizeof(stock_entry));
	item = (item_entry *)pmalloc(nitems * sizeof(item_entry));
	history = (history_entry *)pmalloc(num_histories * sizeof(history_entry));
	order = (order_entry *)pmalloc(num_orders * sizeof(order_entry));
	order_line = (order_line_entry *)pmalloc(num_order_lines * sizeof(order_line_entry));
	new_order = (new_order_entry *)pmalloc(num_new_orders * sizeof(new_order_entry));
	backUpInst = (struct backUpLog **)pmalloc(nthreads * sizeof(struct backUpLog));
	for (uint64_t i = 0; i < nthreads; i++)
	{
		backUpInst[i] = (struct backUpLog *)pmalloc(sizeof(struct backUpLog));
	}

	for (int i = 0; i < 3000; i++)
	{
		random_3000[i] = i;
	}
	for (int i = 0; i < 3000; i++)
	{
		int rand_loc = fastrand() % 3000;
		int temp = random_3000[i];
		random_3000[i] = random_3000[rand_loc];
		random_3000[rand_loc] = temp;
	}
	/*
	perTxLocks = new queue_t[numThreads];
	for(int i=0; i<numThreads; i++) {
	  perTxLocks[i].push(0);
	  perTxLocks[i].pop();
	}
	*/
	/*
	locks = new pthread_mutex_t[numLocks];
	for (int i = 0; i < numLocks; i++) {
		pthread_mutex_init(&locks[i],NULL);
	}
	*/
	rndm_seeds = new unsigned long[NUM_RNDM_SEEDS];
	for (int i = 0; i < NUM_RNDM_SEEDS; i++)
	{
		srand(i);
		rndm_seeds[i] = rand_local(1, NUM_RNDM_SEEDS * 10);
	}
}

void TPCC_DB::deinitialize()
{
	// TODO
}

TPCC_DB::~TPCC_DB()
{
#ifdef _VOLATILE_TPCC_DB
	free(warehouse);
	free(district);
	free(customer);
	free(stock);
	free(item);
	free(history);
	free(order);
	free(new_order);
	free(order_line);
#endif
}

void TPCC_DB::populate_tables()
{
	// std::cout << "Entering " << __FUNCTION__ << std::endl;
	//// std::cout<<"populating item table"<<std::endl;
	for (int i = 0; i < num_items; i++)
	{
		fill_item_entry(i + 1);
	}
	//// std::cout<<"finished populating item table"<<std::endl;

	for (int i = 0; i < num_warehouses; i++)
	{
		fill_warehouse_entry(i + 1);
		for (int j = 0; j < num_items; j++)
		{
			fill_stock_entry(i + 1, j + 1);
		}
		//// std::cout<<"finished populating stock table"<<std::endl;
		for (int j = 0; j < N_DISTRICT_PER_WAREHOUSE; j++)
		{
			fill_district_entry(i + 1, j + 1);
			for (int k = 0; k < N_CUSTOMER_PER_DISTRICT; k++)
			{
				fill_customer_entry(i + 1, j + 1, k + 1);
				fill_history_entry(i + 1, j + 1, k + 1);
				fill_order_entry(i + 1, j + 1, k + 1);
			}
			for (int k = 2100; k < 3000; k++)
			{
				fill_new_order_entry(i + 1, j + 1, k + 1, 0);
			}
		}
	}
}

void TPCC_DB::fill_item_entry(int _i_id)
{
	int indx = (_i_id - 1);
	item[indx].i_id = _i_id;
	item[indx].i_im_id = rand_local(1, num_items);
	random_a_string(14, 24, item[indx].i_name);
	item[indx].i_price = rand_local(1, 100) * (1.0);
	random_a_original_string(26, 50, 10, item[indx].i_data);
}

void TPCC_DB::fill_warehouse_entry(int _w_id)
{
	int indx = (_w_id - 1);
	warehouse[indx].w_id = _w_id;
	random_a_string(6, 10, warehouse[indx].w_name);
	random_a_string(10, 20, warehouse[indx].w_street_1);
	random_a_string(10, 20, warehouse[indx].w_street_2);
	random_a_string(10, 20, warehouse[indx].w_city);
	random_a_string(2, 2, warehouse[indx].w_state);
	random_zip(warehouse[indx].w_zip);
	warehouse[indx].w_tax = (rand_local(0, 20)) / 100.0;
	warehouse[indx].w_ytd = 300000.0;
}

void TPCC_DB::fill_stock_entry(int _s_w_id, int _s_i_id)
{
	//// std::cout<<"entered fill stock entry: "<<_s_w_id<<", "<<_s_i_id<<std::endl;
	int indx = (_s_w_id - 1) * num_items + (_s_i_id - 1);
	stock[indx].s_i_id = _s_i_id;
	//// std::cout<<"1"<<std::endl;
	stock[indx].s_w_id = _s_w_id;
	//// std::cout<<"1"<<std::endl;
	stock[indx].s_quantity = rand_local(10, 100);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_01);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_02);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_03);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_04);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_05);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_06);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_07);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_08);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_09);
	//// std::cout<<"1"<<std::endl;
	random_a_string(24, 24, stock[indx].s_dist_10);
	//// std::cout<<"1"<<std::endl;
	stock[indx].s_ytd = 0.0;
	//// std::cout<<"1"<<std::endl;
	stock[indx].s_order_cnt = 0.0;
	//// std::cout<<"1"<<std::endl;
	stock[indx].s_remote_cnt = 0.0;
	//// std::cout<<"1"<<std::endl;
	random_a_original_string(26, 50, 10, stock[indx].s_data);
	//// std::cout<<"exiting fill stock entry: "<<_s_w_id<<", "<<_s_i_id<<std::endl;
}

void TPCC_DB::fill_district_entry(int _d_w_id, int _d_id)
{
	int indx = (_d_w_id - 1) * 10 + (_d_id - 1);
	district[indx].d_id = _d_id;
	district[indx].d_w_id = _d_w_id;
	random_a_string(6, 10, district[indx].d_name);
	random_a_string(10, 20, district[indx].d_street_1);
	random_a_string(10, 20, district[indx].d_street_2);
	random_a_string(10, 20, district[indx].d_city);
	random_a_string(2, 2, district[indx].d_state);
	random_zip(district[indx].d_zip);
	district[indx].d_tax = (rand_local(0, 20)) / 100.0;
	district[indx].d_ytd = 30000.0;
	district[indx].d_next_o_id = 3001;
}

void TPCC_DB::fill_customer_entry(int _c_w_id, int _c_d_id, int _c_id)
{
	int indx = (_c_w_id - 1) * 10 * 3000 + (_c_d_id - 1) * 3000 + (_c_id - 1);
	customer[indx].c_id = _c_id;
	customer[indx].c_d_id = _c_d_id;
	customer[indx].c_w_id = _c_w_id;
	random_a_string(16, 16, customer[indx].c_last); // FIXME: check tpcc manual for exact setting
	customer[indx].c_middle[0] = 'O';
	customer[indx].c_middle[1] = 'E';
	random_a_string(8, 16, customer[indx].c_first);
	random_a_string(10, 20, customer[indx].c_street_1);
	random_a_string(10, 20, customer[indx].c_street_2);
	random_a_string(10, 20, customer[indx].c_city);
	random_a_string(2, 2, customer[indx].c_state);
	random_zip(customer[indx].c_zip);
	random_n_string(16, 16, customer[indx].c_phone);
	fill_time(customer[indx].c_since);
	if (fastrand() % 10 < 1)
	{
		customer[indx].c_credit[0] = 'G';
		customer[indx].c_credit[1] = 'C';
	}
	else
	{
		customer[indx].c_credit[0] = 'B';
		customer[indx].c_credit[1] = 'C';
	}
	customer[indx].c_credit_lim = 50000.0;
	customer[indx].c_discount = (rand_local(0, 50)) / 100.0;
	customer[indx].c_balance = -10.0;
	customer[indx].c_ytd_payment = 10.0;
	customer[indx].c_payment_cnt = 1.0;
	customer[indx].c_delivery_cnt = 0.0;
	random_a_string(300, 500, customer[indx].c_data);
}

void TPCC_DB::fill_history_entry(int _h_c_w_id, int _h_c_d_id, int _h_c_id)
{
	int indx = (_h_c_w_id - 1) * 10 * 3000 + (_h_c_d_id - 1) * 3000 + (_h_c_id - 1);
	history[indx].h_c_id = _h_c_id;
	history[indx].h_c_d_id = _h_c_d_id;
	history[indx].h_c_w_id = _h_c_w_id;
	fill_time(history[indx].h_date);
	history[indx].h_amount = 10.0;
	random_a_string(12, 24, history[indx].h_data);
}

void TPCC_DB::fill_order_entry(int _o_w_id, int _o_d_id, int _o_id)
{
	int indx = (_o_w_id - 1) * 10 * 3000 + (_o_d_id - 1) * 3000 + (_o_id - 1);
	order[indx].o_id = _o_id;
	order[indx].o_c_id = random_3000[_o_id];
	order[indx].o_d_id = _o_d_id;
	order[indx].o_w_id = _o_w_id;
	fill_time(order[indx].o_entry_d);
	if (_o_id < 2101)
		order[indx].o_carrier_id = fastrand() % 10 + 1;
	else
		order[indx].o_carrier_id = 0;
	order[indx].o_ol_cnt = rand_local(5, 15);
	order[indx].o_all_local = 1.0;
	for (int i = 0; i < order[indx].o_ol_cnt; i++)
	{
		fill_order_line_entry(_o_w_id, _o_d_id, _o_id, i, order[indx].o_entry_d);
	}
}

void TPCC_DB::fill_order_line_entry(int _ol_w_id, int _ol_d_id, int _ol_o_id, int _o_ol_cnt, long long _o_entry_d)
{
	int indx = (_ol_w_id - 1) * 10 * 3000 * 15 + (_ol_d_id - 1) * 3000 * 15 + (_ol_o_id - 1) * 15 + _o_ol_cnt;
	order_line[indx].ol_o_id = _ol_o_id;
	order_line[indx].ol_d_id = _ol_d_id;
	order_line[indx].ol_w_id = _ol_w_id;
	order_line[indx].ol_number = _o_ol_cnt;
	order_line[indx].ol_i_id = rand_local(1, num_items);
	order_line[indx].ol_supply_w_id = _ol_w_id;
	if (_ol_o_id < 2101)
	{
		order_line[indx].ol_delivery_d = _o_entry_d;
		order_line[indx].ol_amount = 0.0;
	}
	else
	{
		order_line[indx].ol_delivery_d = 0;
		order_line[indx].ol_amount = rand_local(1, 999999) / 100.0;
	}
	order_line[indx].ol_quantity = 5.0;
	random_a_string(24, 24, order_line[indx].ol_dist_info);
}

void TPCC_DB::fill_new_order_entry(int _no_w_id, int _no_d_id, int _no_o_id, int tid)
{
	int indx = (_no_w_id - 1) * 10 * 900 + (_no_d_id - 1) * 900 + (_no_o_id - 2101) % 900;
	indx = indx < 0 ? -indx : indx;
	indx = indx % (num_warehouses * 10 * 900);

	// do backup
#ifdef _ENABLE_LOGGING
	backUpInst[tid]->fill_new_order_entry_indx = indx;
	copy_new_order_info(backUpInst[tid]->new_order_entry_back, new_order[indx]);
	flush_caches((void *)&backUpInst[tid]->fill_new_order_entry_indx, (unsigned)sizeof(backUpInst[tid]->fill_new_order_entry_indx));
	flush_caches((void *)&backUpInst[tid]->new_order_entry_back, (unsigned)sizeof(backUpInst[tid]->new_order_entry_back));
	s_fence();
	backUpInst[tid]->fill_new_order_entry_back_valid = 1;
	flush_caches(&backUpInst[tid]->fill_new_order_entry_back_valid, sizeof(backUpInst[tid]->fill_new_order_entry_back_valid));
	s_fence();
#endif

	new_order[indx].no_o_id = _no_o_id;
	new_order[indx].no_d_id = _no_d_id;
	new_order[indx].no_w_id = _no_w_id;
	flush_caches((void *)&new_order[indx], (unsigned)sizeof(new_order[indx]));
	s_fence();
}

void TPCC_DB::fill_new_order_line_entry(int tid, int _ol_w_id, int _ol_d_id, int _ol_o_id, int ol_num, int ol_i_id)
{
	int indx = (_ol_w_id - 1) * N_DISTRICT_PER_WAREHOUSE * N_ORDER_PER_DISTRICT * N_ORDER_LINE_PER_ORDER + (_ol_d_id - 1) * N_ORDER_PER_DISTRICT * N_ORDER_LINE_PER_ORDER + (_ol_o_id - 1) * N_ORDER_LINE_PER_ORDER + ol_num;
	order_line[indx].ol_o_id = _ol_o_id;
	order_line[indx].ol_d_id = _ol_d_id;
	order_line[indx].ol_w_id = _ol_w_id;
	order_line[indx].ol_number = ol_num;
	order_line[indx].ol_i_id = ol_i_id;
	order_line[indx].ol_supply_w_id = _ol_w_id;
	order_line[indx].ol_delivery_d = 0;
	order_line[indx].ol_amount = rand_local(1, 999999) / 100.0;
	order_line[indx].ol_quantity = 5.0;
	random_a_string(24, 24, order_line[indx].ol_dist_info);

	backUpInst[tid]->fill_new_order_line_entry_valid = indx;
	flush_caches(&backUpInst[tid]->fill_new_order_line_entry_valid, sizeof(backUpInst[tid]->fill_new_order_line_entry_valid));
	s_fence();
}

void TPCC_DB::fill_time(long long &time_slot)
{
	// FIXME: put correct time
	time_slot = 12112342433241;
}

/* Random related */
unsigned TPCC_DB::fastrand()
{
	// g_seed = (179423891 * g_seed + 2038073749);
	// return (g_seed >> 8) & 0x7FFFFFFF;
	return rand();
}

int TPCC_DB::rand_local(int min, int max)
{
	return (min + (fastrand() % (max - min + 1)));
}

void TPCC_DB::random_a_string(int min, int max, char *string_ptr)
{
	//// std::cout<<"entered random a string"<<std::endl;
	char alphabets[26] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
						  'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
	//// std::cout<<"2"<<std::endl;
	int string_length = min + (fastrand() % (max - min + 1));
	//// std::cout<<"2"<<std::endl;
	for (int i = 0; i < string_length; i++)
	{
		string_ptr[max - 1 - i] = alphabets[fastrand() % 26];
		//// std::cout<<"f3"<<std::endl;
	}
	//// std::cout<<"2"<<std::endl;
	for (int i = 0; i < max - string_length; i++)
	{
		string_ptr[max - 1 - i] = ' ';
		//// std::cout<<"f4"<<std::endl;
	}
	//// std::cout<<"exiting random a string"<<std::endl;
}

void TPCC_DB::random_a_original_string(int min, int max, int probability, char *string_ptr)
{
	// FIXME: use probability and add ORIGINAL
	random_a_string(min, max, string_ptr);
}

void TPCC_DB::random_n_string(int min, int max, char *string_ptr)
{
	char digits[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
	int string_length = min + (fastrand() % (max - min + 1));
	for (int i = 0; i < string_length; i++)
	{
		string_ptr[max - 1 - i] = digits[fastrand() % 10];
	}
	for (int i = 0; i < max - string_length; i++)
	{
		string_ptr[max - 1 - i] = ' ';
	}
}

void TPCC_DB::random_zip(char *string_ptr)
{
	random_a_string(4, 4, string_ptr);
	for (int i = 4; i < 9; i++)
	{
		string_ptr[i] = '1';
	}
}

unsigned long TPCC_DB::get_random(int tid)
{
	unsigned long tmp;
	tmp = rndm_seeds[tid * 10] = (rndm_seeds[tid * 10] * 16807) % 2147483647;
	// return rand()%(2^32-1);
	return tmp;
}

unsigned long TPCC_DB::get_random(int tid, int min, int max)
{
	unsigned long tmp;
	// return min+(rand()%(max-min+1));
	tmp = rndm_seeds[tid * 10] = (rndm_seeds[tid * 10] * 16807) % 2147483647;
	return min + (tmp % (max - min + 1));
	// return tmp
}

/* Copy Wrappers */
void TPCC_DB::copy_district_info(district_entry &dest, district_entry &source)
{
	memcpy(&dest, &source, sizeof(district_entry));
}

void TPCC_DB::copy_customer_info(customer_entry &dest, customer_entry &source)
{
	memcpy(&dest, &source, sizeof(customer_entry));
}

void TPCC_DB::copy_new_order_info(new_order_entry &dest, new_order_entry &source)
{
	memcpy(&dest, &source, sizeof(new_order_entry));
}

void TPCC_DB::copy_order_info(order_entry &dest, order_entry &source)
{
	memcpy(&dest, &source, sizeof(order_entry));
}

void TPCC_DB::copy_stock_info(stock_entry &dest, stock_entry &source)
{
	memcpy(&dest, &source, sizeof(stock_entry));
}

void TPCC_DB::copy_order_line_info(order_line_entry &dest, order_line_entry &source)
{
	memcpy(&dest, &source, sizeof(order_line_entry));
}

/* Transactions*/

void TPCC_DB::new_order_tx(int tid, int w_id, int d_id, int c_id)
{

	// warehouse
	int w_indx = (w_id - 1);

	// district
	int d_indx = w_indx * N_DISTRICT_PER_WAREHOUSE + (d_id - 1);
	int d_o_id = district[d_indx].d_next_o_id % 15;
	/*
	queue_t reqLocks;
	reqLocks.push(d_indx); // Lock for district
	*/
	/*
	if(TPCC_DEBUG)
	  // std::cout<<"**NOTx** district lock id: "<<d_indx<<std::endl;
	*/

	// customer
	int c_indx = d_indx * N_CUSTOMER_PER_DISTRICT + (c_id - 1);

	// new order
	int no_indx = d_indx * N_NEW_ORDER_PER_DISTRICT + (d_o_id - 2101) % N_NEW_ORDER_PER_DISTRICT;

	// order
	int o_indx = d_indx * N_ORDER_PER_DISTRICT + (d_o_id - 1) % N_ORDER_PER_DISTRICT;

	// Create orderline
	int ol_cnt = get_random(tid, 5, 15);
	int item_ids[15];
	for (int i = 0; i < ol_cnt; i++)
	{
		int new_item_id;
		bool match;
		do
		{
			match = false;
			new_item_id = get_random(tid, 1, num_items);
			for (int j = 0; j < i; j++)
			{
				if (new_item_id == item_ids[j])
				{
					match = true;
					break;
				}
			}
		} while (match);
		item_ids[i] = new_item_id;
	}

	std::sort(item_ids, item_ids + ol_cnt);
	/*
	if(TPCC_DEBUG)
	  // std::cout<<"**NOTx** ol_cnt: "<<ol_cnt<<std::endl;
	// */
	// for (int i = 0; i < ol_cnt; i++)
	// {
	// 	int item_lock_id = num_warehouses * 10 + (w_id - 1) * num_items + item_ids[i] - 1;
	// 	/*
	// 	reqLocks.push(item_lock_id); // Lock for each item in stock table
	// 	*/
	// 	/*
	// 	if(TPCC_DEBUG)
	// 	  // std::cout<<"**NOTx** item lock id: "<<item_lock_id<<" thread id: "<<tid<<std::endl;
	// 	*/
	// }
	// Korakit
	// remove MT stuff
	// acquire_locks(tid, reqLocks);
	/*
	if(TPCC_DEBUG)
	  // std::cout<<"**NOTx** finished start tx: "<<std::endl;
	*/

	// float w_tax = warehouse[w_indx].w_tax;
	// float d_tax = district[d_indx].d_tax;
	// int d_o_id = district[d_indx].d_next_o_id;
	// int no_indx = (w_id - 1) * 10 * 900 + (d_id - 1) * 900 + (d_o_id - 2101) % 900;

	// int o_indx = (w_id - 1) * 10 * 3000 + (d_id - 1) * 3000 + (d_o_id - 1) % 3000;

	// Korakit
	// real stuff here
	//  okay we gonna try really simple stuff first
	//  let's force all writes when the transaction completes
	//   flush_caches(uint64_t addr, unsigned size);
	//   s_fence();

#ifdef _ENABLE_LOGGING
	// prepare backup log
	backUpInst[tid]->district_back_valid = 0;
	backUpInst[tid]->fill_new_order_entry_back_valid = 0;
	backUpInst[tid]->fill_new_order_line_entry_valid = 0;
	backUpInst[tid]->update_order_entry_back_valid = 0;
	backUpInst[tid]->update_stock_entry_num_valid = 0;
	flush_caches(&backUpInst[tid]->district_back_valid, sizeof(backUpInst[tid]->district_back_valid));
	flush_caches(&backUpInst[tid]->fill_new_order_line_entry_valid, sizeof(backUpInst[tid]->fill_new_order_line_entry_valid));
	flush_caches(&backUpInst[tid]->fill_new_order_entry_back_valid, sizeof(backUpInst[tid]->fill_new_order_entry_back_valid));
	flush_caches(&backUpInst[tid]->update_order_entry_back_valid, sizeof(backUpInst[tid]->update_order_entry_back_valid));
	flush_caches(&backUpInst[tid]->update_stock_entry_num_valid, sizeof(backUpInst[tid]->update_stock_entry_num_valid));
	s_fence();
	backUpInst[tid]->log_valid = 1;
	flush_caches((void *)&backUpInst[tid]->log_valid, (unsigned)sizeof(backUpInst[tid]->log_valid));
	s_fence();

	// do backup
	copy_district_info(backUpInst[tid]->district_back, district[d_indx]);
	flush_caches(&backUpInst[tid]->district_back, sizeof(backUpInst[tid]->district_back));
	flush_caches((void *)&district[d_indx].d_next_o_id, (unsigned)sizeof(district[d_indx].d_next_o_id));
	s_fence();
#endif

	district[d_indx].d_next_o_id++;

	// ~= create_neworder
	fill_new_order_entry(w_id, d_id, d_o_id, tid);

	// ~= create_order
	update_order_entry(tid, w_id, d_id, d_o_id, c_id, ol_cnt);

	float total_amount = 0.0;
	for (int i = 0; i < ol_cnt; i++)
	{
		// ~= modify_stock
		update_stock_entry(tid, w_id, item_ids[i], d_id, total_amount, i);

		fill_new_order_line_entry(tid, w_id, d_id, d_o_id, i, item_ids[i]);
	}

#ifdef _ENABLE_LOGGING
	// invalidate log entries
	backUpInst[tid]->log_valid = 0;
	flush_caches((void *)&backUpInst[tid]->log_valid, (unsigned)sizeof(backUpInst[tid]->log_valid));
	s_fence();
#endif

	// Korakit
	// debug removed
	/*
	  if(TPCC_DEBUG)
		// std::cout<<"d_id, d_o_id, ol_cnt, total_amount: "<<d_id<<", "<<d_o_id<<", "<<
													ol_cnt<<", "<<total_amount<<std::endl;
	*/
	// Korakit
	// remove MT stuffs
	// release_locks(tid);

	return;
}

void TPCC_DB::update_order_entry(int tid, int _w_id, short _d_id, int _o_id, int _c_id, int _ol_cnt)
{
	int indx = (_w_id - 1) * N_DISTRICT_PER_WAREHOUSE * N_ORDER_PER_DISTRICT + (_d_id - 1) * N_ORDER_PER_DISTRICT + (_o_id - 1) % N_ORDER_PER_DISTRICT;
	// indx = indx < 0 ? -indx : indx;
	// indx = indx % (num_warehouses * 10 * 900);
	// Korakit
	// create backup
#ifdef _ENABLE_LOGGING
	backUpInst[tid]->update_order_entry_indx = indx;
	backUpInst[tid]->order_entry_back = order[indx];
	flush_caches((void *)&backUpInst[tid]->update_order_entry_indx, (unsigned)sizeof(backUpInst[tid]->update_order_entry_indx));
	flush_caches((void *)&backUpInst[tid]->order_entry_back, (unsigned)sizeof(backUpInst[tid]->order_entry_back));
	s_fence();

	backUpInst[tid]->update_order_entry_back_valid = 1;
	flush_caches((void *)&backUpInst[tid]->update_order_entry_back_valid, sizeof(backUpInst[tid]->update_order_entry_back_valid));
	s_fence();
#endif

	order[indx].o_id = _o_id;
	order[indx].o_d_id = _d_id;
	order[indx].o_w_id = _w_id;
	order[indx].o_c_id = _c_id;
	fill_time(order[indx].o_entry_d);
	order[indx].o_carrier_id = 0;
	order[indx].o_ol_cnt = _ol_cnt;
	order[indx].o_all_local = 1;
	flush_caches((void *)&order[indx], (unsigned)sizeof(order[indx]));
	s_fence();
}

void TPCC_DB::update_stock_entry(int tid, int _w_id, int _i_id, int _d_id, float &amount, int itr)
{
	int indx = (_w_id - 1) * num_items + _i_id - 1;

	// int ol_quantity = get_random(tid, 1, 10);
	int ol_quantity = 7;
#ifdef _ENABLE_LOGGING
	backUpInst[tid]->update_stock_entry_indx[itr] = indx;
	copy_stock_info(backUpInst[tid]->stock_entry_back[itr], stock[indx]);
	backUpInst[tid]->update_stock_entry_num_valid |= (1UL << (itr + 1)); // bitset
	flush_caches((void *)&backUpInst[tid]->update_stock_entry_indx[itr], (unsigned)sizeof(backUpInst[tid]->update_stock_entry_indx[itr]));
	flush_caches((void *)&backUpInst[tid]->stock_entry_back[itr], (unsigned)sizeof(backUpInst[tid]->stock_entry_back[itr]));
	flush_caches((void *)&backUpInst[tid]->update_stock_entry_num_valid, (unsigned)sizeof(backUpInst[tid]->update_stock_entry_num_valid));
	s_fence();
#endif

	if (stock[indx].s_quantity - ol_quantity > 10)
	{
		stock[indx].s_quantity -= ol_quantity;
	}
	else
	{
		stock[indx].s_quantity -= ol_quantity;
		stock[indx].s_quantity += 91;
	}

	stock[indx].s_ytd += ol_quantity;
	stock[indx].s_order_cnt += 1;
	flush_caches((void *)&stock[indx], (unsigned)sizeof(stock[indx]));
	s_fence();

	// Korakit
	// volatile
	amount += ol_quantity * item[_i_id - 1].i_price;
}

/* Multi-threading */
void TPCC_DB::acquire_locks(int tid, queue_t &requestedLocks)
{
	// Acquire locks in order.
	int i = -1;
	while (!requestedLocks.empty())
	{
		i = requestedLocks.front();
		perTxLocks[tid].push(i);
		requestedLocks.pop();
		pthread_mutex_lock(&locks[i]);
	}
}

void TPCC_DB::release_locks(int tid)
{

	// Release locks in order
	int i = -1;
	while (!perTxLocks[tid].empty())
	{
		i = perTxLocks[tid].front();
		perTxLocks[tid].pop();
		pthread_mutex_unlock(&locks[i]);
	}
}

/* Debug Related */
void TPCC_DB::printStackPointer(int *sp, int tid)
{
	std::cout << "Stack Heap: " << sp << std::endl;
}
