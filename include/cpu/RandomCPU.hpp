#ifndef RANDOM_CPU_HPP
#define RANDOM_CPU_HPP

#include <random>

#include "cpu/CPU.hpp"

class RandomCPU : public CPU_IF{
    public:
	SC_HAS_PROCESS(RandomCPU);
	RandomCPU(sc_module_name name, unsigned bInByte, unsigned freqInMHz, Terminator& t,
		uint64_t A, uint64_t S, uint64_t B,
	       	uint64_t W, uint64_t R, bool sR);
    	~RandomCPU();
    private:
	void send();
	void peqCallback(tlm::tlm_generic_payload& gp, const tlm::tlm_phase& phase) override;

    private:
	uint64_t  START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP;
	bool send_READ;
	unsigned char* buffer = nullptr;
	unsigned counter = 0;
};
#endif /* RANDOM_CPU_HPP */
