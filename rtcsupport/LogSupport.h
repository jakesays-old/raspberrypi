#ifndef LOGSUPPORT_H
#define LOGSUPPORT_H

#include <glog/logging.h>

#define logInfo \
	LOG(INFO)

#define logError \
	LOG(ERROR)

#define vlogHigh \
	VLOG(1)

#define vlogMed \
	VLOG(2)
	
#define vlogLow \
	VLOG(3)
	
#endif //LOGSUPPORT_H
