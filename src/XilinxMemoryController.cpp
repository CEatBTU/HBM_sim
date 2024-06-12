#include "XilinxMemoryController.hpp"

XilinxMemoryController::XilinxMemoryController(sc_module_name name, 
	unsigned bInByte, unsigned reqQs, unsigned respQs,	
	unsigned freqInMHz, unsigned freqMC)
    : sc_module(name),
    peq(this, &XilinxMemoryController::peqCallback),
    pendingVector(2, std::vector<std::deque<tlm::tlm_generic_payload*>>(2)),
    queueVector(2, std::vector<std::deque<tlm::tlm_generic_payload*>>(2)),
    busyVector(2, std::vector<unsigned>(2, FREE)),
    buswidthInByte(bInByte), 
    requestQueueSize(reqQs), responseQueueSize(respQs),
    clk("clk", freqInMHz),  
    tCK(1000.0 / freqMC, SC_NS)
{
    if(buswidthInByte == 0){
	SC_REPORT_FATAL(this->name(), "'buswidthInByte' invalid");
    }
    tSocket.register_nb_transport_fw(this, &XilinxMemoryController::nb_transport_fw);
    iSocket.register_nb_transport_bw(this, &XilinxMemoryController::nb_transport_bw);
}

tlm::tlm_sync_enum XilinxMemoryController::nb_transport_fw(
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{ 
    sc_time n_delay = delay;
    if(phase == tlm::BEGIN_REQ){
	// synchronize with MC frequency
	sc_time clockOffset = (sc_time_stamp() + n_delay) % tCK;
	n_delay = (clockOffset == SC_ZERO_TIME) ? n_delay :
	    (n_delay + tCK - clockOffset);
	gp.acquire();
	// save address, because DRAMSys changes address
	addressMemory.emplace(&gp, gp.get_address()); 
    }
    peq.notify(gp, phase, n_delay);
    return tlm::TLM_ACCEPTED;
}

tlm::tlm_sync_enum XilinxMemoryController::nb_transport_bw(
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    if(phase == tlm::BEGIN_RESP){
	// restore address
	gp.set_address(addressMemory[&gp]);
	addressMemory.erase(&gp);
    }
    peq.notify(gp, phase, delay);
    return tlm::TLM_ACCEPTED;
}

/* NOTE: requestQueue and requestPending are shared between R and W */
void XilinxMemoryController::peqCallback(tlm::tlm_generic_payload& _gp,
	const tlm::tlm_phase& _phase)
{
    unsigned RoW = _gp.is_read();
    unsigned addr = _gp.get_address();
    if(_phase == tlm::BEGIN_REQ){
	RoW = 0;
	unsigned TRANS = REQUEST;
	unsigned queueMax = requestQueueSize;
	unsigned& busy = busyVector[TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& pending = pendingVector[TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& queue = queueVector[TRANS][RoW];
	// accept TRANS if possible
	if(busy == FREE || queue.size() < queueMax){
	    tlm::tlm_generic_payload& gp = _gp;
	    sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::END_REQ;
	    tSocket->nb_transport_bw(gp, phase, delay);
	}
	if(busy == FREE){
            //cout<<"\nenter mem ctrl BEGIN_REQ";
	    busy = BUSY;
	    tlm::tlm_generic_payload& gp = _gp;
            //here delay?
	    //sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::BEGIN_REQ;
	    unsigned additionalDelay = 0;
            int diff = abs( static_cast<int> (addr-prevAddr) );
            //cout<<"\naddr: "<<addr<<" prev addr: "<<prevAddr<<" diff: "<<diff;
            if( diff  > pageSize ) {
                     //cout<<"\n extra delay random ";

                     additionalDelay += 0;//52 (for 226 single transaction) ;//117 for 443 (signle transaction); //32 for benchmarks, 0 for applications
                     if (_gp.is_read())
                          additionalDelay += 0;
            }
	    prevAddr = addr ;
	    unsigned beats = 0;
            beats += additionalDelay;
	    sc_time delay = clk.nextPosedge(beats);
	    iSocket->nb_transport_fw(gp, phase, delay);
	}
	else if(queue.size() < queueMax)
	    queue.push_back(&_gp);
	else
	    pending.push_back(&_gp);
    }
    else if(_phase == tlm::END_REQ){ // from target
	RoW = 0;
	unsigned TRANS = REQUEST;
	unsigned queueMax = requestQueueSize;
	unsigned& busy = busyVector[TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& pending = pendingVector[TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& queue = queueVector[TRANS][RoW];
	// accept pending
	if(!pending.empty() && queue.size() < queueMax){
	    tlm::tlm_generic_payload& gp = *pending.front();
	    sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::END_REQ;
	    tSocket->nb_transport_bw(gp, phase, delay);
	    // put in queue
	    pending.pop_front();
	    queue.push_back(&gp);
	}
	// send transaction
	if(!queue.empty()){
	   tlm::tlm_generic_payload& gp = *queue.front();
	    //sc_time delay = SC_ZERO_TIME;

            unsigned additionalDelay = 0;
            unsigned length = gp.get_data_length();
	    unsigned buswidth = buswidthInByte;
	    unsigned cnt = length / buswidth + (length % buswidth == 0 ? 0 : 1);
            int diff = abs( static_cast<int> (addr-prevAddr) );
            //cout<<"\naddr: "<<addr<<" prev addr: "<<prevAddr<<" diff: "<<diff;
            if( diff  > pageSize ) {
                     //cout<<"\n extra delay random mem ctrl ";
                     if (_gp.is_read())
                        additionalDelay += 3 ; //3 original
                     else
                        additionalDelay += 2 ; //2 original
                     if (cnt == 2)
                        additionalDelay ++ ;  //++ original
                     
                     
            }
            else if (cnt == 1)
                   additionalDelay +=2;         //  2 original ------------------> put this block back for microbenchamrks
	    prevAddr = addr ;
	    unsigned beats = 0;
            beats += additionalDelay;
	    sc_time delay = clk.nextPosedge(beats);

	    tlm::tlm_phase phase = tlm::BEGIN_REQ;
	    iSocket->nb_transport_fw(gp, phase, delay);
	    // remove from queue
	    queue.pop_front();
	}
	else
	    busy = FREE;
    }
    else if(_phase == tlm::BEGIN_RESP){ // from target
	unsigned TRANS = RESPONSE;
	unsigned queueMax = responseQueueSize;
	unsigned& busy = busyVector[TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& pending = pendingVector[TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& queue = queueVector[TRANS][RoW];
	// accept TRANS if possible
	if(busy == FREE || queue.size() < queueMax){
	    tlm::tlm_generic_payload& gp = _gp;
	    sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::END_RESP;
	    iSocket->nb_transport_fw(gp, phase, delay);
	}
	if(busy == FREE){
	    busy = BUSY;
	    tlm::tlm_generic_payload& gp = _gp;
	    unsigned length = gp.get_data_length();
	    unsigned buswidth = buswidthInByte;
	    unsigned cnt = length / buswidth + (length % buswidth == 0 ? 0 : 1);
	    // one dead cycle if beat == 1
	     unsigned beats;
            //if (cnt == 1)
              //  beats = 1;
            //else if (gp.is_write()){
              //     beats = 1;
            //}
            //else
                   beats = cnt;
	    sc_time delay = clk.nextPosedge(beats);
	    tlm::tlm_phase phase = tlm::BEGIN_RESP;
	    tSocket->nb_transport_bw(gp, phase, delay);
	}
	else if(queue.size() < queueMax)
	    queue.push_back(&_gp);
	else
	    pending.push_back(&_gp);
    }
    else if(_phase == tlm::END_RESP){ // from initiator
	unsigned TRANS = RESPONSE;
	unsigned queueMax = responseQueueSize;
	unsigned& busy = busyVector[TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& pending = pendingVector[TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& queue = queueVector[TRANS][RoW];
	// accept pending
	if(!pending.empty() && queue.size() < queueMax){
	    tlm::tlm_generic_payload& gp = *pending.front();
	    sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::END_RESP;
	    iSocket->nb_transport_fw(gp, phase, delay);
	    // put in queue
	    pending.pop_front();
	    queue.push_back(&gp);
	}
	// send transaction
	if(!queue.empty()){
	    tlm::tlm_generic_payload& gp = *queue.front();
	    unsigned length = gp.get_data_length();
	    unsigned buswidth = buswidthInByte;
	    unsigned cnt = length / buswidth + (length % buswidth == 0 ? 0 : 1);
	    // one dead cycle if beat == 1
	    //unsigned beats = cnt == 1 ? 2 : gp.is_write() ? 1 : cnt; 
            unsigned beats;
            if (cnt == 1 ) {
                beats = 2;
            }
            else {
                beats = cnt;
            }
	    sc_time delay = clk.nextPosedge(beats);
	    tlm::tlm_phase phase = tlm::BEGIN_RESP;
	    tSocket->nb_transport_bw(gp, phase, delay);
	    // remove from queue
	    queue.pop_front();
	}
	else
	    busy = FREE;
	// clean up
	tlm::tlm_generic_payload& gp = _gp;
	gp.release();
    }
    else
	SC_REPORT_ERROR(name(), "unsupported phase");
}
