/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "cofaction.h"

using namespace rofl;

cofaction::cofaction(
	size_t datalen) :
	action(datalen)
{
	WRITELOG(COFACTION, DBG, "cofaction(%p)::cofaction()", this);
	oac_header = (struct ofp_action_header*)action.somem();
}


cofaction::cofaction(
	struct ofp_action_header* achdr,
	size_t aclen) throw (eBadActionBadLen, eBadActionBadOutPort) :
	action(aclen)
{
	WRITELOG(COFACTION, DBG, "cofaction(%p)::cofaction()", this);
	oac_header = (struct ofp_action_header*)action.somem();

	if (be16toh(oac_header->len) > aclen)
	{
		throw eBadActionBadLen();
	}

	unpack(achdr, aclen);

	__make_info();
}


cofaction::~cofaction()
{
	WRITELOG(COFACTION, DBG, "cofaction(%p)::~cofaction()", this);
}


cofaction::cofaction(cofaction const& action)
{
	*this = action;
}


cofaction&
cofaction::operator= (const cofaction& ac)
{
	if (this == &ac)
		return *this;

	this->action = ac.action;

	this->oac_header = (struct ofp_action_header*)this->action.somem();

	return *this;
}


struct ofp_action_header*
cofaction::pack(
	struct ofp_action_header* achdr,
	size_t aclen) const throw (eActionInval)
{
	if (aclen < this->length())
		throw eActionInval();

	//WRITELOG(CSYSLOG_ACTION, "soaction()=%p actionlen()=%d", soaction(), actionlen());

	oac_header->len = htobe16(length());

	memcpy((uint8_t*)achdr, (uint8_t*)soaction(), length());
	return achdr;
}


void
cofaction::unpack(
	struct ofp_action_header *achdr,
	size_t aclen)
throw (eBadActionBadLen, eBadActionBadOutPort, eBadActionBadType)
{
	if (action.memlen() < aclen)
	{
		oac_header = (struct ofp_action_header*)action.resize(aclen);;
	}

	memcpy((uint8_t*)oac_header, (uint8_t*)achdr, aclen);

	WRITELOG(COFACTION, DBG, "cofaction(%p)::unpack() oac_header: %p action: %s",
			this, oac_header, action.c_str());

	if (be16toh(oac_header->len) < sizeof(struct ofp_action_header))
		throw eBadActionBadLen();


	switch (be16toh(oac_header->type)) {
	case OFPAT_OUTPUT:
		{
			oac_output = (struct ofp_action_output*)oac_header;
			if (action.memlen() < sizeof(struct ofp_action_output))
			{
				throw eBadActionBadLen();
			}
			uint32_t port_no = be32toh(oac_output->port);
			if ((OFPP_ANY == port_no) || (0 == port_no))
			{
				throw eBadActionBadOutPort();
			}
		}
		break;
	case OFPAT_SET_FIELD:
		oac_set_field = (struct ofp_action_set_field*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_set_field))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_COPY_TTL_OUT:
		// only generic oac_header is used
		if (action.memlen() < sizeof(struct ofp_action_header))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_COPY_TTL_IN:
		// only generic oac_header is used
		if (action.memlen() < sizeof(struct ofp_action_header))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_SET_MPLS_TTL:
		oac_mpls_ttl = (struct ofp_action_mpls_ttl*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_mpls_ttl))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_DEC_MPLS_TTL:
		oac_push = (struct ofp_action_push*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_push))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_PUSH_VLAN:
		oac_push = (struct ofp_action_push*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_push))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_POP_VLAN:
		// only generic oac_header is used
		if (action.memlen() < sizeof(struct ofp_action_header))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_PUSH_MPLS:
		oac_push = (struct ofp_action_push*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_push))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_POP_MPLS:
		oac_pop_mpls = (struct ofp_action_pop_mpls*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_pop_mpls))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_SET_QUEUE:
		oac_set_queue = (struct ofp_action_set_queue*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_set_queue))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_GROUP:
		oac_group = (struct ofp_action_group*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_group))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_SET_NW_TTL:
		oac_nw_ttl = (struct ofp_action_nw_ttl*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_nw_ttl))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_DEC_NW_TTL:
		// only generic oac_header is used
		if (action.memlen() < sizeof(struct ofp_action_header))
		{
			throw eBadActionBadLen();
		}
		break;
	case OFPAT_EXPERIMENTER:
		oac_experimenter_header = (struct ofp_action_experimenter_header*)oac_header;
		if (action.memlen() < sizeof(struct ofp_action_experimenter_header))
		{
			throw eBadActionBadLen();
		}
		break;
	default:
		WRITELOG(COFACTION, DBG, "cofaction(%p)::__parse_action() invalid action type %d => %s", this, be16toh(oac_header->type), action.c_str());
		throw eBadActionBadType();
		//throw eActionInvalType();
		break;
	}
	
}


struct ofp_action_header*
cofaction::soaction() const
{
	return (struct ofp_action_header*)(action.somem());
}	


size_t
cofaction::length() const
{
	switch (be16toh(oac_header->type)) {
	case OFPAT_OUTPUT:
		return sizeof(struct ofp_action_output);

	case OFPAT_COPY_TTL_OUT:
		return sizeof(ofp_action_header);

	case OFPAT_COPY_TTL_IN:
		return sizeof(ofp_action_header);

	case OFPAT_SET_MPLS_TTL:
		return sizeof(struct ofp_action_mpls_ttl);

	case OFPAT_DEC_MPLS_TTL:
		return sizeof(ofp_action_header);

	case OFPAT_PUSH_VLAN:
		return sizeof(struct ofp_action_push);

	case OFPAT_POP_VLAN:
		return sizeof(ofp_action_header);

	case OFPAT_PUSH_MPLS:
		return sizeof(struct ofp_action_push);

	case OFPAT_POP_MPLS:
		return sizeof(ofp_action_pop_mpls);

	case OFPAT_SET_QUEUE:
		return sizeof(struct ofp_action_set_queue);

	case OFPAT_GROUP:
		return sizeof(struct ofp_action_group);

	case OFPAT_SET_NW_TTL:
		return sizeof(struct ofp_action_nw_ttl);

	case OFPAT_DEC_NW_TTL:
		return sizeof(ofp_action_header);

	case OFPAT_EXPERIMENTER:
		return sizeof(struct ofp_action_experimenter_header);

	case OFPAT_SET_FIELD:
		return action.memlen();

	default: {
		return action.memlen();
#if 0
		cofaction tmp(*this);
		WRITELOG(COFACTION, DBG, "cofaction(%p)::actionlen() unknown action type %d => action: %s", this, be16toh(oac_header->type), tmp.c_str());
		throw eActionInvalType();
#endif
	}
	}
}


const char*
cofaction::c_str()
{
	__make_info();
#if 0
	cvastring vas;
	info.assign(vas("cofaction(%p)", this));
#endif
	return info.c_str();
}


void
cofaction::__make_info()
{
	cvastring vas(4096);

	switch (be16toh(oac_header->type)) {
	case OFPAT_OUTPUT:

#if 1
		info.assign(vas("cofaction(%p) OFPAT_OUTPUT length[%zu] port[0x%x] max_len[%d]",
				 this,
				 length(),
				 be32toh(oac_output->port),
				 be16toh(oac_output->max_len)));
#else
		info.assign(vas("cofaction(%p) OFPAT_OUTPUT length[%zu] port[0x%x] max_len[%d] body: %s",
				 this,
				 length(),
				 be32toh(oac_output->port),
				 be16toh(oac_output->max_len),
				 action.c_str()));
#endif
		break;

	case OFPAT_COPY_TTL_OUT:

		info.assign(vas(
				 "cofaction(%p) OFPAT_COPY_TTL_OUT length[%zu] ",
				 this,
				 length()));
		break;

	case OFPAT_COPY_TTL_IN:

		info.assign(vas(
				 "cofaction(%p) OFPAT_COPY_TTL_IN length[%zu] ",
				this,
				length()));
		break;

	case OFPAT_SET_MPLS_TTL:

		info.assign(vas(
				 "cofaction(%p) OFPAT_SET_MPLS_TTL length[%zu] mpls_ttl[%d]",
				 this,
				 length(),
				 oac_mpls_ttl->mpls_ttl));
		break;

	case OFPAT_DEC_MPLS_TTL:

		info.assign(vas(
				 "cofaction(%p) OFPAT_DEC_MPLS_TTL length[%zu] ",
				 this, length()));
		break;

	case OFPAT_PUSH_VLAN:

		info.assign(vas(
				 "cofaction(%p) OFPAT_PUSH_VLAN length[%zu] ethertype[0x%04x]",
				 this,
				 length(),
				 be16toh(oac_push->ethertype)));
		break;

	case OFPAT_POP_VLAN:

		info.assign(vas(
				 "cofaction(%p) OFPAT_POP_VLAN length[%zu] ",
				 this, length()));
		break;

	case OFPAT_PUSH_MPLS:

		info.assign(vas(
				 "cofaction(%p) OFPAT_PUSH_MPLS length[%zu] ethertype[0x%x]",
				 this,
				 length(),
				 be16toh(oac_push->ethertype)));
		break;

	case OFPAT_POP_MPLS:

		info.assign(vas(
				 "cofaction(%p) OFPAT_POP_MPLS length[%zu] ethertype[0x%x]",
				 this,
				 length(),
				 be16toh(oac_pop_mpls->ethertype)));
		break;

	case OFPAT_SET_QUEUE:

		info.assign(vas(
				 "cofaction(%p) OFPAT_SET_QUEUE length[%zu] queue_id[%d]",
				 this,
				 length(),
				 be32toh(oac_set_queue->queue_id)));
		break;

	case OFPAT_GROUP:

		info.assign(vas(
				 "cofaction(%p) OFPAT_GROUP length[%zu] group_id[%d]",
				 this,
				 length(),
				 be32toh(oac_group->group_id)));
		break;

	case OFPAT_SET_NW_TTL:

		info.assign(vas(
				 "cofaction(%p) OFPAT_SET_NW_TTL length[%zu] nw_ttl[%d]",
				 this,
				 length(),
				 oac_nw_ttl->nw_ttl));
		break;

	case OFPAT_DEC_NW_TTL:

		info.assign(vas(
				 "cofaction(%p) OFPAT_DEC_NW_TTL length[%zu] ",
				 this, length()));
		break;

	case OFPAT_EXPERIMENTER:
		info.assign(vas(
				 "cofaction(%p) OFPAT_EXPERIMENTER length[%zu] experimenter[%d]",
				 this,
				 length(),
				 be32toh(oac_experimenter_header->experimenter)));
		break;

	case OFPAT_SET_FIELD:
	{
		coxmatch oxm((struct ofp_oxm_hdr*)oac_set_field->field,
				be16toh(oac_set_field->len) - 2 * sizeof(uint16_t));
#if 1
		info.assign(vas(
				 "cofaction(%p) OFPAT_SET_FIELD length[%zu] [%s]",
				 this,
				 length(),
				 oxm.c_str()));
#else
		info.assign(vas(
				 "cofaction(%p) OFPAT_SET_FIELD length[%zu] [%s] body: %s",
				 this,
				 length(),
				 oxm.c_str(),
				 action.c_str()));
#endif
		break;
	}

	default:
		info.assign(vas("cofaction(%p) unknown action ", this));
		break;
	}
}



uint16_t
cofaction::get_type() const
{
	return be16toh(oac_header->type);
}


uint32_t
cofaction::get_port() throw (eActionInvalType)
{
	switch (get_type()) {
	case OFPAT_OUTPUT:
		return be32toh(oac_output->port);

	default:
		throw eActionInvalType();
	}
}


uint16_t
cofaction_output::get_max_len() const throw (eActionInvalType)
{
	switch (get_type()) {
	case OFPAT_OUTPUT:
		return be16toh(oac_output->max_len);

	default:
		throw eActionInvalType();
	}
}


coxmatch
cofaction::get_oxm() throw (eActionInvalType)
{
	if (OFPAT_SET_FIELD != get_type())
	{
		throw eActionInvalType();
	}

	struct ofp_oxm_hdr* oxm_hdr = (struct ofp_oxm_hdr*)oac_set_field->field;
	size_t oxm_len = be16toh(oac_set_field->len) - 2 * sizeof(uint16_t); // without action header

	coxmatch oxm(oxm_hdr, oxm_len);

	return oxm;
}

template class coflist<cofaction>;
