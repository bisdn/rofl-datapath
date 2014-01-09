/*
 * queuetest.cc
 *
 *  Created on: 04.05.2013
 *      Author: andi
 */

#include "queuetest.h"


queuetest::queuetest(
		cofhello_elem_versionbitmap const& versionbitmap) :
			crofbase(versionbitmap)
{

}



queuetest::~queuetest()
{

}



void
queuetest::handle_timeout(
		int opaque)
{
	switch (opaque) {
	case QUEUETEST_TIMER_GET_CONFIG_INTERVAL: {
		register_timer(QUEUETEST_TIMER_GET_CONFIG_INTERVAL, 15);
		fprintf(stderr, "C");
		for (std::set<crofdpt*>::iterator
				it = dpaths.begin(); it != dpaths.end(); ++it) {
			(*(*it)).send_queue_get_config_request(openflow12::OFPP_ALL);
		}
	} break;
	case QUEUETEST_TIMER_STATS_INTERVAL: {
		fprintf(stderr, "S");
		register_timer(QUEUETEST_TIMER_STATS_INTERVAL, 5);
		for (std::set<crofdpt*>::iterator
				it = dpaths.begin(); it != dpaths.end(); ++it) {
			(*(*it)).send_queue_stats_request(
					0,
					cofqueue_stats_request(
							(*it)->get_version(),
							openflow12::OFPP_ALL,
							OFPQ_ALL));
		}
	} break;
	default: {
		crofbase::handle_timeout(opaque);
	} break;
	}
}



void
queuetest::handle_dpath_open(
		crofdpt *dpt)
{
	if (dpaths.empty()) {
		reset_timer(QUEUETEST_TIMER_GET_CONFIG_INTERVAL, 1);
		reset_timer(QUEUETEST_TIMER_STATS_INTERVAL, 10);
	}
	dpaths.insert(dpt);
}



void
queuetest::handle_dpath_close(
		crofdpt *dpt)
{
	dpaths.erase(dpt);
}



void
queuetest::handle_queue_get_config_reply(
			crofdpt *dpt,
			cofmsg_queue_get_config_reply *msg)
{
	//fprintf(stderr, "queue-get-config-reply: msg:%p\n", msg);

	std::cerr << "queue-get-config-reply => " << *msg << std::endl;

	delete msg;
}



void
queuetest::handle_queue_stats_reply(
			crofdpt *dpt,
			cofmsg_queue_stats_reply *msg)
{
	//fprintf(stderr, "queue-stats-reply: msg:%p\n", msg);

	std::cerr << "queue-stats-reply => " << *msg << std::endl;

	delete msg;
}


