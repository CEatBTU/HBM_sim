#ifndef RW_RANDOM_CPU_HPP
#define RW_RANDOM_CPU_HPP

#include <random>

#include "cpu/CPU.hpp"

class RW_RandomCPU : public CPU_IF{
    public:
	SC_HAS_PROCESS(RW_RandomCPU);
	RW_RandomCPU(sc_module_name name, unsigned bInByte, unsigned freqInMHz, Terminator& t,
		uint64_t A, uint64_t S, uint64_t B,
	       	uint64_t W, uint64_t R, uint64_t C,
		bool sR);
    	~RW_RandomCPU();
    private:
	void send();
	void peqCallback(tlm::tlm_generic_payload& gp, const tlm::tlm_phase& phase) override;

    private:
	uint64_t  START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP, CHUNK_SIZE;
	bool startREAD;
	unsigned char* buffer = nullptr;
	sc_event arrival;
	unsigned counter = 0;
};
#endif /* RW_RANDOM_CPU_HPP */
