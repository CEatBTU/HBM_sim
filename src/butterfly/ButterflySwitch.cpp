#include "butterfly/ButterflySwitch.hpp"

ButterflySwitch::ButterflySwitch(sc_module_name name, 
	unsigned bInByte, unsigned reqQs, unsigned respQs,
	unsigned freqMHz,
	std::vector<std::pair<uint64_t, uint64_t>> aR,
	unsigned vc) 
    : Switch_IF(name, bInByte, reqQs, respQs, freqMHz, aR, vc)
{
    // empty constructor
}

void ButterflySwitch::end_of_elaboration() 
{
    Switch_IF::end_of_elaboration();
    // init vectors
    unsigned maxSize = iSocket.size() > tSocket.size() ?
	iSocket.size() : tSocket.size();
    busyVector.resize(maxSize, 
	    std::vector<std::vector<unsigned>>
	    (2, std::vector<unsigned>(2, FREE)));
    queueVector.resize(maxSize, 
	    std::vector<std::vector<std::deque<tlm::tlm_generic_payload*>>>
	    (2, std::vector<std::deque<tlm::tlm_generic_payload*>>(2)));
    pendingVector.resize(maxSize, 
	    std::vector<std::vector<std::deque<tlm::tlm_generic_payload*>>>
	    (2, std::vector<std::deque<tlm::tlm_generic_payload*>>(2)));
}

tlm::tlm_sync_enum ButterflySwitch::nb_transport_fw(int id, 
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    if(phase == tlm::BEGIN_REQ){
	// to get only the first timestamp 
	if(!firstVisited){
	    firstArrival = sc_time_stamp();
	    firstVisited = true;
	}
	setOrigin(gp, static_cast<unsigned>(id));
	setDestination(gp, route(gp));
	gp.acquire();
    }
    peq.notify(gp, phase, delay);
    return tlm::TLM_ACCEPTED;
}

tlm::tlm_sync_enum ButterflySwitch::nb_transport_bw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    peq.notify(gp, phase, delay);
    return tlm::TLM_ACCEPTED;
}

void ButterflySwitch::peqCallback(tlm::tlm_generic_payload& _gp, 
	const tlm::tlm_phase& _phase)
{
    unsigned RoW = _gp.is_read(); 
    if(_phase == tlm::BEGIN_REQ){ // from initiator	
	unsigned connection = getDestination(_gp);
	unsigned TRANS = REQUEST;
	unsigned queueMax = requestQueueSize;
	unsigned& busy = busyVector[connection][TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& pending = 
	    pendingVector[connection][TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& queue = 
	    queueVector[connection][TRANS][RoW];
	// accept TRANS if possible
	if(busy == FREE || queue.size() < queueMax){
	    tlm::tlm_generic_payload& gp = _gp;
	    unsigned socketId = getOrigin(gp);
	    sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::END_REQ;
	    tSocket[static_cast<int>(socketId)]->nb_transport_bw(gp, phase, delay);
	}
	if(busy == FREE){
	    busy = BUSY;
	    tlm::tlm_generic_payload& gp = _gp;
	    unsigned socketId = getDestination(gp);
	    unsigned beats = getBeats(gp, TRANS);
	    sc_time delay = clk.nextPosedge(beats);
	    tlm::tlm_phase phase = tlm::BEGIN_REQ;
	    iSocket[static_cast<int>(socketId)]->nb_transport_fw(gp, phase, delay);
	}
	else if(queue.size() < queueMax)
	    queue.push_back(&_gp);
	else
	    pending.push_back(&_gp);
    }
    else if(_phase == tlm::END_REQ){ // from target
	unsigned connection = getDestination(_gp);
	unsigned TRANS = REQUEST;
	unsigned queueMax = requestQueueSize;
	unsigned& busy = busyVector[connection][TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& pending = 
	    pendingVector[connection][TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& queue = 
	    queueVector[connection][TRANS][RoW];
	// accept pending
	if(!pending.empty() && queue.size() < queueMax){
	    tlm::tlm_generic_payload& gp = *pending.front();
	    unsigned socketId = getOrigin(gp);
	    sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::END_REQ;
	    tSocket[static_cast<int>(socketId)]->nb_transport_bw(gp, phase, delay);
	    // put in queue
	    pending.pop_front();
	    queue.push_back(&gp);
	}
	// send transaction
	if(!queue.empty()){
	    tlm::tlm_generic_payload& gp = *queue.front();
	    unsigned socketId = getDestination(gp);
	    unsigned beats = getBeats(gp, TRANS);
	    sc_time delay = clk.nextPosedge(beats);
	    tlm::tlm_phase phase = tlm::BEGIN_REQ;
	    iSocket[static_cast<int>(socketId)]->nb_transport_fw(gp, phase, delay);
	    // remove from queue
	    queue.pop_front();
	}
	else
	    busy = FREE;
    }
    else if(_phase == tlm::BEGIN_RESP){ // from target
	unsigned connection = getOrigin(_gp);
	unsigned TRANS = RESPONSE;
	unsigned queueMax = responseQueueSize;
	unsigned& busy = busyVector[connection][TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& pending = 
	    pendingVector[connection][TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& queue = 
	    queueVector[connection][TRANS][RoW];
	// accept TRANS if possible
	if(busy == FREE || queue.size() < queueMax){
	    tlm::tlm_generic_payload& gp = _gp;
	    unsigned socketId = getDestination(gp);
	    sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::END_RESP;
	    iSocket[static_cast<int>(socketId)]->nb_transport_fw(gp, phase, delay);
	}
	if(busy == FREE){
	    busy = BUSY;
	    tlm::tlm_generic_payload& gp = _gp;
	    unsigned socketId = getOrigin(gp);
	    unsigned beats = getBeats(gp, TRANS);
	    sc_time delay = clk.nextPosedge(beats);
	    tlm::tlm_phase phase = tlm::BEGIN_RESP;
	    tSocket[static_cast<int>(socketId)]->nb_transport_bw(gp, phase, delay);
	}
	else if(queue.size() < queueMax)
	    queue.push_back(&_gp);
	else
	    pending.push_back(&_gp);
    }
    else if(_phase == tlm::END_RESP){ // from initiator
	unsigned connection = getOrigin(_gp);
	unsigned TRANS = RESPONSE;
	unsigned queueMax = responseQueueSize;
	unsigned& busy = busyVector[connection][TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& pending = 
	    pendingVector[connection][TRANS][RoW];
	std::deque<tlm::tlm_generic_payload*>& queue = 
	    queueVector[connection][TRANS][RoW];
	// accept pending
	if(!pending.empty() && queue.size() < queueMax){
	    tlm::tlm_generic_payload& gp = *pending.front();
	    unsigned socketId = getDestination(gp);
	    sc_time delay = SC_ZERO_TIME;
	    tlm::tlm_phase phase = tlm::END_RESP;
	    iSocket[static_cast<int>(socketId)]->nb_transport_fw(gp, phase, delay);
	    // put in queue
	    pending.pop_front();
	    queue.push_back(&gp);
	}
	// send transaction
	if(!queue.empty()){
	    tlm::tlm_generic_payload& gp = *queue.front();
	    unsigned socketId = getOrigin(gp);
	    unsigned beats = getBeats(gp, TRANS);
	    sc_time delay = clk.nextPosedge(beats);
	    tlm::tlm_phase phase = tlm::BEGIN_RESP;
	    tSocket[static_cast<int>(socketId)]->nb_transport_bw(gp, phase, delay);
	    // remove from queue
	    queue.pop_front();
	}
	else
	    busy = FREE;
	// for measurement
	tlm::tlm_generic_payload& gp = _gp;
	processedData += gp.get_data_length();
	lastDeparture = sc_time_stamp();
	// clean up
	deleteOrigin(gp);
	deleteDestination(gp);
	gp.release();
    }
    else
	SC_REPORT_ERROR(name(), "unsupported phase");
}

unsigned ButterflySwitch::route(tlm::tlm_generic_payload& gp){
    uint64_t START_ADDRESS = addressRegions.front().first;
    uint64_t END_ADDRESS = addressRegions.back().second;
    uint64_t address = gp.get_address();
    int res = -1;
    for(unsigned i = 0; i < addressRegions.size(); i++){
	if(addressRegions[i].first <= address && addressRegions[i].second >= address)
	    res = i;
    }
    if(res == -1){
	std::string msg("address: ");
	msg += std::to_string(address) + " out of bounds [" 
	    + std::to_string(START_ADDRESS) + "," 
	    + std::to_string(END_ADDRESS) + "]";
	SC_REPORT_FATAL(name(), msg.c_str());
    }
    return static_cast<unsigned>(res);
}

unsigned ButterflySwitch::getBeats(tlm::tlm_generic_payload& gp, unsigned type){
    unsigned length = gp.get_data_length();
    unsigned cnt = length / buswidthInByte + 
	(length % buswidthInByte == 0 ? 0 : 1);
    if(type == REQUEST && gp.is_read())
	return 1;
    if(type == RESPONSE && gp.is_write())
	return 1;
    return cnt;
}
