#pragma once

#include "EmulationStrategy.h"

class NopStrategy : public EmulationStrategy {
public:
	NopStrategy() {}
	virtual ~NopStrategy() {}
	virtual void getEmulationMap(EmulationMapType *dest) {
		// Empty map - no key emulation
		dest->clear();
	}
};
