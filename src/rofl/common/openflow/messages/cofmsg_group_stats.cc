#include "cofmsg_group_stats.h"

using namespace rofl;




cofmsg_group_stats_request::cofmsg_group_stats_request(
		uint8_t of_version,
		uint32_t xid,
		uint16_t flags,
		cofgroup_stats_request const& group_stats) :
	cofmsg_stats(of_version, xid, 0, flags),
	group_stats(group_stats)
{
	switch (of_version) {
	case openflow12::OFP_VERSION: {
		set_type(openflow12::OFPT_STATS_REQUEST);
		set_stats_type(openflow12::OFPST_GROUP);
		resize(sizeof(struct openflow12::ofp_stats_request) + sizeof(struct openflow12::ofp_group_stats_request));
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
}



cofmsg_group_stats_request::cofmsg_group_stats_request(
		cmemory *memarea) :
	cofmsg_stats(memarea)
{
	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		ofh_group_stats = soframe() + sizeof(struct openflow12::ofp_stats_request);
	} break;
	case openflow13::OFP_VERSION: {
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
}



cofmsg_group_stats_request::cofmsg_group_stats_request(
		cofmsg_group_stats_request const& stats)
{
	*this = stats;
}



cofmsg_group_stats_request&
cofmsg_group_stats_request::operator= (
		cofmsg_group_stats_request const& stats)
{
	if (this == &stats)
		return *this;

	cofmsg_stats::operator =(stats);

	ofh_group_stats = soframe();

	group_stats = stats.group_stats;

	return *this;
}



cofmsg_group_stats_request::~cofmsg_group_stats_request()
{

}



void
cofmsg_group_stats_request::reset()
{
	cofmsg_stats::reset();
}



uint8_t*
cofmsg_group_stats_request::resize(size_t len)
{
	cofmsg_stats::resize(len);
	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		ofh_group_stats = soframe() + sizeof(struct openflow12::ofp_stats_request);
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
	return soframe();
}



size_t
cofmsg_group_stats_request::length() const
{
	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		return (sizeof(struct openflow12::ofp_stats_request) + sizeof(struct openflow12::ofp_group_stats_request));
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
	return 0;
}



void
cofmsg_group_stats_request::pack(uint8_t *buf, size_t buflen)
{
	cofmsg_stats::pack(buf, buflen); // copies common statistics header

	if ((0 == buf) || (0 == buflen))
		return;

	if (buflen < length())
		throw eInval();


	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		if (buflen < length())
			throw eInval();
		group_stats.pack(buf + sizeof(struct openflow12::ofp_stats_request), sizeof(struct openflow12::ofp_group_stats_request));
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
}



void
cofmsg_group_stats_request::unpack(uint8_t *buf, size_t buflen)
{
	cofmsg_stats::unpack(buf, buflen);

	validate();
}



void
cofmsg_group_stats_request::validate()
{
	cofmsg_stats::validate(); // check generic statistics header

	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		if (get_length() < (sizeof(struct openflow12::ofp_stats_request) + sizeof(struct openflow12::ofp_group_stats_request)))
			throw eBadSyntaxTooShort();
		group_stats.set_version(openflow12::OFP_VERSION);
		ofh_group_stats = soframe() + sizeof(struct openflow12::ofp_stats_request);
		group_stats.unpack(soframe() + sizeof(struct openflow12::ofp_stats_request), sizeof(struct openflow12::ofp_group_stats_request));
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadRequestBadVersion();
	}
}



cofgroup_stats_request&
cofmsg_group_stats_request::get_group_stats()
{
	return group_stats;
}





cofmsg_group_stats_reply::cofmsg_group_stats_reply(
		uint8_t of_version,
		uint32_t xid,
		uint16_t flags,
		std::vector<cofgroup_stats_reply> const& group_stats) :
	cofmsg_stats(of_version, xid, 0, flags),
	group_stats(group_stats)
{
	switch (of_version) {
	case openflow12::OFP_VERSION: {
		set_type(openflow12::OFPT_STATS_REPLY);
		set_stats_type(openflow12::OFPST_GROUP);
		resize(length());
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
}



cofmsg_group_stats_reply::cofmsg_group_stats_reply(
		cmemory *memarea) :
	cofmsg_stats(memarea)
{
	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		ofh_group_stats = soframe() + sizeof(struct openflow12::ofp_stats_reply);
	} break;
	case openflow13::OFP_VERSION: {
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
}



cofmsg_group_stats_reply::cofmsg_group_stats_reply(
		cofmsg_group_stats_reply const& stats)
{
	*this = stats;
}



cofmsg_group_stats_reply&
cofmsg_group_stats_reply::operator= (
		cofmsg_group_stats_reply const& stats)
{
	if (this == &stats)
		return *this;

	cofmsg_stats::operator =(stats);

	ofh_group_stats = soframe();

	return *this;
}



cofmsg_group_stats_reply::~cofmsg_group_stats_reply()
{

}



void
cofmsg_group_stats_reply::reset()
{
	cofmsg_stats::reset();
}



uint8_t*
cofmsg_group_stats_reply::resize(size_t len)
{
	cofmsg_stats::resize(len);
	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		ofh_group_stats = soframe() + sizeof(struct openflow12::ofp_stats_reply);
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
	return soframe();
}



size_t
cofmsg_group_stats_reply::length() const
{
	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		size_t len = sizeof(struct openflow12::ofp_stats_reply);
		for (std::vector<cofgroup_stats_reply>::const_iterator
				it = group_stats.begin(); it != group_stats.end(); ++it) {
			len += (*it).length();
		}
		return len;
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
	return 0;
}



void
cofmsg_group_stats_reply::pack(uint8_t *buf, size_t buflen)
{
	cofmsg_stats::pack(buf, buflen); // copies common statistics header

	if ((0 == buf) || (0 == buflen))
		return;

	if (buflen < length())
		throw eInval();

	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		if (buflen < length())
			throw eInval();
		size_t offset = 0;
		for (unsigned int i = 0; i < group_stats.size(); i++) {
			group_stats[i].pack(buf + sizeof(struct openflow12::ofp_stats_reply) + offset, group_stats[i].length());
			offset += group_stats[i].length();
		}
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadVersion();
	}
}



void
cofmsg_group_stats_reply::unpack(uint8_t *buf, size_t buflen)
{
	cofmsg_stats::unpack(buf, buflen);

	validate();
}



void
cofmsg_group_stats_reply::validate()
{
	cofmsg_stats::validate(); // check generic statistics header

	group_stats.clear();

	switch (get_version()) {
	case openflow12::OFP_VERSION: {
		if (get_length() < (sizeof(struct openflow12::ofp_stats_reply) + sizeof(struct openflow12::ofp_group_stats)))
			throw eBadSyntaxTooShort();

		size_t residual = get_length() - sizeof(struct openflow12::ofp_stats_reply);

		while (residual >= sizeof(struct openflow12::ofp_group_stats)) {

			uint8_t* p_group_stats_reply = soframe() + get_length() - residual;
			size_t length = be16toh(((struct openflow12::ofp_group_stats*)p_group_stats_reply)->length);

			cofgroup_stats_reply group_stats_reply(openflow12::OFP_VERSION);
			group_stats_reply.unpack(p_group_stats_reply, length);
			group_stats.push_back(group_stats_reply);
		}
	} break;
	case openflow13::OFP_VERSION: {
		// TODO
		throw eNotImplemented();
	} break;
	default:
		throw eBadRequestBadVersion();
	}
}



std::vector<cofgroup_stats_reply>&
cofmsg_group_stats_reply::get_group_stats()
{
	return group_stats;
}


