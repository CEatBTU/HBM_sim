#ifndef MAO_HPP
#define MAO_HPP

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/multi_passthrough_target_socket.h>
#include <tlm_utils/multi_passthrough_initiator_socket.h>

class MAO_IF : public sc_module{
    public:
	tlm_utils::multi_passthrough_target_socket<MAO_IF> tSocket;
	tlm_utils::multi_passthrough_initiator_socket<MAO_IF> iSocket;

    protected:
	MAO_IF(sc_module_name name);

	virtual tlm::tlm_sync_enum nb_transport_fw(int id,
		tlm::tlm_generic_payload& gp,
		tlm::tlm_phase& phase, sc_time& delay) = 0;
	virtual tlm::tlm_sync_enum nb_transport_bw(int id,
		tlm::tlm_generic_payload& gp,
		tlm::tlm_phase& phase, sc_time& delay) = 0;
	virtual tlm::tlm_sync_enum innerTarget_nb_transport_fw(int id,
		tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase,
		sc_time& delay) = 0;
	virtual tlm::tlm_sync_enum innerInitiator_nb_transport_bw(int id,
		tlm::tlm_generic_payload& gp,
		tlm::tlm_phase& phase, sc_time& delay) = 0;
    
    protected:
	tlm_utils::multi_passthrough_target_socket<MAO_IF> innerTarget;
	tlm_utils::multi_passthrough_initiator_socket<MAO_IF> innerInitiator;
};
#endif // MAO_HPP
