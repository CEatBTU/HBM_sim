#include "butterfly/Butterfly2x2MAO.hpp"

Butterfly2x2MAO::Butterfly2x2MAO(sc_module_name name, std::string configFile)
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
    // must be power of 2
    if((numberOfSwitches * numberOfVerticalConnections) & 
	    (numberOfSwitches * numberOfVerticalConnections - 1) != 0){
	SC_REPORT_ERROR(this->name(), "invalid configuration");
	terminate = true;
    }
    if(terminate)
	SC_REPORT_FATAL(this->name(), "invalid configuration!");
    xilinx = std::make_unique<XilinxMAO>("XilinxMAO", configFile);
    build();
}

/* NOTE: only works if number of MBs and numberOfVerticalConnections
 * are multiple of 2 */
void Butterfly2x2MAO::build()
{
    /////////////////////////// INIT MODULES /////////////////////
    unsigned buses = numberOfSwitches * numberOfVerticalConnections; 
    uint64_t memPerSwitch = memorySizeInByte / numberOfSwitches;
    uint64_t memPerChannel = memPerSwitch / numberOfVerticalConnections;
    // excluding last level
    unsigned levels = static_cast<int>(log2(buses));
    switchesB.resize(levels);
    /////////////// BUTTERFLY 
    for(unsigned i = 0; i < levels - 1; i++){
	unsigned delimeter =  buses >> (i + 1);
	for(unsigned j = 0; j < buses / delimeter; j++){
	    for(unsigned k = j * delimeter; k < j * delimeter + delimeter; k++){
		std::vector<std::pair<uint64_t, uint64_t>> addressRegions;
		uint64_t localMax = (k + 1) * memPerChannel;
		addressRegions.push_back(std::pair<uint64_t, uint64_t>(
			    0, localMax - 1));
		addressRegions.push_back(std::pair<uint64_t, uint64_t>(
			    localMax, memorySizeInByte - 1));
		std::stringstream name;
		name << "ButterflySwitch_" << i << "_" << k;
		switchesB[i].emplace_back(new ButterflySwitch(name.str().c_str(),
			    buswidthInByte, 
			    requestQueueSize, responseQueueSize,
			    frequencyInMHz,
			    addressRegions,
			    2));
	    }
	}
    }
    // last level
    for(unsigned i = 0; i < buses; i++){
	std::vector<std::pair<uint64_t, uint64_t>> addressRegions;
	addressRegions.emplace_back(
		std::pair<uint64_t, uint64_t>(0, memorySizeInByte - 1));
	std::stringstream name;
	name << "ButterflySwitch_" << levels - 1 << "_" << i;
	switchesB.back().emplace_back(new ButterflySwitch(name.str().c_str(),
		    buswidthInByte, 
		    requestQueueSize, responseQueueSize,
		    frequencyInMHz,
		    addressRegions,
		    2));
    }
    /////////////////////////// BIND MODULES /////////////////////
    /////////////// BUTTERFLY 
    for(unsigned i = 0; i < levels - 1; i++){
	unsigned delimeter =  buses >> (i + 1);
	for(unsigned j = 0; j < buses / delimeter; j++){
	    for(unsigned k = j * delimeter; k < j * delimeter + delimeter; k++){
		if(j % 2 == 0){ // go right
		    switchesB[i][k]->iSocket.bind(
			    switchesB[i + 1][k]->tSocket);
		    switchesB[i][k]->iSocket.bind(
			    switchesB[i + 1][k + delimeter]->tSocket);
		}
		else{ // go left
		    switchesB[i][k]->iSocket.bind(
			    switchesB[i + 1][k - delimeter]->tSocket);
		    switchesB[i][k]->iSocket.bind(
			    switchesB[i + 1][k]->tSocket);
		}
	    }
	}
    } 
    /////////////// BUTTERFLY to Xilinx 
    for(unsigned i = 0; i < buses;i++){
	switchesB.back()[i]->iSocket.bind(xilinx->tSocket);
    }
    ////////////////////// BINDING TO INTERFACE ///////////////////////
    for(unsigned i = 0; i < buses; i++){
	innerInitiator.bind(switchesB.front()[i]->tSocket);
	xilinx->iSocket.bind(innerTarget);
    }
}

void Butterfly2x2MAO::end_of_elaboration()
{
    unsigned buses = numberOfSwitches * numberOfVerticalConnections;
    if(tSocket.size() != buses)
	SC_REPORT_FATAL(this->name(), "invalid binding!");
    if(iSocket.size() != buses)
	SC_REPORT_FATAL(this->name(), "invalid binding!");
}

void Butterfly2x2MAO::end_of_simulation()
{
    std::vector<std::unique_ptr<ButterflySwitch>>& switches =
	switchesB.front();
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
    std::cout << "Total Throughput: " << std::setprecision (2) << std::fixed << GB_s << " GB/s" << std::endl;
}

// from CPU to interconnect
tlm::tlm_sync_enum Butterfly2x2MAO::nb_transport_fw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    return innerInitiator[id]->nb_transport_fw(gp, phase, delay);
}

// from interconnect to CPU
tlm::tlm_sync_enum Butterfly2x2MAO::innerInitiator_nb_transport_bw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    return tSocket[id]->nb_transport_bw(gp, phase, delay);
}

// from MC to memory
tlm::tlm_sync_enum Butterfly2x2MAO::innerTarget_nb_transport_fw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    return iSocket[id]->nb_transport_fw(gp, phase, delay);
}

// from memory to MC
tlm::tlm_sync_enum Butterfly2x2MAO::nb_transport_bw(int id,
	tlm::tlm_generic_payload& gp, tlm::tlm_phase& phase, sc_time& delay)
{
    return innerTarget[id]->nb_transport_bw(gp, phase, delay);
}
