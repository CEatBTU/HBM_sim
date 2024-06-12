#ifndef XILINX_MEMORY_CONTROLLER_HPP
#define XILINX_MEMORY_CONTROLLER_HPP

#include <vector>
#include <deque>
#include <map>

#define SC_INCLUDE_DYNAMIC_PROCESSES // for sc_core::sc_spawn
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>

#include "Clock.hpp"

class XilinxMemoryController : sc_module{
    public:
	tlm_utils::simple_target_socket<XilinxMemoryController> tSocket;
	tlm_utils::simple_initiator_socket<XilinxMemoryController> iSocket;

	SC_HAS_PROCESS(XilinxMemoryController);
	XilinxMemoryController(sc_module_name name, unsigned bInByte, 
		unsigned reqQs, unsigned respQs,
		unsigned freqInMHz, unsigned freqMC);
	~XilinxMemoryController() = default;

    private:
	tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& gp,
		tlm::tlm_phase& phase, sc_time& delay);
	tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& gp,
		tlm::tlm_phase& phase, sc_time& delay);
	void peqCallback(tlm::tlm_generic_payload& _gp, const tlm::tlm_phase& _phase);

    private:
	const unsigned REQUEST = 0, RESPONSE = 1, FREE = 0, BUSY = 1;
	tlm_utils::peq_with_cb_and_phase<XilinxMemoryController> peq;
	// REQUEST/RESPONSE | READ/WRITE
	std::vector<std::vector<std::deque<tlm::tlm_generic_payload*>>> 
	    pendingVector, queueVector;
	std::vector<std::vector<unsigned>> busyVector;
	int prevAddr = -1024 * 8 * 4 - 1;
        int pageSize = 1 * 1024 * 8 ;  // 1KB page

	unsigned buswidthInByte, buswidthToDRAMinByte = 8;
	unsigned requestQueueSize, responseQueueSize;
	Clock clk;
	sc_time tCK;

	std::map<tlm::tlm_generic_payload*, uint64_t> addressMemory;
};
#endif // XILINX_MEMORY_CONTROLLER_HPP
