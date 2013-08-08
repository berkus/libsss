#pragma once

#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace ssu {
namespace async {

class timer;

/**
 * Timer engine provides a substitutable interface to create and arm timers.
 * A custom timer engine subclass is used for simulation purposes, allowing whole
 * protocol to run on simulated time.
 */
class timer_engine
{
	timer* origin_{0};
public:
	typedef boost::posix_time::time_duration duration_type;

	timer_engine(timer* t) : origin_(t) {}

	virtual void start(duration_type interval) = 0;
	virtual void stop() = 0;

protected:
	/**
	 * Signal timeout on the origin timer.
	 * Subclasses of timer_engine call this method when requested time interval expires.
	 */
	void timeout();
};

}
}
