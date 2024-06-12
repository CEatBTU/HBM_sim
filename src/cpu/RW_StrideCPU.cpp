#include "cpu/RW_StrideCPU.hpp"

RW_StrideCPU::RW_StrideCPU(sc_module_name name, unsigned bInByte,
       	unsigned freqInMHz, Terminator& t,
	uint64_t A, uint64_t S, uint64_t B,
	uint64_t W, uint64_t R, unsigned C, bool sR)
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

RW_StrideCPU::~RW_StrideCPU()
{
    delete[] buffer;
}

void RW_StrideCPU::peqCallback(tlm::tlm_generic_payload& gp, 
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

void RW_StrideCPU::send()
{
    unsigned tmp=0;
    add();
    tlm::tlm_command command = startREAD == true ?
	tlm::TLM_READ_COMMAND : tlm::TLM_WRITE_COMMAND;	
    bool just_once = true;
    for(unsigned i = 0; i < REP; i++){
        uint64_t address = START_ADDRESS + ((i * STRIDE) % WIDTH);
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

        //BEGIN NEW CODE
        //start of PCH16
        std::string mod_name;
        mod_name = this->name();
        int port_origin;
        port_origin = stoi(mod_name.substr(4));
        //std::cout<<"name: "<<port_origin<<endl;

        //if (((port_origin<16)&&(address>=8589934592))||((port_origin>=16)&&(address<8589934592))){ //inter-stack communication
        if (((port_origin<16)&&(address>=(16*this->WIDTH)))||((port_origin>=16)&&(address<(16*this->WIDTH)))){
            if(command== tlm::TLM_READ_COMMAND){
                tmp=8.5;
            }
            else if(command== tlm::TLM_WRITE_COMMAND){
                tmp=6.5;
            }
        }
        else{
            tmp=0;
        }
        //END NEW CODE

        sc_time delay = clk.nextPosedge(burstcnt(*gp)+ tmp);
        //sc_time delay = SC_ZERO_TIME;
        tlm::tlm_phase phase = tlm::BEGIN_REQ;
        iSocket->nb_transport_fw(*gp, phase, delay);
        wait(recievedEndReq);
        }
}
