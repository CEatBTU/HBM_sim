#ifndef RW_STRIDE_CPU_HPP
#define RW_STRIDE_CPU_HPP

#include "cpu/CPU.hpp"

class RW_StrideCPU : public CPU_IF{
    public:
	SC_HAS_PROCESS(RW_StrideCPU);
	RW_StrideCPU(sc_module_name name, unsigned bInByte, unsigned freqInMHz, Terminator& t,
	       	uint64_t A, uint64_t S, uint64_t B,
	       	uint64_t W, uint64_t R, unsigned C, bool sR);
    	~RW_StrideCPU();
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
#endif /* RW_STRIDE_CPU_HPP */
