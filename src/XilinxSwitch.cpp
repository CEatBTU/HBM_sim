#include "XilinxSwitch.hpp"

XilinxSwitch::XilinxSwitch(sc_module_name name, 
	unsigned bInByte, unsigned reqQs, unsigned respQs,
	unsigned freqMHz,
	std::vector<std::pair<uint64_t, uint64_t>> aR,
	unsigned vc, unsigned bc) 
    : Switch_IF(name, bInByte, reqQs, respQs, freqMHz, aR, vc), 
    bilateralConnections(bc)
{
    if(bilateralConnections == 0)
	SC_REPORT_FATAL(name, "invalid number of bilateral connections");

    nlohmann::json config;
    std::ifstream cf("../config/configuration.json", std::ios::in);
    if(!cf.is_open())
	SC_REPORT_FATAL(this->name(), "cannot open configuration file");
    cf >> config;
    cf.close();


    memorySizeInByte = config["memorySizeInByte"].get<uint64_t>();
    boundary_left = (memorySizeInByte / 8) * 3;
    boundary_right = (memorySizeInByte / 8) * 5 + 1;
    channelSize = memorySizeInByte / 32;
    
}

void XilinxSwitch::end_of_elaboration() 
{
    Switch_IF::end_of_elaboration();
    if(iSocket.size() != 2 * bilateralConnections + verticalConnections)
	SC_REPORT_FATAL(name(), "invalid number of buses");
    // init vectors
    busyVector.resize(iSocket.size(), 
	    std::vector<std::vector<unsigned>>
	    (2, std::vector<unsigned>(2, FREE)));
    prevVector.resize(iSocket.size(), 
	    std::vector<std::vector<unsigned>>
	    (2, std::vector<unsigned>(2, 0)));
    queueVector.resize(iSocket.size(), 
	    std::vector<std::vector<std::deque<tlm::tlm_generic_payload*>>>
	    (2, std::vector<std::deque<tlm::tlm_generic_payload*>>(2)));
    pendingVector.resize(iSocket.size(), 
	    std::vector<std::vector<std::deque<tlm::tlm_generic_payload*>>>
	    (2, std::vector<std::deque<tlm::tlm_generic_payload*>>(2)));
    addrVector.resize(1);
}

tlm::tlm_sync_enum XilinxSwitch::nb_transport_fw(int id, 
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

tlm::tlm_sync_enum XilinxSwitch::nb_transport_bw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    peq.notify(gp, phase, delay);
    return tlm::TLM_ACCEPTED;
}

void XilinxSwitch::peqCallback(tlm::tlm_generic_payload& _gp, 
	const tlm::tlm_phase& _phase)
{
    unsigned RoW = _gp.is_read(); 
    if(_phase == tlm::BEGIN_REQ){ // from initiator	
	unsigned connection = getDestination(_gp);
	unsigned TRANS = REQUEST;
	unsigned queueMax = requestQueueSize;
	unsigned& busy = busyVector[connection][TRANS][RoW];
	unsigned& prev = prevVector[connection][TRANS][RoW];
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
	    unsigned additionalDelay = 0;
	    unsigned origin = getOrigin(gp);

	    // destination is lateral, previous BM changed -> dead cycle
            //std::cout<<"origin: "<<origin<<"   destination: "<< socketId <<endl;
	    if(socketId < 2 * bilateralConnections){

               
               if (!boundary)                   
                     additionalDelay +=0;
                
                else {
                  // cout<<"\n extra delay BEGIN_REQ ";
                   if(gp.is_read())
                       additionalDelay += 5;
                   else
                       additionalDelay += 5;
                }   
                
               
             }

	    unsigned beats = getBeats(gp, TRANS) + additionalDelay;
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
        unsigned addr = _gp.get_address();
        unsigned TRANS = REQUEST;
        unsigned queueMax = requestQueueSize;
        unsigned& busy = busyVector[connection][TRANS][RoW];
        unsigned& prev = prevVector[connection][TRANS][RoW];
	unsigned& prev_address = addrVector[connection];
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
            unsigned additionalDelay = 0;
            unsigned origin = getOrigin(gp);
            // destination is lateral, previous BM changed -> dead cycle

            uint64_t diff = abs( static_cast<int> (addr-prev_address) );
            //cout<<"\ndiff: "<<diff;
            if( diff  > channelSize ) {
                   if(gp.is_read()){
                     //cout<<"\n extra delay random ";
                     additionalDelay +=20;
                   }
                   if(gp.is_write()){
                    // cout<<"\n extra delay random ";
                     additionalDelay +=2;
                   }

            }


            unsigned beats = getBeats(gp, TRANS) + additionalDelay;
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
        unsigned& prev = prevVector[connection][TRANS][RoW];
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


	    unsigned additionalDelay = 0;
	      if(socketId < 2 * bilateralConnections){

               
               if (!boundary)                   
                     additionalDelay +=0;
                
                else {
                   //cout<<"\n extra delay BEGIN_REsp ";
                   if(gp.is_read())
                       additionalDelay += 4;
                   else
                       additionalDelay += 4;
                }   
                
               
             }

	    unsigned beats = getBeats(gp, TRANS) + additionalDelay;
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
        unsigned& prev = prevVector[connection][TRANS][RoW];
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
	    unsigned additionalDelay = 0;
           if(socketId < 2 * bilateralConnections){

               
               if (!boundary)                   
                     additionalDelay +=0;
                
                else {
                   //cout<<"\n extra delay end_resp ";
                   if(gp.is_read())
                       additionalDelay += 5;
                   else
                       additionalDelay += 5;
                }   
                
               
             }
	    unsigned beats = getBeats(gp, TRANS) + additionalDelay;
	    //unsigned beats = getBeats(gp, TRANS);
	    sc_time delay = clk.nextPosedge(beats);
	    tlm::tlm_phase phase = tlm::BEGIN_RESP;
	    tSocket[static_cast<int>(socketId)]->nb_transport_bw(gp, phase, delay);
	    // remove from queue
            //cout<<"\nremove from queue";
	    queue.pop_front();
	}
        else
            busy = FREE;
        // for measurement
        tlm::tlm_generic_payload& gp = _gp;
        if(getOrigin(_gp) > 2 * bilateralConnections - 1){ // only vertical traffic
            //std::cout<<"\n name "<<name()<<" gp.get_data_length() "<<gp.get_data_length()<<" \n";
            processedData += gp.get_data_length();
            lastDeparture = sc_time_stamp();
            //std::cout<<"\n lastDeparture "<<lastDeparture;
        }
        else {
               crossedData += gp.get_data_length();
               //lastDeparture = sc_time_stamp();
        }
        //std::cout<<name()<<" "<<gp.get_address()<<", true Eresp, "<<sc_time_stamp()<<endl;
        // clean up
        deleteOrigin(gp);
        deleteDestination(gp);
        gp.release();
    }
    else
	SC_REPORT_ERROR(name(), "unsupported phase");
}

/* assuming left lateral connections are bound first 
 * then right lateral connections
 * and at last the vertical connections
 * */
unsigned XilinxSwitch::route(tlm::tlm_generic_payload& gp){
    uint64_t START_ADDRESS = addressRegions.front().first;
    uint64_t END_ADDRESS = addressRegions.back().second;
    uint64_t address = gp.get_address();
    //cout<<"\n route "<<"address: "<<address;
    //cout<<"\nSTART_ADDRESS: "<<START_ADDRESS;
    
    //cout<<"\nEND_ADDRESS: "<<END_ADDRESS;
   // uint64_t boundary_right =  
    if( (START_ADDRESS == boundary_left &&  address > START_ADDRESS)  || (START_ADDRESS == boundary_right && address < START_ADDRESS)  )
         boundary = true;
    else
         boundary = false;
    unsigned offset = 2 * bilateralConnections;
    unsigned origin = getOrigin(gp); 
    // gp recieved via vertical connection
    if(address < START_ADDRESS)
    {   
        //cout<<"\nvertical";
	return (origin < offset) ? origin : (origin - offset) / bilateralConnections;
    }
    if(address > END_ADDRESS) {
        //cout<<"\nlateral";
	return (origin < offset) ? origin : 
	    bilateralConnections + ((origin - offset) / bilateralConnections);
    }
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
    return offset + static_cast<unsigned>(res);
}

unsigned XilinxSwitch::getBeats(tlm::tlm_generic_payload& gp, unsigned type){
    unsigned length = gp.get_data_length();
    unsigned cnt = length / buswidthInByte + 
	(length % buswidthInByte == 0 ? 0 : 1);
    if(type == REQUEST && gp.is_read())
	return 1;
    if(type == RESPONSE && gp.is_write())
	return 1;
    return cnt;
}

////////////////////////////// XILINXENDSWITCH //////////////////////////////////////
XilinxEndSwitch::XilinxEndSwitch(sc_module_name name)
    : sc_module(name)
{
    tSocket.register_nb_transport_fw(this, &XilinxEndSwitch::nb_transport_fw);
}

tlm::tlm_sync_enum XilinxEndSwitch::nb_transport_fw(
	[[maybe_unused]] int id,
	[[maybe_unused]] tlm::tlm_generic_payload& gp,
	[[maybe_unused]] tlm::tlm_phase& phase,
	[[maybe_unused]] sc_time& delay)
{
    SC_REPORT_FATAL(name(), "transactions shouldn't end up in this module");
    return tlm::TLM_UPDATED;
}
