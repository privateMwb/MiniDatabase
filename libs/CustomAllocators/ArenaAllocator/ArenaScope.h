#pragma once

#include "ArenaAllocator.h"

class [[nodiscard]] ArenaScope {
private:
	// Reference
	ArenaAllocator& arena;

public:
	// Constructor & Destructor
	explicit ArenaScope(ArenaAllocator& arena) :
		arena(arena) {
		arena.beginFrame();
	}

	~ArenaScope() {
		arena.endFrame();
	}

	ArenaScope(const ArenaScope&)            = delete;
	ArenaScope& operator=(const ArenaScope&) = delete;
	
	ArenaScope(ArenaScope&&)                 = delete;
	ArenaScope& operator=(ArenaScope&&)      = delete;
};
