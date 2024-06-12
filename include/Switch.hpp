#ifndef SWITCH_HPP
#define SWITCH_HPP

#define SC_INCLUDE_DYNAMIC_PROCESSES // for sc_core::sc_spawn
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>

#include <utility> // std::pair
#include <vector>
#include <map>
#include <stdexcept> // std::out_of_range

#include "Clock.hpp"

class Switch_IF : public sc_module{
    public:
	tlm_utils::multi_passthrough_target_socket<Switch_IF> tSocket;
	tlm_utils::multi_passthrough_initiator_socket<Switch_IF> iSocket;

	SC_HAS_PROCESS(Switch_IF);
	Switch_IF(sc_module_name name, 
		unsigned bInByte, unsigned reqQs, unsigned respQs,
		unsigned freqMHz, 
		std::vector<std::pair<uint64_t, uint64_t>> aR, 
		unsigned vc);
	virtual ~Switch_IF() = default;
	
	sc_time getFirstArrival();
	sc_time getLastDeparture();
	uint64_t getProcessedData();
	uint64_t getCrossedData();
	bool getFirstVisited();

    protected:
	void end_of_elaboration() override;
	virtual tlm::tlm_sync_enum nb_transport_fw(int id,
		tlm::tlm_generic_payload& gp, 
		tlm::tlm_phase& phase, sc_time& delay) = 0;
	virtual tlm::tlm_sync_enum nb_transport_bw(int id, 
		tlm::tlm_generic_payload& gp,
	       	tlm::tlm_phase& phase, sc_time& delay) = 0;
	/* handles payload */
	virtual void peqCallback(tlm::tlm_generic_payload& gp,
		const tlm::tlm_phase& phase) = 0;
	/* calculates index of socket */
	virtual unsigned route(tlm::tlm_generic_payload& gp) = 0;
	/* store/return initiator socket and it's id a.k.a. the busmaster */
	void setOrigin(tlm::tlm_generic_payload& gp, unsigned BM);
	unsigned getOrigin(tlm::tlm_generic_payload& gp);
	void deleteOrigin(tlm::tlm_generic_payload& gp);
	
	unsigned getDestination(tlm::tlm_generic_payload& gp);
	void setDestination(tlm::tlm_generic_payload& gp, unsigned BM);
	void deleteDestination(tlm::tlm_generic_payload& gp);
    
    protected:
	const unsigned REQUEST = 0, RESPONSE = 1, FREE = 0, BUSY = 1;
	tlm_utils::peq_with_cb_and_phase<Switch_IF> peq;
	unsigned buswidthInByte, requestQueueSize, responseQueueSize;
	Clock clk;
	std::vector<std::pair<uint64_t , uint64_t>> addressRegions;
	unsigned verticalConnections;
	// for measurements
	uint64_t processedData = 0;
	uint64_t crossedData = 0;
	bool firstVisited = false;
	sc_time firstArrival = SC_ZERO_TIME;
	sc_time lastDeparture = SC_ZERO_TIME;
   
    private:
	// to find the way back and forth
	std::map<tlm::tlm_generic_payload*, unsigned> originMap, destinationMap;
};
#endif // SWITCH_HPP
