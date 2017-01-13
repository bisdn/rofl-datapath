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

						if( (*mask2_l || *mask2_h || *mask1_l || *mask2_l) &&
							((UINT128__T_LO(mask.u128)&__u64_alike_masks[i]) == 0x0ULL))
							return false;

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

						if( (*mask2_l || *mask2_h || *mask1_l || *mask2_l) &&
							((UINT128__T_HI(mask.u128)&__u64_alike_masks[i]) == 0x0ULL))
							return false;

						goto MATCH_TERN_ALIKE;
					}
				} //for

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
