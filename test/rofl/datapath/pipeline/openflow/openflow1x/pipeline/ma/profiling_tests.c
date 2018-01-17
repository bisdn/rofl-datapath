#include "profiling_tests.h"
#include "rofl/datapath/pipeline/openflow/of_switch_pp.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include "CUnit/Basic.h"
#include "profiling_tests.h"


static of1x_switch_t* sw=NULL;
static datapacket_t pkt;
static uint64_t accumulated_time;

static int setup=0;

#define NUM_OF_ITERATONS 100000

int set_up(){

	of1x_flow_entry_t* entry;
	uint32_t port_in=1;

	physical_switch_init();

	if(!setup){
		enum of1x_matching_algorithm_available ma_list[4]={of1x_loop_matching_algorithm, of1x_loop_matching_algorithm,
		of1x_loop_matching_algorithm, of1x_loop_matching_algorithm};

		//Create instance
		sw = of1x_init_switch("Test switch", OF_VERSION_12, SW_FLAVOR_GENERIC, 0x0101,4,ma_list);
	}else{
		enum of1x_matching_algorithm_available ma_list[4]={of1x_trie_matching_algorithm, of1x_trie_matching_algorithm,
		of1x_trie_matching_algorithm, of1x_trie_matching_algorithm};

		//Create instance
		sw = of1x_init_switch("Test switch", OF_VERSION_12, SW_FLAVOR_GENERIC, 0x0101,4,ma_list);
	}

	if(!sw)
		return EXIT_FAILURE;

	//Set PORT_IN_MATCH
	entry = of1x_init_flow_entry(false, /*builtin=*/false);
	of1x_add_match_to_entry(entry,of1x_init_port_in_match(port_in));
	of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false);

	setup=1;

	return EXIT_SUCCESS;
}

int tear_down(){
	//Destroy the switch
	if(__of1x_destroy_switch(sw) != ROFL_SUCCESS)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

//tmp val
extern uint128__t tmp_val;

//Single flow_mod profiling
void reset(){
	tear_down();
	set_up();
}

//Single flow_mod profiling
void profile_basic_match(bool lock){

	int i;
	uint64_t tics;
	uint32_t average_tics;
	unsigned int tid = (lock) ? ROFL_PIPELINE_LOCKED_TID  : 1;

	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 1);

	//PKT
	*((uint32_t*)&tmp_val) = 1;

	//Execute
	for(i=0, accumulated_time=0;i<NUM_OF_ITERATONS;i++){
		//Measure time
		tics = rdtsc();

		//Process
		of_process_packet_pipeline(tid, (const struct of_switch *)sw, &pkt);

		//Accumulate
		accumulated_time += rdtsc() - tics;
	}

	//Calculate average
	average_tics = accumulated_time / NUM_OF_ITERATONS;

	//Print
	fprintf(stderr,"\n%s MATCH num_of_iterations: %u average cycles: %u\n",  __func__, NUM_OF_ITERATONS, average_tics);
	//TODO output results
}

void profile_basic_no_match(bool lock){

	int i;
	uint64_t tics;
	uint32_t average_tics;
	unsigned int tid = (lock) ? ROFL_PIPELINE_LOCKED_TID  : 1;

	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 1);

	//PKT
	*((uint32_t*)&tmp_val) = 2;

	//Execute
	for(i=0, accumulated_time=0;i<NUM_OF_ITERATONS;i++){
		//Measure time
		tics = rdtsc();

		//Process
		of_process_packet_pipeline(tid, (const struct of_switch *)sw, &pkt);

		//Accumulate
		accumulated_time += rdtsc() - tics;
	}

	//Calculate average
	average_tics = accumulated_time / NUM_OF_ITERATONS;

	//Print
	fprintf(stderr,"\n%s NO-MATCH num_of_iterations: %u average cycles: %u\n",  __func__, NUM_OF_ITERATONS, average_tics);

	//TODO output results
}

/* Test cases */
void profile_basic_match_lock(void){
	profile_basic_match(true);
}

void profile_basic_no_match_lock(void){
	profile_basic_no_match(true);
}

void profile_basic_match_no_lock(void){
	profile_basic_match(false);
}

void profile_basic_no_match_no_lock(void){
	profile_basic_no_match(false);
}


//Profile N numbe rof entries
void __profile_match_n_entries(int num_entries){

	int i;
	of1x_flow_entry_t* entry;
	uint64_t tics;

	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);

	//PKT
	*((uint32_t*)&tmp_val) = 1;

	//Measure time
	tics = rdtsc();

	//First fill in all the entries
	for(i=0, accumulated_time=0;i<num_entries; i++){
		entry = of1x_init_flow_entry(false, /*builtin=*/false);
		of1x_add_match_to_entry(entry,of1x_init_port_in_match(i));
		CU_ASSERT(of1x_add_flow_entry_table(&sw->pipeline, 0, &entry, false,false) == ROFL_OF1X_FM_SUCCESS);
	}

	//Accumulate
	accumulated_time = rdtsc() - tics;

	fprintf(stderr,"\n%s, Insertion of %u entries took %"PRIu64" cycles\n",  __func__, num_entries, accumulated_time);

#ifdef EXTENDED_PROFILE_TESTS
	unsigned int tid = 2;
	uint32_t average_tics;

	//Execute
	for(i=0, accumulated_time=0;i<NUM_OF_ITERATONS;i++){
		//Measure time
		tics = rdtsc();

		//Process
		of_process_packet_pipeline(tid, (const struct of_switch *)sw, &pkt);

		//Accumulate
		accumulated_time += rdtsc() - tics;
	}

	//Calculate average
	average_tics = accumulated_time / NUM_OF_ITERATONS;

	//Print
	fprintf(stderr,"\n%s MATCH %d pkts, with number of inst. entries: %u. Average cycles/pkt: %u\n",  __func__, NUM_OF_ITERATONS, num_entries, average_tics);
	//TODO output results
#endif
}

void clean_pipeline(){

	of1x_flow_entry_t* deleting_entry = of1x_init_flow_entry(false, /*builtin=*/false);

	CU_ASSERT(deleting_entry != NULL);

	CU_ASSERT(of1x_remove_flow_entry_table(&sw->pipeline, 0, deleting_entry, NOT_STRICT, OF1X_PORT_ANY, OF1X_GROUP_ANY) == ROFL_OF1X_FM_SUCCESS);

	//Check real size of the table
	CU_ASSERT(sw->pipeline.tables[0].num_of_entries == 0);


}

void profile_match_n_entries(){

	clean_pipeline();

	//10
	__profile_match_n_entries(10);
	clean_pipeline();

	//100
	__profile_match_n_entries(100);
	clean_pipeline();

	//1000
	__profile_match_n_entries(1000);
	clean_pipeline();

	//2000
	__profile_match_n_entries(2000);
	clean_pipeline();

	//5000
	__profile_match_n_entries(5000);
	clean_pipeline();

	//10000
	__profile_match_n_entries(10000);
	clean_pipeline();

#ifdef EXTENDED_PROFILE_TESTS
	//20000
	__profile_match_n_entries(20000);
	clean_pipeline();

	//100000
	__profile_match_n_entries(100000);
	clean_pipeline();
#endif
}

int main(int args, char** argv){

	int return_code;
	//main to call all the other tests written in the oder files in this folder
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("Suite_Pipeline_profiling", set_up, tear_down);

	if (NULL == pSuite){
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if ((NULL == CU_add_test(pSuite, "Basic profiling (single flow_mod); match match (lock)", profile_basic_match_lock)) ||
	(NULL == CU_add_test(pSuite, "Basic profiling (single flow_mod); match no-match (lock)", profile_basic_no_match_lock)) ||
	(NULL == CU_add_test(pSuite, "Basic profiling (single flow_mod); match match (no lock)", profile_basic_match_no_lock)) ||
	(NULL == CU_add_test(pSuite, "Basic profiling (single flow_mod); match no-match (no lock)", profile_basic_no_match_no_lock)) ||
	(NULL == CU_add_test(pSuite, "Profile 10/100/1000/10000 entries (no lock)", profile_match_n_entries)) ||
	(NULL == CU_add_test(pSuite, "Reset (use trie now)", reset)) ||
	(NULL == CU_add_test(pSuite, "[trie] Basic profiling (single flow_mod); match match (lock)", profile_basic_match_lock)) ||
	(NULL == CU_add_test(pSuite, "[trie] Basic profiling (single flow_mod); match no-match (lock)", profile_basic_no_match_lock)) ||
	(NULL == CU_add_test(pSuite, "[trie] Basic profiling (single flow_mod); match match (no lock)", profile_basic_match_no_lock)) ||
	(NULL == CU_add_test(pSuite, "[trie] Basic profiling (single flow_mod); match no-match (no lock)", profile_basic_no_match_no_lock)) ||
	(NULL == CU_add_test(pSuite, "[trie] Profile 10/100/1000/2000/5000/10000/20000/100000 entries (no lock)", profile_match_n_entries))
		)
	{
		fprintf(stderr,"ERROR WHILE ADDING TEST\n");
		return_code = CU_get_error();
		CU_cleanup_registry();
		return return_code;
	}

	/* Run all tests using the CUnit Basic interface */
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	return_code = CU_get_number_of_failures();
	CU_cleanup_registry();

	return return_code;
}
