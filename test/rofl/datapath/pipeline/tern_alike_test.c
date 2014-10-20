#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
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

	utern_t one, two, three, four, five, six;
	utern_t null_tern_full_mask, null_tern_half_mask;
	utern_t common, disc;
	bool res;

	//Create basic values + mask
	__init_utern8(&one, u8_1, 0xFF);
	__init_utern8(&two, u8_2, 0xFF);
	__init_utern8(&three, u8_3, 0xFF);
	__init_utern8(&four, u8_4, 0xFF);
	__init_utern8(&five, u8_5, 0x7F);
	__init_utern8(&six, u8_6, 0x7D);

	//Nulls
	__init_utern8(&null_tern_full_mask, 0x00,0xFF);

	//Check itself8s)
	res = __utern_get_alike(&one, &one, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(common.value.u8 == u8_1);
	CU_ASSERT(common.mask.u8 == 0xFF);
	res = __utern_get_alike(&two, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(common.value.u8 == u8_2);
	CU_ASSERT(common.mask.u8 == 0xFF);
	res = __utern_get_alike(&five, &five, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&five,&common) == true);

	//Check disjoint
	res = __utern_get_alike(&one, &null_tern_full_mask, &common);
	CU_ASSERT(res == false);

	//Check masked sharing first 4 bits
	__init_utern8(&null_tern_half_mask, 0x00,0xF0);
	res = __utern_get_alike(&null_tern_full_mask, &null_tern_half_mask, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(common.mask.u8 == null_tern_half_mask.mask.u8);
	CU_ASSERT(common.value.u8 == 0x00);

	//Check matches that share value
	res = __utern_get_alike(&one, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(common.mask.u8 == 0x80)
	CU_ASSERT(common.value.u8 == 0x80);

	res = __utern_get_alike(&one, &three, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(common.mask.u8 == 0xC0)
	CU_ASSERT(common.value.u8 == 0xC0);

	res = __utern_get_alike(&one, &four, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(common.mask.u8 == 0xF8)
	CU_ASSERT(common.value.u8 == 0xF8);

	//Discontinuous masks
	__init_utern8(&disc, 0xFF,0xBF);
	res = __utern_get_alike(&one, &disc, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(common.mask.u8 == 0x80)
	CU_ASSERT(common.value.u8 == 0x80);

	//Leave 0s in the very beginning
	res = __utern_get_alike(&five, &six, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(common.mask.u8 == 0x7C)
	CU_ASSERT(common.value.u8 == 0x7C);


}

void test_get_alike_16(){

	uint8_t* aux;
	uint8_t tmp[2], tmp_mask[2];

	utern_t one, two, three, four, five, six, seven;
	utern_t common;
	bool res;

	uint16_t* tmp_u16 = (uint16_t*)tmp;
	uint16_t* tmp_mask_u16 = (uint16_t*)tmp_mask;

	//One
	tmp[0] = 0xF0;
	tmp[1] = 0x00;
	tmp_mask[0] = 0xFF;
	tmp_mask[1] = 0xFF;
	__init_utern16(&one, *tmp_u16, *tmp_mask_u16);

	tmp[0] = 0xF8;
	tmp[1] = 0x00;
	tmp_mask[0] = 0xFF;
	tmp_mask[1] = 0xFF;
	__init_utern16(&two, *tmp_u16, *tmp_mask_u16);

	tmp[0] = 0xF0;
	tmp[1] = 0x0F;
	tmp_mask[0] = 0xFF;
	tmp_mask[1] = 0xFF;
	__init_utern16(&three, *tmp_u16, *tmp_mask_u16);

	tmp[0] = 0xFF;
	tmp[1] = 0x0B;
	tmp_mask[0] = 0xFF;
	tmp_mask[1] = 0xFF;
	__init_utern16(&four, *tmp_u16, *tmp_mask_u16);

	tmp[0] = 0x1F;
	tmp[1] = 0xFB;
	tmp_mask[0] = 0x1F;
	tmp_mask[1] = 0xFF;
	__init_utern16(&five, *tmp_u16, *tmp_mask_u16);

	tmp[0] = 0x0F;
	tmp[1] = 0xF0;
	tmp_mask[0] = 0x0F;
	tmp_mask[1] = 0xF0;
	__init_utern16(&six, *tmp_u16, *tmp_mask_u16);

	tmp[0] = 0x0F;
	tmp[1] = 0xFF;
	tmp_mask[0] = 0x0F;
	tmp_mask[1] = 0xFC;
	__init_utern16(&seven, *tmp_u16, *tmp_mask_u16);


	//Check itself(s)
	res = __utern_get_alike(&one, &one, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one,&common) == true);

	res = __utern_get_alike(&two, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&two,&common) == true);

	res = __utern_get_alike(&three, &three, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&three, &common) == true);

	res = __utern_get_alike(&four, &four, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&four, &common) == true);

	res = __utern_get_alike(&five, &five, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&five, &common) == true);

	res = __utern_get_alike(&six, &six, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&six, &common) == true);
	res = __utern_get_alike(&seven, &seven, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&seven, &common) == true);


	//Check 1 against two
	res = __utern_get_alike(&one, &two, &common);
	CU_ASSERT(res == true);
	aux = (uint8_t*)&common.value.u16;
	CU_ASSERT(aux[0] == 0xF0 && aux[1] == 0x00);
	aux = (uint8_t*)&common.mask.u16;
	CU_ASSERT(aux[0] == 0xF0 && aux[1] == 0x00);

	//1, 3
	res = __utern_get_alike(&one, &three, &common);
	CU_ASSERT(res == true);
	aux = (uint8_t*)&common.value.u16;
	CU_ASSERT(aux[0] == 0xF0 && aux[1] == 0x00);
	aux = (uint8_t*)&common.mask.u16;
	CU_ASSERT(aux[0] == 0xFF && aux[1] == 0xF0);

	//2, 4
	res = __utern_get_alike(&two, &four, &common);
	CU_ASSERT(res == true);
	aux = (uint8_t*)&common.value.u16;
	CU_ASSERT(aux[0] == 0xF8 && aux[1] == 0x00);
	aux = (uint8_t*)&common.mask.u16;
	CU_ASSERT(aux[0] == 0xF8 && aux[1] == 0x00);

	//4,5
	res = __utern_get_alike(&four, &five, &common);
	CU_ASSERT(res == false);

	//5,6
	res = __utern_get_alike(&five, &six, &common);
	CU_ASSERT(res == false);

	//6,7
	res = __utern_get_alike(&six, &seven, &common);
	CU_ASSERT(res == true);
	aux = (uint8_t*)&common.value.u16;
	CU_ASSERT(aux[0] == 0x0F && aux[1] == 0xF0);
	aux = (uint8_t*)&common.mask.u16;
	CU_ASSERT(aux[0] == 0x0F && aux[1] == 0xF0);
}

void test_get_alike_32(){

	uint32_t ip1, ip2, ip3, ip4, ip5;
	uint32_t mask1, mask2, mask3, mask4, mask5;
	utern_t one, two, three, four, five;

	utern_t common;
	bool res;

	//Assign values and masks
	ip1 = htonl(0xC0A80001);//192.168.0.1/24
	mask1 = htonl(0xFFFFFF00);
	__init_utern32(&one, ip1, mask1);

	ip2 = htonl(0xC0A80001);//192.168.0.1/32
	mask2 = htonl(0xFFFFFFFF);
	__init_utern32(&two, ip2, mask2);

	ip3 = htonl(0xC0A80001);//192.168.0.1/16
	mask3 = htonl(0xFFFF0000);
	__init_utern32(&three, ip3, mask3);

	ip4 = htonl(0xC0E00001);//192.224.0.0/32
	mask4 = htonl(0xFFFFFFFF);
	__init_utern32(&four, ip4, mask4);

	ip5 = htonl(0xC0A80001);//192.168.0.1
	mask5 = htonl(0xFFFFF8FF); //non-contiguous mask
	__init_utern32(&five, ip5, mask5);


	//Check itself(s)
	res = __utern_get_alike(&one, &one, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == true);

	res = __utern_get_alike(&two, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&two, &common) == true);

	res = __utern_get_alike(&three, &three, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&three, &common) == true);

	res = __utern_get_alike(&four, &four, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&four, &common) == true);

	//1,2
	res = __utern_get_alike(&one, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == true);
	CU_ASSERT(ntohl(common.value.u32) == 0xC0A80000);
	CU_ASSERT(ntohl(common.mask.u32) == 0xFFFFFF00);

	//2,1
	res = __utern_get_alike(&two, &one, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == true);
	CU_ASSERT(ntohl(common.value.u32) == 0xC0A80000);
	CU_ASSERT(ntohl(common.mask.u32) == 0xFFFFFF00);


	//2,3
	res = __utern_get_alike(&two, &three, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&three, &common) == true);
	CU_ASSERT(ntohl(common.value.u32) == 0xC0A80000);
	CU_ASSERT(ntohl(common.mask.u32) == 0xFFFF0000);


	//3,4
	res = __utern_get_alike(&four, &three, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&four, &common) == false);
	CU_ASSERT(ntohl(common.value.u32) == 0xC0800000);
	CU_ASSERT(ntohl(common.mask.u32) == 0xFF800000);

	//1,5
	res = __utern_get_alike(&one, &five, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == false);
	CU_ASSERT(__utern_equals(&five, &common) == false);
	CU_ASSERT(ntohl(common.value.u32) == 0xC0A80000);
	CU_ASSERT(ntohl(common.mask.u32) == 0xFFFFF800);
}

void test_get_alike_64(){

	uint8_t *tmp;
	uint8_t val1[8], val2[8], val3[8], val4[8];
	uint8_t mask1[8], mask2[8], mask3[8], mask4[8];

	utern_t one, two, three, four;

	utern_t common;
	bool res;

	uint64_t* tmp_u64;
	uint64_t* tmp_mask_u64;

	/* Assign values and masks */

	//01:23:45:67:89:AB
	val1[0] = 0x01;
	val1[1] = 0x23;
	val1[2] = 0x45;
	val1[3] = 0x67;
	val1[4] = 0x89;
	val1[5] = 0xAB;
	val1[6] = 0x00;
	val1[7] = 0x00;
	mask1[0] = mask1[1] = mask1[2] = mask1[3] = mask1[4] =  mask1[5] = 0xFF;
	mask1[6] = mask1[7] = 0x00;
	tmp_u64 = (uint64_t*)val1;
	tmp_mask_u64 = (uint64_t*)mask1;
	__init_utern64(&one, *tmp_u64, *tmp_mask_u64);

	//01:23:45:67:89:1B
	val2[0] = 0x01;
	val2[1] = 0x23;
	val2[2] = 0x45;
	val2[3] = 0x67;
	val2[4] = 0x89;
	val2[5] = 0x8B;
	val2[6] = 0x00;
	val2[7] = 0x00;
	mask2[0] = mask2[1] = mask2[2] = mask2[3] = mask2[4] =  mask2[5] = 0xFF;
	mask2[6] = mask2[7] = 0x00;
	tmp_u64 = (uint64_t*)val2;
	tmp_mask_u64 = (uint64_t*)mask2;
	__init_utern64(&two, *tmp_u64, *tmp_mask_u64);

	//81:23:45:67:89:1B
	val3[0] = 0x81;
	val3[1] = 0x23;
	val3[2] = 0x45;
	val3[3] = 0x67;
	val3[4] = 0x89;
	val3[5] = 0x8B;
	val3[6] = 0x00;
	val3[7] = 0x00;
	mask3[0] = mask3[1] = mask3[2] = mask3[3] = mask3[4] = mask3[5] = 0xFF;
	mask3[6] = mask3[7] = 0x00;
	tmp_u64 = (uint64_t*)val3;
	tmp_mask_u64 = (uint64_t*)mask3;
	__init_utern64(&three, *tmp_u64, *tmp_mask_u64);

	//01:23:45:76:89:1B
	val4[0] = 0x01;
	val4[1] = 0x23;
	val4[2] = 0x45;
	val4[3] = 0x76;
	val4[4] = 0x89;
	val4[5] = 0x8B;
	val4[6] = 0x00;
	val4[7] = 0x00;
	mask4[0] = mask4[1] = mask4[2] = mask4[3] = mask4[4] = mask4[5] = 0xFF;
	mask4[6] = mask4[7] = 0x00;
	tmp_u64 = (uint64_t*)val4;
	tmp_mask_u64 = (uint64_t*)mask4;
	__init_utern64(&four, *tmp_u64, *tmp_mask_u64);


	//Check itself(s)
	res = __utern_get_alike(&one, &one, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == true);

	res = __utern_get_alike(&two, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&two, &common) == true);

	res = __utern_get_alike(&three, &three, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&three, &common) == true);

	res = __utern_get_alike(&four, &four, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&four, &common) == true);

	//1,2
	res = __utern_get_alike(&one, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == false);
	tmp = (uint8_t*)&common.value.u64;
	CU_ASSERT(tmp[0] == 0x01);
	CU_ASSERT(tmp[1] == 0x23);
	CU_ASSERT(tmp[2] == 0x45);
	CU_ASSERT(tmp[3] == 0x67);
	CU_ASSERT(tmp[4] == 0x89);
	CU_ASSERT(tmp[5] == 0x80);
	CU_ASSERT(tmp[6] == 0x00);
	CU_ASSERT(tmp[7] == 0x00);
	tmp = (uint8_t*)&common.mask.u64;
	CU_ASSERT(tmp[0] == 0xFF);
	CU_ASSERT(tmp[1] == 0xFF);
	CU_ASSERT(tmp[2] == 0xFF);
	CU_ASSERT(tmp[3] == 0xFF);
	CU_ASSERT(tmp[4] == 0xFF);
	CU_ASSERT(tmp[5] == 0xC0);
	CU_ASSERT(tmp[6] == 0x00);
	CU_ASSERT(tmp[7] == 0x00);

	//1,3
	res = __utern_get_alike(&one, &three, &common);
	CU_ASSERT(res == false);

	//1,4
	res = __utern_get_alike(&one, &four, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == false);
	tmp = (uint8_t*)&common.value.u64;
	CU_ASSERT(tmp[0] == 0x01);
	CU_ASSERT(tmp[1] == 0x23);
	CU_ASSERT(tmp[2] == 0x45);
	CU_ASSERT(tmp[3] == 0x60);
	CU_ASSERT(tmp[4] == 0x00);
	CU_ASSERT(tmp[5] == 0x00);
	CU_ASSERT(tmp[6] == 0x00);
	CU_ASSERT(tmp[7] == 0x00);
	tmp = (uint8_t*)&common.mask.u64;
	CU_ASSERT(tmp[0] == 0xFF);
	CU_ASSERT(tmp[1] == 0xFF);
	CU_ASSERT(tmp[2] == 0xFF);
	CU_ASSERT(tmp[3] == 0xE0);
	CU_ASSERT(tmp[4] == 0x00);
	CU_ASSERT(tmp[5] == 0x00);
	CU_ASSERT(tmp[6] == 0x00);
	CU_ASSERT(tmp[7] == 0x00);
}

void test_get_alike_128(){

	uint8_t *tmp;
	uint8_t val1[16], val2[16], val3[16]; //, val4[16];
	uint8_t mask1[16], mask2[16], mask3[16]; //, mask4[16];

	utern_t one, two, three; //, four;
	utern_t common;
	bool res;


	uint128__t* tmp_u128;
	uint128__t* tmp_mask_u128;

	/* Assign values and masks */
	val1[0] = 0x01;
	val1[1] = 0x23;
	val1[2] = 0x45;
	val1[3] = 0x67;
	val1[4] = 0x89;
	val1[5] = 0xAB;
	val1[6] = 0xCD;
	val1[7] = 0xEF;
	val1[8] = 0x10;
	val1[9] = 0x32;
	val1[10] = 0x54;
	val1[11] = 0x76;
	val1[12] = 0x98;
	val1[13] = 0xBA;
	val1[14] = 0xDC;
	val1[15] = 0xFE;

	memset(mask1, 0xFF, 16);
	mask1[14] = mask1[15] = 0x00;
	tmp_u128 = (uint128__t*)val1;
	tmp_mask_u128 = (uint128__t*)mask1;
	__init_utern128(&one, *tmp_u128, *tmp_mask_u128);

	/* Assign values and masks */
	val2[0] = 0x01;
	val2[1] = 0x23;
	val2[2] = 0x45;
	val2[3] = 0x67;
	val2[4] = 0x89;
	val2[5] = 0xAB;
	val2[6] = 0xCD;
	val2[7] = 0xEF;
	val2[8] = 0x10;
	val2[9] = 0x32;
	val2[10] = 0x54;
	val2[11] = 0x76;
	val2[12] = 0x98;
	val2[13] = 0xB0; //!
	val2[14] = 0xDC;
	val2[15] = 0xFE;

	memset(mask2, 0xFF, 16);
	mask3[14] = mask2[15] = 0x00;
	tmp_u128 = (uint128__t*)val2;
	tmp_mask_u128 = (uint128__t*)mask2;
	__init_utern128(&two, *tmp_u128, *tmp_mask_u128);


	/* Assign values and masks */
	val3[0] = 0x01;
	val3[1] = 0x23;
	val3[2] = 0x45;
	val3[3] = 0x67;
	val3[4] = 0x88; //!
	val3[5] = 0xAB;
	val3[6] = 0xCD;
	val3[7] = 0xEF;
	val3[8] = 0x10;
	val3[9] = 0x32;
	val3[10] = 0x54;
	val3[11] = 0x76;
	val3[12] = 0x98;
	val3[13] = 0xBA;
	val3[14] = 0xDC;
	val3[15] = 0xFE;

	memset(mask3, 0xFF, 16);
	mask3[14] = mask3[15] = 0x00;
	tmp_u128 = (uint128__t*)val3;
	tmp_mask_u128 = (uint128__t*)mask3;
	__init_utern128(&three, *tmp_u128, *tmp_mask_u128);

	//Check itself(s)
	res = __utern_get_alike(&one, &one, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == true);

	res = __utern_get_alike(&two, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&two, &common) == true);

	res = __utern_get_alike(&three, &three, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&three, &common) == true);


	//1,2
	res = __utern_get_alike(&one, &two, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == false);
	tmp = (uint8_t*)&common.value.u128;
	CU_ASSERT(tmp[0] == 0x01);
	CU_ASSERT(tmp[1] == 0x23);
	CU_ASSERT(tmp[2] == 0x45);
	CU_ASSERT(tmp[3] == 0x67);
	CU_ASSERT(tmp[4] == 0x89);
	CU_ASSERT(tmp[5] == 0xAB);
	CU_ASSERT(tmp[6] == 0xCD);
	CU_ASSERT(tmp[7] == 0xEF);
	CU_ASSERT(tmp[8] == 0x10);
	CU_ASSERT(tmp[9] == 0x32);
	CU_ASSERT(tmp[10] == 0x54);
	CU_ASSERT(tmp[11] == 0x76);
	CU_ASSERT(tmp[12] == 0x98);
	CU_ASSERT(tmp[13] == 0xB0);
	CU_ASSERT(tmp[14] == 0x00);
	CU_ASSERT(tmp[15] == 0x00);

	tmp = (uint8_t*)&common.mask.u128;
	CU_ASSERT(tmp[0] == 0xFF);
	CU_ASSERT(tmp[1] == 0xFF);
	CU_ASSERT(tmp[2] == 0xFF);
	CU_ASSERT(tmp[3] == 0xFF);
	CU_ASSERT(tmp[4] == 0xFF);
	CU_ASSERT(tmp[5] == 0xFF);
	CU_ASSERT(tmp[6] == 0xFF);
	CU_ASSERT(tmp[7] == 0xFF);
	CU_ASSERT(tmp[8] == 0xFF);
	CU_ASSERT(tmp[9] == 0xFF);
	CU_ASSERT(tmp[10] == 0xFF);
	CU_ASSERT(tmp[11] == 0xFF);
	CU_ASSERT(tmp[12] == 0xFF);
	CU_ASSERT(tmp[13] == 0xF0);
	CU_ASSERT(tmp[14] == 0x00);
	CU_ASSERT(tmp[15] == 0x00);

	//1,3
	res = __utern_get_alike(&one, &three, &common);
	CU_ASSERT(res == true);
	CU_ASSERT(__utern_equals(&one, &common) == false);
	tmp = (uint8_t*)&common.value.u128;
	CU_ASSERT(tmp[0] == 0x01);
	CU_ASSERT(tmp[1] == 0x23);
	CU_ASSERT(tmp[2] == 0x45);
	CU_ASSERT(tmp[3] == 0x67);
	CU_ASSERT(tmp[4] == 0x88);
	CU_ASSERT(tmp[5] == 0x00);
	CU_ASSERT(tmp[6] == 0x00);
	CU_ASSERT(tmp[7] == 0x00);
	CU_ASSERT(tmp[8] == 0x00);
	CU_ASSERT(tmp[9] == 0x00);
	CU_ASSERT(tmp[10] == 0x00);
	CU_ASSERT(tmp[11] == 0x00);
	CU_ASSERT(tmp[12] == 0x00);
	CU_ASSERT(tmp[13] == 0x00);
	CU_ASSERT(tmp[14] == 0x00);
	CU_ASSERT(tmp[15] == 0x00);

	tmp = (uint8_t*)&common.mask.u128;
	CU_ASSERT(tmp[0] == 0xFF);
	CU_ASSERT(tmp[1] == 0xFF);
	CU_ASSERT(tmp[2] == 0xFF);
	CU_ASSERT(tmp[3] == 0xFF);
	CU_ASSERT(tmp[4] == 0xFE);
	CU_ASSERT(tmp[5] == 0x00);
	CU_ASSERT(tmp[6] == 0x00);
	CU_ASSERT(tmp[7] == 0x00);
	CU_ASSERT(tmp[8] == 0x00);
	CU_ASSERT(tmp[9] == 0x00);
	CU_ASSERT(tmp[10] == 0x00);
	CU_ASSERT(tmp[11] == 0x00);
	CU_ASSERT(tmp[12] == 0x00);
	CU_ASSERT(tmp[13] == 0x00);
	CU_ASSERT(tmp[14] == 0x00);
	CU_ASSERT(tmp[15] == 0x00);

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
		(NULL == CU_add_test(pSuite, "16 bit test", test_get_alike_16)) ||
		(NULL == CU_add_test(pSuite, "32 bit test", test_get_alike_32)) ||
		(NULL == CU_add_test(pSuite, "64 bit test", test_get_alike_64)) ||
		(NULL == CU_add_test(pSuite, "128 bit test", test_get_alike_128)) //||
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
