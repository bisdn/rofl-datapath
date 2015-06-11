#include "ternary_fields.h"

#include <assert.h>

#include "../platform/memory.h"
#include "ternary_fields.h"
#include "large_types.h"
#include "alike_masks.h"

/*
* Initializers
*/
inline utern_t* __init_utern8(uint8_t value, uint8_t mask){
	utern_t* tern = (utern_t*)platform_malloc_shared(sizeof(utern_t));

	if(!tern)
		return NULL;

	tern->type = UTERN8_T;
	tern->value.u8 = value;
	tern->mask.u8 = mask;
	return (utern_t*)tern;
}
inline utern_t* __init_utern16(uint16_t value, uint16_t mask){
	utern_t* tern = (utern_t*)platform_malloc_shared(sizeof(utern_t));

	if(!tern)
		return NULL;

	tern->type = UTERN16_T;
	tern->value.u16 = value;
	tern->mask.u16 = mask;
	return (utern_t*)tern;
}
inline utern_t* __init_utern32(uint32_t value, uint32_t mask){
	utern_t* tern = (utern_t*)platform_malloc_shared(sizeof(utern_t));

	if(!tern)
		return NULL;
	
	tern->type = UTERN32_T;
	tern->value.u32 = value;
	tern->mask.u32 = mask;
	return (utern_t*)tern;
}
inline utern_t* __init_utern64(uint64_t value, uint64_t mask){
	utern_t* tern = (utern_t*)platform_malloc_shared(sizeof(utern_t));

	if(!tern)
		return NULL;
	
	tern->type = UTERN64_T;
	tern->value.u64 = value;
	tern->mask.u64 = mask;
	return (utern_t*)tern;
}
inline utern_t* __init_utern128(uint128__t value, uint128__t mask){ //uint128_t funny!
	utern_t* tern = (utern_t*)platform_malloc_shared(sizeof(utern_t));
	
	if(!tern)
		return NULL;
	
	tern->type = UTERN128_T;
	tern->value.u128 = value;
	tern->mask.u128 = mask;
	return (utern_t*)tern;
}

/*
* Single destructor
*/
rofl_result_t __destroy_utern(utern_t* utern){
	platform_free_shared(utern);
	//FIXME: maybe check returning value
	return ROFL_SUCCESS; 
}	




/*
* Contained 
*/
//Extensive tern is a more generic (with a less restrictive mask or equal) to tern
inline bool __utern_is_contained(const utern_t* extensive_tern, const utern_t* tern){
	
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
inline bool __utern_equals(const utern_t* tern1, const utern_t* tern2){
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
inline utern_t* __utern_get_alike(const utern_t* tern1, const utern_t* tern2){

	int i;
	utern_t* common;
	wrap_uint_t value;
	wrap_uint_t mask;
	uint8_t max_pos;

	//Diferent type of ternary matches cannot share anything
	if(tern1->type != tern2->type)
		return NULL;

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

						if(mask.u8 == 0x0)
							return NULL;

						goto MATCH_TERN_ALIKE;
					}
				}
				return NULL;
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

						if(mask.u16 == 0x0)
							return NULL;

						goto MATCH_TERN_ALIKE;
					}
				}
				return NULL;
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

						if(mask.u32 == 0x0)
							return NULL;

						goto MATCH_TERN_ALIKE;
					}
				}
				return NULL;
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
						goto MATCH_TERN_ALIKE;
					}
				}
				return NULL;
		case UTERN128_T:
				{
				max_pos = (sizeof(uint64_t)*8)-1;
				uint64_t *value1_h, *value1_l, *mask1_h, *mask1_l;
				uint64_t *value2_h, *value2_l, *mask2_h, *mask2_l;

				//Make code more readable
				value1_h = &UINT128__T_HI(tern1->value.u128);
				value2_h = &UINT128__T_HI(tern2->value.u128);
				mask1_h = &UINT128__T_HI(tern1->mask.u128);
				mask2_h = &UINT128__T_HI(tern2->mask.u128);

				value1_l = &UINT128__T_LO(tern1->value.u128);
				value2_l = &UINT128__T_LO(tern2->value.u128);
				mask1_l = &UINT128__T_LO(tern1->mask.u128);
				mask2_l = &UINT128__T_LO(tern2->mask.u128);

				//Check right 64 bits first(LOW)
				for(i=max_pos; i>=0; i--){

					//Check that masks match (HIGH)
					if ( (*mask1_h  & __u64_alike_masks[max_pos]) !=
						(*mask2_h  & __u64_alike_masks[max_pos]) )
						break;

					//We want contiguos masks, so if HIGH does not match skip LOG checks
					if ( (*value1_h & *mask1_h  & __u64_alike_masks[max_pos]) !=
						(*value2_h & *mask2_h  & __u64_alike_masks[max_pos]) )
						break;

					//Check that masks match (LOW)
					if ( (*mask1_l  & __u64_alike_masks[i]) !=
						(*mask2_l  & __u64_alike_masks[i]) )
						continue;

					if ( (*value1_l & *mask1_l  & __u64_alike_masks[i]) ==
						(*value2_l & *mask2_l  & __u64_alike_masks[i]) ){
						//Set LOW
						UINT128__T_LO(value.u128) = *value1_l & __u64_alike_masks[i];
						UINT128__T_LO(mask.u128) = *mask1_l & __u64_alike_masks[i];
						//Set HIGH
						UINT128__T_HI(value.u128) = *value1_h;
						UINT128__T_HI(mask.u128) = *mask1_h;

						goto MATCH_TERN_ALIKE;
					}
				}

				//Check left 64bit chunk (HIGH)
				for(i=max_pos; i>=0; i--){

					//Check that masks match (HIGH)
					if ( (*mask1_h  & __u64_alike_masks[max_pos]) !=
						(*mask2_h  & __u64_alike_masks[max_pos]) )
						break;

					if ( (*value1_h & *mask1_h  & __u64_alike_masks[i]) ==
						(*value2_h & *mask2_h  & __u64_alike_masks[i]) ){
						//Set LOW
						UINT128__T_LO(value.u128) = UINT128__T_LO(mask.u128) = 0x0ULL;
						//Set HIGH
						UINT128__T_HI(value.u128) = *value1_h & __u64_alike_masks[i];
						UINT128__T_HI(mask.u128) = *mask1_h & __u64_alike_masks[i];

						if(UINT128__T_HI(mask.u128) == 0x0ULL)
							return NULL;

						goto MATCH_TERN_ALIKE;
					}
				} //for

				} //case scope
				return NULL;
	}

MATCH_TERN_ALIKE:


	//Allocate space
	common  = (utern_t*)platform_malloc_shared(sizeof(utern_t));
	if(!common)
		return NULL;

	common->type = tern1->type;
	common->value = value;
	common->mask = mask;

	return common;
}
