#ifndef XILINX_SWITCH_HPP
#define XILINX_SWITCH_HPP

#include <deque>
#include <cassert>

#include "Switch.hpp"
#include "json.hpp"

class XilinxSwitch : public Switch_IF{
    public:
	// sockets for the lateral connection
	XilinxSwitch(sc_module_name name, 
		unsigned bInByte, unsigned reqQs, unsigned respQs,
		unsigned freqMHz,
		std::vector<std::pair<uint64_t, uint64_t>> aR,
		unsigned vc, unsigned bc);
    private:
	void end_of_elaboration() override;
	tlm::tlm_sync_enum nb_transport_fw(int id, tlm::tlm_generic_payload& gp, 
		tlm::tlm_phase& phase, sc_time& delay) override;
	tlm::tlm_sync_enum nb_transport_bw(int id, tlm::tlm_generic_payload& gp,
	       	tlm::tlm_phase& phase, sc_time& delay) override;
	
	void peqCallback(tlm::tlm_generic_payload& _gp,
		const tlm::tlm_phase& _phase) override;
	unsigned route(tlm::tlm_generic_payload& gp) override;
	unsigned getBeats(tlm::tlm_generic_payload& gp, unsigned type);

    private:	
        bool boundary;
        uint64_t memorySizeInByte;
        uint64_t boundary_left;
        uint64_t boundary_right;
        uint64_t channelSize;
        std::vector<unsigned> addrVector;
	unsigned bilateralConnections;
	// CONNECTION | REQUEST/RESPONSE | READ/WRITE
	std::vector<std::vector<std::vector<unsigned>>> busyVector, prevVector;
	std::vector<std::vector<std::vector<std::deque<tlm::tlm_generic_payload*>>>>
	    pendingVector, queueVector;
};

class XilinxEndSwitch : sc_module{
    public:
	tlm_utils::multi_passthrough_target_socket<XilinxEndSwitch> tSocket;
	tlm_utils::multi_passthrough_initiator_socket<XilinxEndSwitch> iSocket;

	SC_HAS_PROCESS(XilinxEndSwitch);
	XilinxEndSwitch(sc_module_name name);

    private:
	tlm::tlm_sync_enum nb_transport_fw(int id,
		tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay);
};
#endif // XILINX_SWITCH_HPP
