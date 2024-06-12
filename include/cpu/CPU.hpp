#ifndef CPU_HPP
#define CPU_HPP

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/peq_with_cb_and_phase.h>

#include <string>
#include <iostream>
#include <iomanip> // std::setprecision

#include "Clock.hpp"
#include "MemoryManager.hpp"
#include "cpu/Terminator.hpp"

class CPU_IF : public sc_module{
    public:
	tlm_utils::simple_initiator_socket<CPU_IF> iSocket;

    protected:
	CPU_IF(sc_module_name name, unsigned bInByte, unsigned freqInMHz,
		Terminator& t);

    protected:
	tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& gp,
		tlm::tlm_phase& phase, sc_core::sc_time& delay);
	unsigned burstcnt(tlm::tlm_generic_payload& gp);
	void terminate();
	void add();
	virtual void peqCallback(tlm::tlm_generic_payload& gp,
		const tlm::tlm_phase& phase) = 0;
    protected:
	Clock clk;
	MemoryManager mm;
	sc_event recievedEndReq;

    private:
	unsigned buswidthInByte;	
	Terminator& terminator;
	tlm_utils::peq_with_cb_and_phase<CPU_IF> peq;
};
#endif // CPU_HPP
