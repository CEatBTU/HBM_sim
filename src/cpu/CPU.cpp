#include "cpu/CPU.hpp"

/////////////////////////////// CPU_IF ///////////////////////////////
CPU_IF::CPU_IF(sc_module_name name, unsigned bInByte, unsigned freqInMHz,
	Terminator& t)
    : sc_module(name), buswidthInByte(bInByte), 
    clk("Clock", freqInMHz), terminator(t),
    peq(this, &CPU_IF::peqCallback)
{
    if(buswidthInByte == 0)
	SC_REPORT_FATAL(this->name(), "'buswidthInBit' invalid");
    iSocket.register_nb_transport_bw(this, &CPU_IF::nb_transport_bw);
}

tlm::tlm_sync_enum CPU_IF::nb_transport_bw(tlm::tlm_generic_payload& gp,
	tlm::tlm_phase& phase, sc_core::sc_time& delay){ 
    peq.notify(gp, phase, delay); 
    return tlm::TLM_ACCEPTED;
}

unsigned CPU_IF::burstcnt(tlm::tlm_generic_payload& gp)
{
    if(gp.is_read())
	return 1;
    return gp.get_data_length() / buswidthInByte + 
	(gp.get_data_length() % buswidthInByte == 0 ? 0 : 1);
}

void CPU_IF::terminate()
{
    terminator.terminate();
}

void CPU_IF::add()
{
    terminator.add();
}
