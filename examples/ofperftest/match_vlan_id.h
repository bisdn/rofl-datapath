#ifndef MATCH_VLAN_ID_H
#define MATCH_VLAN_ID_H 1

#include <map>
#include "rofl/common/cmacaddr.h"
#include "rofl/common/caddress.h"
#include "rofl/common/crofbase.h"
#include "rofl/common/crofdpt.h"

#include "ofperftest.h"

using namespace rofl;

class match_vlan_id :
		public ofperftest
{
private:

	struct fibentry_t {
		uint32_t 		port_no;	// port where a certain is attached
		time_t 			timeout;	// timeout event for this FIB entry
	};

	unsigned int 		n_entries;	// number of competing flowmods to be installed
	// a very simple forwarding information base
	std::map<crofdpt*, std::map<uint16_t, std::map<cmacaddr, struct fibentry_t> > > fib;

	unsigned int 		fib_check_timeout; 		// periodic timeout for removing expired FIB entries
	unsigned int		fm_delete_all_timeout;	// periodic purging of all FLOW-MODs

	enum match_vlan_id_timer_t {
		ETHSWITCH_TIMER_BASE = ((0x6271)),
		ETHSWITCH_TIMER_FIB,
		ETHSWITCH_TIMER_FLOW_MOD_DELETE_ALL,
	};

public:

	match_vlan_id(cofhello_elem_versionbitmap const& versionbitmap, unsigned int n_entries = 0);

	virtual
	~match_vlan_id();

	virtual void
	handle_timeout(int opaque);

	virtual void
	handle_dpath_open(crofdpt& dpt);

	virtual void
	handle_dpath_close(crofdpt& dpt);

	virtual void
	handle_packet_in(crofdpt& dpt, cofmsg_packet_in& msg, uint8_t aux_id);

private:

	void
	install_flow_mods(crofdpt *dpt, unsigned int n = 0);

	void
	drop_expired_fib_entries();

	void
	flow_mod_delete_all();
};

#endif
