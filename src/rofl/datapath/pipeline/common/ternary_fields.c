#include "ternary_fields.h"

#include <assert.h>

#include "../platform/memory.h"
#include "ternary_fields.h"
#include "large_types.h"
#include "alike_masks.h"

/*
* Initializers
*/
void __init_utern8(utern_t* tern, uint8_t value, uint8_t mask){
	if(!tern)
		return;

	tern->type = UTERN8_T;
	tern->value.u8 = value&mask;
	tern->mask.u8 = mask;
}
void __init_utern16(utern_t* tern, uint16_t value, uint16_t mask){
	if(!tern)
		return;

	tern->type = UTERN16_T;
	tern->value.u16 = value&mask;
	tern->mask.u16 = mask;
}
void __init_utern32(utern_t* tern, uint32_t value, uint32_t mask){
	if(!tern)
		return;

	tern->type = UTERN32_T;
	tern->value.u32 = value&mask;
	tern->mask.u32 = mask;
}
void __init_utern64(utern_t* tern, uint64_t value, uint64_t mask){
	if(!tern)
		return;

	tern->type = UTERN64_T;
	tern->value.u64 = value&mask;
	tern->mask.u64 = mask;
}
void __init_utern128(utern_t* tern, uint128__t value, uint128__t mask){ //uint128_t funny!

	w128_t *tmp, *tmp2;

	if(!tern)
		return;

	tern->type = UTERN128_T;
	tern->value.u128 = value;
	tern->mask.u128 = mask;

	//Mask value
	tmp = (w128_t*)&tern->value.u128;
	tmp2 = (w128_t*)&tern->mask.u128;
	tmp->hi = tmp->hi & tmp2->hi;
	tmp->lo = tmp->lo & tmp2->lo;
}

/*
* Contained 
*/
//Extensive tern is a more generic (with a less restrictive mask or equal) to tern
bool __utern_is_contained(const utern_t* extensive_tern, const utern_t* tern){
	
	switch(extensive_tern->type){
		
		case UTERN8_T:
			//if(((extensive_tern->mask.u8 ^ tern->mask.u8) & extensive_tern->mask.u8) > 0)
			//	return false;
			return (extensive_tern->value.u8 & extensive_tern->mask.u8) == (tern->value.u8 & extensive_tern->mask.u8);
			break;
		case UTERN16_T:
			//if(((extensive_tern->mask.u16 ^ tern->mask.u16) & extensive_tern->mask.u16) > 0)
			//	return false;
			return (extensive_tern->value.u16 & extensive_tern->mask.u16) == (tern->value.u16 & extensive_tern->mask.u16);
			break;
		case UTERN32_T:
			//if(((extensive_tern->mask.u32 ^ tern->mask.u32) & extensive_tern->mask.u32) > 0)
			//	return false;
			return (extensive_tern->value.u32 & extensive_tern->mask.u32) == (tern->value.u32 & extensive_tern->mask.u32);
			break;
		case UTERN64_T:
			if(((extensive_tern->mask.u64 ^ tern->mask.u64) & extensive_tern->mask.u64) > 0)
				return false;
			return (extensive_tern->value.u64 & extensive_tern->mask.u64) == (tern->value.u64 & extensive_tern->mask.u64);
			break;
		case UTERN128_T:
			return ( (UINT128__T_LO(extensive_tern->value.u128) & UINT128__T_LO(extensive_tern->mask.u128)) == (UINT128__T_LO(tern->value.u128) & UINT128__T_LO(extensive_tern->mask.u128)) ) &&
				((UINT128__T_HI(extensive_tern->value.u128) & UINT128__T_HI(extensive_tern->mask.u128)) == (UINT128__T_HI(tern->value.u128) & UINT128__T_HI(extensive_tern->mask.u128))	);
			break;
		default:
			assert(0); //we should never reach this point
			return false;
	}
}

/*
* Check if two ternary values are equal
*/
bool __utern_equals(const utern_t* tern1, const utern_t* tern2){
	switch(tern1->type)	{
		
		case UTERN8_T:
			return (tern1->value.u8 == tern2->value.u8) && (tern1->mask.u8 == tern2->mask.u8);
			break;
		case UTERN16_T:
			return (tern1->value.u16 == tern2->value.u16) && (tern1->mask.u16 == tern2->mask.u16);
			break;
		case UTERN32_T:
			return (tern1->value.u32 == tern2->value.u32) && (tern1->mask.u32 == tern2->mask.u32);
			break;
		case UTERN64_T:
			return (tern1->value.u64 == tern2->value.u64) && (tern1->mask.u64 == tern2->mask.u64);
			break;
		case UTERN128_T:
			return (UINT128__T_LO(tern1->value.u128) == UINT128__T_LO(tern2->value.u128)) && (UINT128__T_LO(tern1->mask.u128) == UINT128__T_LO(tern2->mask.u128)) &&
				(UINT128__T_HI(tern1->value.u128) == UINT128__T_HI(tern2->value.u128)) && (UINT128__T_HI(tern1->mask.u128) == UINT128__T_HI(tern2->mask.u128));
			break;
		default:
			assert(0); // we should never reach this point
			return false;
	}
}

//This function shall find the common and contiguous shared ternary portion of two ternary values
//Masks go from left to right in memory
bool __utern_get_alike(const utern_t* tern1, const utern_t* tern2, utern_t* common){

	int i;
	wrap_uint_t value;
	wrap_uint_t mask;
	uint8_t max_pos;

	//Diferent type of ternary matches cannot share anything
	if(tern1->type != tern2->type)
		return false;

	switch(tern1->type) {
		case UTERN8_T:
				max_pos = (sizeof(uint8_t)*8)-1;
				for(i=max_pos; i>=0; i--){

					//Check that masks match
					if( (tern1->mask.u8 & __u8_alike_masks[i])  != (tern2->mask.u8 & __u8_alike_masks[i]) )
						continue;

					//Check
					if( (tern1->value.u8 & tern1->mask.u8 & __u8_alike_masks[i]) ==
						(tern2->value.u8 & tern2->mask.u8 & __u8_alike_masks[i] ) ){
						value.u8 = tern1->value.u8 & __u8_alike_masks[i];
						mask.u8 = tern1->mask.u8 & __u8_alike_masks[i];

						if((tern1->mask.u8 || tern2->mask.u8) && mask.u8 == 0x0)
							return false;

						goto MATCH_TERN_ALIKE;
					}
				}
				return false;
		case UTERN16_T:
				max_pos = (sizeof(uint16_t)*8)-1;
				for(i=max_pos; i>=0; i--){

					//Check that masks match
					if( (tern1->mask.u16 & __u16_alike_masks[i])  != (tern2->mask.u16 & __u16_alike_masks[i]) )
						continue;

					if( (tern1->value.u16 & tern1->mask.u16 & __u16_alike_masks[i]) ==
						(tern2->value.u16 & tern2->mask.u16 & __u16_alike_masks[i] ) ){
						value.u16 = tern1->value.u16 & __u16_alike_masks[i];
						mask.u16 = tern1->mask.u16 & __u16_alike_masks[i];

						if((tern1->mask.u16 || tern2->mask.u16) && mask.u16 == 0x0)
							return false;

						goto MATCH_TERN_ALIKE;
					}
				}
				return false;
		case UTERN32_T:
				max_pos = (sizeof(uint32_t)*8)-1;
				for(i=max_pos; i>=0; i--){

					//Check that masks match
					if( (tern1->mask.u32 & __u32_alike_masks[i])  != (tern2->mask.u32 & __u32_alike_masks[i]) )
						continue;

					if( (tern1->value.u32 & tern1->mask.u32 & __u32_alike_masks[i]) ==
						(tern2->value.u32 & tern2->mask.u32 & __u32_alike_masks[i] ) ){
						value.u32 = tern1->value.u32 & __u32_alike_masks[i];
						mask.u32 = tern1->mask.u32 & __u32_alike_masks[i];

						if((tern1->mask.u32 || tern2->mask.u32) && mask.u32 == 0x0)
							return false;

						goto MATCH_TERN_ALIKE;
					}
				}
				return false;
		case UTERN64_T:
				max_pos = (sizeof(uint64_t)*8)-1;
				for(i=max_pos; i>=0; i--){

					//Check that masks match
					if( (tern1->mask.u64 & __u64_alike_masks[i])  != (tern2->mask.u64 & __u64_alike_masks[i]) )
						continue;

					if( (tern1->value.u64 & tern1->mask.u64 & __u64_alike_masks[i]) ==
						(tern2->value.u64 & tern2->mask.u64 & __u64_alike_masks[i] ) ){
						value.u64 = tern1->value.u64 & __u64_alike_masks[i];
						mask.u64 = tern1->mask.u64 & __u64_alike_masks[i];

						if((tern1->mask.u64 || tern2->mask.u64) && mask.u64 == 0x0ULL)
							return false;

						goto MATCH_TERN_ALIKE;
					}
				}
				return false;
		case UTERN128_T:
				{
				bool alike = false;
				uint8_t max_u64_bit = (sizeof(uint64_t)*8);
				uint64_t *value1, *mask1;
				uint64_t *value2, *mask2;
				uint64_t tmp1, tmp2;
				uint64_t *val_ptr, *mask_ptr;

				/*
				* Check from left to right (bytes 1...16)
				*/

				//Make code more readable
				value1 = &UINT128__T_HI(tern1->value.u128);
				value2 = &UINT128__T_HI(tern2->value.u128);
				mask1 = &UINT128__T_HI(tern1->mask.u128);
				mask2 = &UINT128__T_HI(tern2->mask.u128);

				//Clear up lows
				((w128_t*)&value.u128)->hi = 0x0UL;
				((w128_t*)&value.u128)->lo = 0x0UL;
				((w128_t*)&mask.u128)->hi = 0x0UL;
				((w128_t*)&mask.u128)->lo = 0x0UL;

				val_ptr = &(((w128_t*)&value.u128)->hi);
				mask_ptr = &(((w128_t*)&mask.u128)->hi);

				//HIGH
				for(i=0; i<max_u64_bit; ++i){
					tmp1 = *mask1 & __u64_alike_masks[i];
					tmp2 = *mask2 & __u64_alike_masks[i];

					if(tmp1 != tmp2)
						break;

					if((tmp1&*value1) != (tmp2&*value2))
						break;

					*val_ptr = tmp1&*value1;
					*mask_ptr = tmp1;

					alike = true;
				}

				if(i != 64)
					goto MATCH_TERN_SKIP_LO_U128;

				value1 = &UINT128__T_LO(tern1->value.u128);
				value2 = &UINT128__T_LO(tern2->value.u128);
				mask1 = &UINT128__T_LO(tern1->mask.u128);
				mask2 = &UINT128__T_LO(tern2->mask.u128);

				val_ptr = &(((w128_t*)&value.u128)->lo);
				mask_ptr = &(((w128_t*)&mask.u128)->lo);

				//LOW
				for(i=0; i<max_u64_bit; ++i){
					tmp1 = *mask1 & __u64_alike_masks[i];
					tmp2 = *mask2 & __u64_alike_masks[i];

					if(tmp1 != tmp2)
						break;

					if((tmp1&*value1) != (tmp2&*value2))
						break;

					*val_ptr = tmp1&*value1;
					*mask_ptr = tmp1;
				}

MATCH_TERN_SKIP_LO_U128:
				//There's match
				if(alike)
					goto MATCH_TERN_ALIKE;

				} //case scope
				return false;
	}

MATCH_TERN_ALIKE:

	if(common){
		common->type = tern1->type;
		common->value = value;
		common->mask = mask;
	}

	return true;
}
