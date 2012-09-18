/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "fvlanframe.h"


fvlanframe::fvlanframe(
		uint8_t *data,
		size_t datalen) :
	fframe(data, datalen),
	vlan_hdr(0)
{
	initialize();
}



fvlanframe::~fvlanframe()
{
	// do _NOT_ delete or deallocate (data,datalen) here!
}



void
fvlanframe::initialize()
{
	vlan_hdr = (struct vlan_hdr_t*)soframe();
}


bool
fvlanframe::complete()
{
	if (framelen() < sizeof(struct vlan_hdr_t))
		return false;

	initialize();

	return true;
}


size_t
fvlanframe::need_bytes()
{
	if (framelen() < sizeof(struct vlan_hdr_t))
		return (sizeof(struct vlan_hdr_t) - framelen());

	initialize();

	return 0;
}


void
fvlanframe::payload_insert(
		uint8_t *data,
		size_t datalen) throw (eFrameOutOfRange)
{
	if (datalen > (framelen() - sizeof(struct vlan_hdr_t)))
	{
		throw eFrameOutOfRange();
	}
	memcpy(soframe() + sizeof(struct vlan_hdr_t), data, datalen);
}


uint8_t*
fvlanframe::payload() throw (eFrameNoPayload)
{
	if (framelen() <= sizeof(struct vlan_hdr_t))
		throw eFrameNoPayload();

	initialize();

	return (soframe() + sizeof(struct vlan_hdr_t));
}


size_t
fvlanframe::payloadlen() throw (eFrameNoPayload)
{
	if (framelen() <= sizeof(struct vlan_hdr_t))
		throw eFrameNoPayload();

	initialize();

	return (framelen() - sizeof(struct vlan_hdr_t));
}


void
fvlanframe::validate() throw (eFrameInvalidSyntax)
{
	initialize();

	if (framelen() < (sizeof(struct vlan_hdr_t)))
		throw eFrameInvalidSyntax();

	// TODO: check on minimum length of 64 bytes?
}


void
fvlanframe::set_dl_vlan_id(uint16_t vid)
{
	uint16_t v = (be16toh(vlan_hdr->hdr) & 0x000f) + (vid << 4);
	vlan_hdr->hdr = htobe16(v);
}


uint16_t
fvlanframe::get_dl_vlan_id() const
{
	uint16_t v = be16toh(vlan_hdr->hdr);
	return (v >> 4);
}


void
fvlanframe::set_dl_vlan_pcp(uint8_t pcp)
{
	uint16_t v = (be16toh(vlan_hdr->hdr) & 0xfff0) + (pcp & 0x07);
	vlan_hdr->hdr = htobe16(v);
}


uint8_t
fvlanframe::get_dl_vlan_pcp() const
{
	uint16_t v = be16toh(vlan_hdr->hdr);
	return (v & 0x0007);
}


void
fvlanframe::set_dl_vlan_cfi(bool cfi)
{
	uint16_t v = (be16toh(vlan_hdr->hdr) & ~0x0008) | cfi;
	vlan_hdr->hdr = htobe16(v);
}


bool
fvlanframe::get_dl_vlan_cfi() const
{
	uint16_t v = be16toh(vlan_hdr->hdr);
	return ((v & 0x0008) >> 3);
}


void
fvlanframe::set_dl_type(uint16_t dl_type) throw (eFrameInval)
{
	initialize();

	if (!vlan_hdr)
	{
		throw eFrameInval();
	}

	vlan_hdr->dl_type = htobe16(dl_type);
}


uint16_t
fvlanframe::get_dl_type() const
{
	return be16toh(vlan_hdr->dl_type);
}



const char*
fvlanframe::c_str()
{
	cvastring vas;

	info.assign(vas("[fvlanframe(%p) vid:%d pcp:%d cfi:%d %s]",
			this,
			get_dl_vlan_id(),
			get_dl_vlan_pcp(),
			get_dl_vlan_cfi(),
			fframe::c_str()));

	return info.c_str();
}
