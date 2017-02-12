#ifndef SWEEP_SERIAL_575F0FB571D1_H
#define SWEEP_SERIAL_575F0FB571D1_H

/*
 * Communication with serial devices.
 * Implementation detail; not exported.
 */

#include "sweep.h"
#include "Error.h"

#include <memory>

namespace sweep {
namespace serial {

struct Error : public ErrorBase {
	using ErrorBase::ErrorBase;
};

class Device {
public:
	Device(const char* port, int32_t bitrate);
	Device(Device&& other) = default;
	~Device();

	void read(void* to, int32_t len);
	void write(const void* from, int32_t len);
	void flush();
private:
	std::unique_ptr<struct State> _state;
};

} // ns serial
} // ns sweep

#endif
