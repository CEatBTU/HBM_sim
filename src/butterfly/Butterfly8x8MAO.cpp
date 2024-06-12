#include "butterfly/Butterfly8x8MAO.hpp"

Butterfly8x8MAO::Butterfly8x8MAO(sc_module_name name, std::string configFile)
    : MAO_IF(name)
{
    nlohmann::json config;
    std::ifstream cf(configFile, std::ios::in);
    if(!cf.is_open())
	SC_REPORT_FATAL(this->name(), "cannot open configuration file");
    cf >> config;
    cf.close();

    numberOfSwitches =
	config["numberOfSwitches"].get<uint64_t>();
    frequencyInMHz =
	config["frequencyInMHz"].get<uint64_t>();
    frequencyMC =
	config["frequencyMC"].get<uint64_t>();
    numberOfBilateralConnections =
	config["numberOfBilateralConnections"].get<uint64_t>();
    buswidthInByte =
	config["buswidthInByte"].get<uint64_t>();
    memorySizeInByte =
	config["memorySizeInByte"].get<uint64_t>();
    numberOfVerticalConnections =
	config["numberOfVerticalConnections"].get<uint64_t>();
    requestQueueSize =
	config["requestQueueSize"].get<uint64_t>();
    responseQueueSize =
	config["responseQueueSize"].get<uint64_t>();
    requestQueueSizeMC =
	config["requestQueueSizeMC"].get<uint64_t>();
    responseQueueSizeMC =
	config["responseQueueSizeMC"].get<uint64_t>();

    bool terminate = false;
    if(numberOfSwitches == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'numberOfSwitches' invalid");
	terminate = true;
    }
    if(frequencyInMHz == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'frequencyInMHz' invalid");
	terminate = true;
    }
    if(frequencyInMHz == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'frequencyInMHz' invalid");
	terminate = true;
    }
    if(numberOfBilateralConnections == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'numberOfBilateralConnections' invalid");
	terminate = true;
    }
    if(buswidthInByte == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'buswidthInByte' invalid");
	terminate = true;
    }
    if(numberOfVerticalConnections == 0 || 
	    (numberOfVerticalConnections & (numberOfVerticalConnections - 1)) != 0){
	SC_REPORT_ERROR(this->name(), "Config: 'numberOfVerticalConnections' invalid");
	terminate = true;
    }
    // must be power of 2
    if(memorySizeInByte == 0 || memorySizeInByte % numberOfSwitches != 0 ||
	    (memorySizeInByte / numberOfSwitches) % numberOfVerticalConnections != 0){
	SC_REPORT_ERROR(this->name(), "Config: 'memorySizeInByte' invalid");
	terminate = true;
    }
    if(requestQueueSize == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'requestQueueSize' invalid");
	terminate = true;
    }
    if(responseQueueSize == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'responseQueueSize' invalid");
	terminate = true;
    }
    if(requestQueueSizeMC == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'requestQueueSizeMC' invalid");
	terminate = true;
    }
    if(responseQueueSize == 0){
	SC_REPORT_ERROR(this->name(), "Config: 'responseQueueSizeMC' invalid");
	terminate = true;
    }
    if(numberOfSwitches * numberOfVerticalConnections % 8 != 0){
	SC_REPORT_ERROR(this->name(), "invalid configuration");
	terminate = true;
    }
    if(terminate)
	SC_REPORT_FATAL(this->name(), "invalid configuration!");
    build();
}

void Butterfly8x8MAO::build()
{
    // ############################## XILINX ############################
    uint64_t memPerSwitch = memorySizeInByte / numberOfSwitches;
    for(uint64_t i = 0; i < numberOfSwitches; i++){
	std::vector<std::pair<uint64_t, uint64_t>> addressRegions;
	uint64_t memPerChannel = memPerSwitch / numberOfVerticalConnections;
	for(uint64_t j = 0; j < numberOfVerticalConnections; j++){
	    uint64_t first = i * memPerSwitch + j * memPerChannel;
	    uint64_t second = i * memPerSwitch + (j + 1) * memPerChannel - 1;
	    addressRegions.push_back(std::pair<uint64_t, uint64_t>(first, second));
	}
	std::stringstream name;
	name << "XilinxSwitch_" << i;
	switches.emplace_back(new XilinxSwitch(name.str().c_str(), buswidthInByte,
		    requestQueueSize, responseQueueSize,
		    frequencyInMHz, 
		    addressRegions, 
		    numberOfVerticalConnections, numberOfBilateralConnections));
    }
    leftEnd = std::make_unique<XilinxEndSwitch>("leftEnd");
    rightEnd = std::make_unique<XilinxEndSwitch>("rightEnd");

    for(uint64_t i = 0; i < numberOfSwitches * numberOfVerticalConnections; i++){
	std::string name("MC_");
	name += std::to_string(i);
	MCs.emplace_back(std::make_unique<XilinxMemoryController>(
		    name.c_str(), buswidthInByte, 
		    requestQueueSizeMC, responseQueueSizeMC, 
		    frequencyInMHz,
		    frequencyMC));
    }
    ////////////////////// BINDING OF INTERNAL STRUCTURE ///////////////////////
    // bind left
    for(unsigned i = 0; i < numberOfBilateralConnections; i++){
	rightEnd->iSocket.bind(switches.back()->tSocket);
    }
    for(unsigned i = numberOfSwitches - 1; i > 0; i--){
	for(unsigned j = 0; j < numberOfBilateralConnections; j++){
	    switches[i]->iSocket.bind(switches[i - 1]->tSocket);
	}
    }
    for(unsigned i = 0; i < numberOfBilateralConnections; i++){
	switches.front()->iSocket.bind(leftEnd->tSocket);
    }
    // bind right
    for(unsigned i = 0; i < numberOfBilateralConnections; i++){
	leftEnd->iSocket.bind(switches.front()->tSocket);
    }
    for(unsigned i = 0; i < numberOfSwitches - 1; i++){
	for(unsigned j = 0; j < numberOfBilateralConnections; j++){
	    switches[i]->iSocket.bind(switches[i + 1]->tSocket);
	}
    }
    for(unsigned i = 0; i < numberOfBilateralConnections; i++){
	switches.back()->iSocket.bind(rightEnd->tSocket);
    }
    // ############################## BUTTERFLY ############################
    uint64_t BMs = numberOfSwitches * numberOfVerticalConnections;
    uint64_t rangeSwitch = memorySizeInByte / numberOfSwitches;
    std::vector<std::pair<uint64_t, uint64_t>> addresses;
    for(unsigned i = 0; i < numberOfSwitches; i++){
	addresses.emplace_back(std::pair<uint64_t, uint64_t>(
		    i * rangeSwitch, (i + 1) * rangeSwitch - 1));
	//std::cout << "[" << addresses.back().first << ", " << addresses.back().second << "]" << std::endl;
    }
    //std::cout << "@@@@@@@@@@@@@@@@@@@@@@@" << std::endl; 
    unsigned butterflyVertical = 8;
    for(unsigned i = 0; i < BMs / butterflyVertical; i++){
	std::vector<std::pair<uint64_t, uint64_t>> addressRegions = addresses;
	/*
	for(unsigned j = 0; j < numberOfSwitches; j++){
	    std::cout << "[" << addressRegions.back().first << ", " << addressRegions.back().second << "]" << std::endl;
	}
	std::cout << "#######################################" << std::endl;
	*/
	std::stringstream name;
	name << "ButterflySwitch_" << i;
	switchesB.emplace_back(new ButterflySwitch(name.str().c_str(),
		    buswidthInByte, 
		    requestQueueSize, responseQueueSize,
		    frequencyInMHz,
		    addressRegions,
		    butterflyVertical));
    }
    /////////////////////////// BIND MODULES /////////////////////
    for(unsigned i = 0; i < switchesB.size(); i++){
	for(unsigned j = 0; j < butterflyVertical; j++){
	    switchesB[i]->iSocket.bind(switches[j]->tSocket);
	}
    }
    ////////////////////// BINDING TO INTERFACE ///////////////////////
    for(unsigned i = 0; i < switchesB.size(); i++){
	for(unsigned j = 0; j < butterflyVertical; j++){
	    innerInitiator.bind(switchesB[i]->tSocket);
	}
    }
    // Xilinx
    for(uint64_t i = 0; i < numberOfSwitches; i++){
	for(uint64_t j = 0; j < numberOfVerticalConnections; j++){
	    switches[i]->iSocket.bind(MCs[i * numberOfVerticalConnections + j]->tSocket);
	    MCs[i * numberOfVerticalConnections + j]->iSocket.bind(innerTarget);
	}
    }
}

void Butterfly8x8MAO::end_of_elaboration()
{
    unsigned buses = numberOfSwitches * numberOfVerticalConnections;
    if(tSocket.size() != buses)
	SC_REPORT_FATAL(this->name(), "invalid binding!");
    if(iSocket.size() != buses)
	SC_REPORT_FATAL(this->name(), "invalid binding!");
}

void Butterfly8x8MAO::end_of_simulation()
{
    std::vector<std::unique_ptr<ButterflySwitch>>& switches = switchesB;
    uint64_t data = 0;
    for(unsigned i = 0; i < switches.size(); i++){
	data += switches[i]->getProcessedData();
    }
    std::cout << " data: " << data << std::endl;
    sc_time firstArrival = SC_ZERO_TIME;
    for(unsigned i = 0; i < switches.size(); i++){
	if(switches[i]->getFirstVisited()){
	    firstArrival = switches[i]->getFirstArrival();
	    break;
	}
    }
    for(unsigned i = 0; i < switches.size(); i++){
	if(switches[i]->getFirstArrival() < firstArrival)
	    firstArrival = switches[i]->getFirstArrival();
    }
    sc_time lastDeparture = switches[0]->getLastDeparture();
    for(unsigned i = 0; i < switches.size(); i++){
	if(switches[i]->getLastDeparture() > lastDeparture)
	    lastDeparture = switches[i]->getLastDeparture();
    }
    sc_time duration = lastDeparture - firstArrival;
    std::cout << "duration: " << duration << " start: " << firstArrival << " last: " << lastDeparture << std::endl;
    double GB_s = 0.0;
    if(data != 0 && duration != SC_ZERO_TIME)
	GB_s = ((double) data) / duration.value() * 1000.0;
    std::cout << "Butterfly Total Throughput: " << std::setprecision (2) << std::fixed << GB_s << " GB/s" << std::endl;
}

// from CPU to interconnect
tlm::tlm_sync_enum Butterfly8x8MAO::nb_transport_fw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    return innerInitiator[id]->nb_transport_fw(gp, phase, delay);
}

// from interconnect to CPU
tlm::tlm_sync_enum Butterfly8x8MAO::innerInitiator_nb_transport_bw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    return tSocket[id]->nb_transport_bw(gp, phase, delay);
}

// from MC to memory
tlm::tlm_sync_enum Butterfly8x8MAO::innerTarget_nb_transport_fw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    return iSocket[id]->nb_transport_fw(gp, phase, delay);
}

// from memory to MC
tlm::tlm_sync_enum Butterfly8x8MAO::nb_transport_bw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    return innerTarget[id]->nb_transport_bw(gp, phase, delay);
}
