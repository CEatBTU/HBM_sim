#ifndef BUTTERFLY_SWITCH_HPP
#define BUTTERFLY_SWITCH_HPP

#include <deque>
#include <cassert>

#include "Switch.hpp"

class ButterflySwitch: public Switch_IF{
    public:
	// sockets for the lateral connection
	ButterflySwitch(sc_module_name name, 
		unsigned bInByte, unsigned reqQs, unsigned respQs,
		unsigned freqMHz,
		std::vector<std::pair<uint64_t, uint64_t>> aR,
		unsigned vc);
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
	// CONNECTION | REQUEST/RESPONSE | READ/WRITE
	std::vector<std::vector<std::vector<unsigned>>> busyVector;
	std::vector<std::vector<std::vector<std::deque<tlm::tlm_generic_payload*>>>>
	    pendingVector, queueVector;
};
#endif // BUTTERFLY_SWITCH_HPP
