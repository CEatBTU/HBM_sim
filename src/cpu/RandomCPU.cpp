#include "cpu/RandomCPU.hpp"

RandomCPU::RandomCPU(sc_module_name name, unsigned bInByte,
	unsigned freqInMHz, Terminator& t, 
	uint64_t A, uint64_t S, uint64_t B,
	uint64_t W, uint64_t R, bool sR)
    : CPU_IF(name, bInByte, freqInMHz, t),
    START_ADDRESS(A), STRIDE(S), BURST_LENGTH(B),
    WIDTH(W), REP(R), send_READ(sR) 
{
    if(STRIDE == 0 || BURST_LENGTH == 0 || WIDTH == 0)
	SC_REPORT_FATAL(this->name(), "invalid arguments");
    if(REP != 0)
	SC_THREAD(send);
    buffer = new unsigned char[BURST_LENGTH];
}

RandomCPU::~RandomCPU()
{
    delete[] buffer;
}

void RandomCPU::peqCallback(tlm::tlm_generic_payload& gp, 
	const tlm::tlm_phase& phase)
{
    if(phase == tlm::END_REQ){
	recievedEndReq.notify(SC_ZERO_TIME);
    }
    else if(phase == tlm::BEGIN_RESP){
	counter++;
        std::cout<<"\n gp.is_response_ok() "<<gp.is_response_ok()<<"\n";
	if(!gp.is_response_ok())
	    std::cerr << name() << " response error: " << (counter == 1 ? "1st" : 
		    counter == 2 ? "2nd" :
		    counter == 3 ? "3rd" :
		    std::to_string(counter) + "th") <<
		" Transaction failed; &gp = " << &gp << std::endl;

	tlm::tlm_generic_payload& n_gp = gp;
	tlm::tlm_phase n_phase = tlm::END_RESP;
	sc_time n_delay = SC_ZERO_TIME;
	iSocket->nb_transport_fw(n_gp, n_phase, n_delay);
	gp.release();
	if(counter == REP)
	    terminate();
    }
    else{
	SC_REPORT_FATAL("StrideCPU", "PEQ was triggered with unknown phase");
    }
}
void RandomCPU::send()
{
    add();
    tlm::tlm_command command = send_READ ?
	tlm::TLM_READ_COMMAND : tlm::TLM_WRITE_COMMAND;	
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()


    std::uniform_int_distribution<uint64_t> distrib(0, WIDTH);
    for(unsigned i = 0; i < REP; i++){
	uint64_t address = START_ADDRESS + distrib(gen);
        //uint64_t address = rand() % WIDTH ;
	tlm::tlm_generic_payload* gp = mm.allocate();
	gp->acquire();
	gp->set_command(command);
	gp->set_address(address);
	gp->set_data_ptr(buffer);
	gp->set_data_length(BURST_LENGTH);
	gp->set_streaming_width(BURST_LENGTH);
	gp->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
	sc_time delay = clk.nextPosedge(burstcnt(*gp));
	//sc_time delay = SC_ZERO_TIME;
	tlm::tlm_phase phase = tlm::BEGIN_REQ;
	iSocket->nb_transport_fw(*gp, phase, delay);
	wait(recievedEndReq);
    }
}
