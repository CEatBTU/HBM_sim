#include "cpu/RW_RandomCPU.hpp"

RW_RandomCPU::RW_RandomCPU(sc_module_name name, unsigned bInByte, 
	unsigned freqInMHz, Terminator& t,
	uint64_t A, uint64_t S, uint64_t B,
	uint64_t W, uint64_t R, uint64_t C, bool sR)
    : CPU_IF(name, bInByte, freqInMHz, t),
    START_ADDRESS(A), STRIDE(S), BURST_LENGTH(B),
    WIDTH(W), REP(R), CHUNK_SIZE(C), startREAD(sR) 
{
    if(STRIDE == 0 || BURST_LENGTH == 0 || WIDTH == 0 || CHUNK_SIZE == 0)
	SC_REPORT_FATAL(this->name(), "invalid arguments");
    if(REP != 0)
	SC_THREAD(send);
    buffer = new unsigned char[BURST_LENGTH];
}

RW_RandomCPU::~RW_RandomCPU()
{
    delete[] buffer;
}

void RW_RandomCPU::peqCallback(tlm::tlm_generic_payload& gp, 
	const tlm::tlm_phase& phase)
{
    if(phase == tlm::END_REQ){
    	recievedEndReq.notify(SC_ZERO_TIME);
    }
    else if(phase == tlm::BEGIN_RESP){
	counter++;
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
	if(counter % CHUNK_SIZE == 0)
	    arrival.notify(SC_ZERO_TIME);
	if(counter == REP)
	    terminate();
    }
    else{
        SC_REPORT_FATAL("RWStrideCPU", "PEQ was triggered with unknown phase");
    }
}

void RW_RandomCPU::send()
{
    add();
    tlm::tlm_command command = startREAD == true ?
	tlm::TLM_READ_COMMAND : tlm::TLM_WRITE_COMMAND;	
    bool just_once = true;
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    cout<<"\nWIDTH: "<<WIDTH<<"\n";
    cout<<"\nSTART_ADDRESS: "<<START_ADDRESS<<"\n";
    std::uniform_int_distribution<uint64_t> distrib(0, WIDTH);
    for(unsigned i = 0; i < REP; i++){
	uint64_t address = START_ADDRESS + distrib(gen);
        //uint64_t address = rand() % WIDTH ;
	tlm::tlm_generic_payload* gp = mm.allocate();
	if(i != 0 && i % CHUNK_SIZE == 0){
	    startREAD = !startREAD;
	    command = startREAD == true ?
		tlm::TLM_READ_COMMAND : tlm::TLM_WRITE_COMMAND;
	    if(just_once){
		wait(arrival);
		just_once = false;
	    }
	}
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
