#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "CUnit/Basic.h"
#include "../../../../../src/rofl/datapath/pipeline/common/ternary_fields.h"

int set_up(){
	return 0;
}

int tear_down(){
	return 0;
}

void test_get_alike_8(){

	uint8_t u8_1 = 0xFF;
	uint8_t u8_2 = 0x80;
	uint8_t u8_3 = 0xC0;
	uint8_t u8_4 = 0xF8;
	uint8_t u8_5 = 0x7F;
	uint8_t u8_6 = 0x7D;

	utern_t *one, *two, *three, *four, *five, *six;
	utern_t *null_tern_full_mask, *null_tern_half_mask;
	utern_t* res;

	//Create basic values + mask
	one = __init_utern8(u8_1, 0xFF);
	two = __init_utern8(u8_2, 0xFF);
	three = __init_utern8(u8_3, 0xFF);
	four = __init_utern8(u8_4, 0xFF);
	five = __init_utern8(u8_5, 0x7F);
	six = __init_utern8(u8_6, 0x7D);

	//Nulls
	null_tern_full_mask = __init_utern8(0x00,0xFF);
	CU_ASSERT(one != NULL);
	CU_ASSERT(two != NULL);

	//Check itself8s)
	res = __utern_get_alike(one, one);
	CU_ASSERT(res != NULL);
	CU_ASSERT(res->value.u8 == u8_1);
	CU_ASSERT(res->mask.u8 == 0xFF);
	res = __utern_get_alike(two, two);
	CU_ASSERT(res != NULL);
	CU_ASSERT(res->value.u8 == u8_2);
	CU_ASSERT(res->mask.u8 == 0xFF);
	res = __utern_get_alike(five, five);
	CU_ASSERT(res != NULL);
	CU_ASSERT(__utern_equals(five,res) == true);

	//Check disjoint
	res = __utern_get_alike(one, null_tern_full_mask);
	CU_ASSERT(res == NULL);

	//Check masked sharing first 4 bits
	null_tern_half_mask = __init_utern8(0x00,0xF0);
	res = __utern_get_alike(null_tern_full_mask, null_tern_half_mask);
	CU_ASSERT(res != NULL);
	CU_ASSERT(res->mask.u8 == null_tern_half_mask->mask.u8);
	CU_ASSERT(res->value.u8 == 0x00);

	//Check matches that share value
	res = __utern_get_alike(one, two);
	CU_ASSERT(res != NULL);
	CU_ASSERT(res->mask.u8 == 0x80)
	CU_ASSERT(res->value.u8 == 0x80);

	res = __utern_get_alike(one, three);
	CU_ASSERT(res != NULL);
	CU_ASSERT(res->mask.u8 == 0xC0)
	CU_ASSERT(res->value.u8 == 0xC0);

	res = __utern_get_alike(one, four);
	CU_ASSERT(res != NULL);
	CU_ASSERT(res->mask.u8 == 0xF8)
	CU_ASSERT(res->value.u8 == 0xF8);

	//Discontinuous masks
	utern_t* disc = __init_utern8(0xFF,0xBF);
	res = __utern_get_alike(one, disc);
	CU_ASSERT(res != NULL);
	CU_ASSERT(res->mask.u8 == 0x80)
	CU_ASSERT(res->value.u8 == 0x80);

	//Leave 0s in the very beginning
	res = __utern_get_alike(five, six);
	CU_ASSERT(res != NULL);
	CU_ASSERT(res->mask.u8 == 0x7C)
	CU_ASSERT(res->value.u8 == 0x7C);


}

void test_get_alike_16(){

	uint8_t tmp[2], tmp_mask[2];

	utern_t *one, *two, *three, *four, *five;
	utern_t* res;

	//One
	tmp[0] = 0xF0;
	tmp[1] = 0x00;
	tmp_mask[0] = 0xFF;
	tmp_mask[1] = 0xFF;
	one = __init_utern16(*((uint16_t*)tmp), *((uint16_t*)tmp_mask));

	tmp[0] = 0xF8;
	tmp[1] = 0x00;
	tmp_mask[0] = 0xFF;
	tmp_mask[1] = 0xFF;
	two = __init_utern16(*((uint16_t*)tmp), *((uint16_t*)tmp_mask));

	tmp[0] = 0xFF;
	tmp[1] = 0x0F;
	tmp_mask[0] = 0xFF;
	tmp_mask[1] = 0xFF;
	three = __init_utern16(*((uint16_t*)tmp), *((uint16_t*)tmp_mask));

	tmp[0] = 0xFF;
	tmp[1] = 0x0B;
	tmp_mask[0] = 0xFF;
	tmp_mask[1] = 0xFF;
	three = __init_utern16(*((uint16_t*)tmp), *((uint16_t*)tmp_mask));

	tmp[0] = 0x1F;
	tmp[1] = 0xFB;
	tmp_mask[0] = 0x1F;
	tmp_mask[1] = 0xFF;
	four = __init_utern16(*((uint16_t*)tmp), *((uint16_t*)tmp_mask));

	tmp[0] = 0x0F;
	tmp[1] = 0xF0;
	tmp_mask[0] = 0x0F;
	tmp_mask[1] = 0xF0;
	five = __init_utern16(*((uint16_t*)tmp), *((uint16_t*)tmp_mask));

	//Check itself(s)
	res = __utern_get_alike(one, one);
	CU_ASSERT(res != NULL);
	CU_ASSERT(__utern_equals(one,res) == true);

	res = __utern_get_alike(two, two);
	CU_ASSERT(res != NULL);
	CU_ASSERT(__utern_equals(two,res) == true);

	res = __utern_get_alike(three, three);
	CU_ASSERT(res != NULL);
	CU_ASSERT(__utern_equals(three, res) == true);

	res = __utern_get_alike(four, four);
	CU_ASSERT(res != NULL);
	CU_ASSERT(__utern_equals(four, res) == true);

	res = __utern_get_alike(five, five);
	CU_ASSERT(res != NULL);
	CU_ASSERT(__utern_equals(five, res) == true);

	//Check 1 against two
	res = __utern_get_alike(one, two);
	CU_ASSERT(res != NULL);
	CU_ASSERT(__utern_equals(one,res) == true);




	//Check agai

}

int main(int args, char** argv){

	int return_code;
	//main to call all the other tests written in the oder files in this folder
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	/* add a suite to the registry */
	pSuite = CU_add_suite("Suite_Pipeline_Get_Alike", set_up, tear_down);

	if (NULL == pSuite){
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if ((NULL == CU_add_test(pSuite, "8 bit test", test_get_alike_8)) ||
		(NULL == CU_add_test(pSuite, "16 bit test", test_get_alike_16)) //||
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
