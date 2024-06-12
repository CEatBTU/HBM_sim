#include "Switch.hpp"

Switch_IF::Switch_IF(sc_module_name name, 
	unsigned bInByte, unsigned reqQs, unsigned respQs,
	unsigned freqMHz,
	std::vector<std::pair<uint64_t, uint64_t>> aR, 
	unsigned vc) 
    : sc_module(name), tSocket("tSocket"), iSocket("iSocket"),
    peq(this, &Switch_IF::peqCallback),
    buswidthInByte(bInByte), requestQueueSize(reqQs), responseQueueSize(respQs),
    clk("clk", freqMHz),
    addressRegions(aR),
    verticalConnections(vc)
{
    if(buswidthInByte == 0){
	SC_REPORT_FATAL(this->name(), "'buswidthInByte' invalid");
    }
    tSocket.register_nb_transport_fw(this, &Switch_IF::nb_transport_fw);
    iSocket.register_nb_transport_bw(this, &Switch_IF::nb_transport_bw);
}

void Switch_IF::end_of_elaboration()
{
    if(iSocket.size() != tSocket.size()){
	std::cerr << "MASTERS: " << tSocket.size() << " SLAVES: " << iSocket.size() << std::endl;
	SC_REPORT_FATAL(name(), "number of master and slave buses don't match");
    }
    if(addressRegions.empty() || addressRegions.size() != verticalConnections){
	SC_REPORT_FATAL(name(), "invalid address regions");
    }
    // check if addresses sorted chronologically
    for(unsigned i = 1; i <= addressRegions.size() - 1; i++){
    	if(addressRegions[i - 1].first >= addressRegions[i].first){
	    SC_REPORT_FATAL(name(), "invalid address region");
	}
    }
    for(auto e : addressRegions){
	if(e.first > e.second)
	    SC_REPORT_FATAL(name(), "invalid address region");
    }
}
////////////////////////// ORIGIN ////////////////////////////
void Switch_IF::setOrigin(tlm::tlm_generic_payload& gp, unsigned BM)
{
    originMap.emplace(&gp, BM);
}

unsigned Switch_IF::getOrigin(tlm::tlm_generic_payload& gp)
{
    unsigned res = 0;
    try{
	res = originMap.at(&gp);
    }
    catch(const std::out_of_range& e){
	SC_REPORT_FATAL(name(), e.what());
    }
    return res;
}

void Switch_IF::deleteOrigin(tlm::tlm_generic_payload& gp)
{
    originMap.erase(&gp);
}
////////////////////////// DESTINATION ////////////////////////////
void Switch_IF::setDestination(tlm::tlm_generic_payload& gp, unsigned BM)
{
    destinationMap.emplace(&gp, BM);
}

unsigned Switch_IF::getDestination(tlm::tlm_generic_payload& gp)
{
    unsigned res = 0;
    try{
	res = destinationMap.at(&gp);
    }
    catch(const std::out_of_range& e){
	SC_REPORT_FATAL(name(), e.what());
    }
    return res;
}

void Switch_IF::deleteDestination(tlm::tlm_generic_payload& gp)
{
    destinationMap.erase(&gp);
}
////////////////////////// for MEASUREMENTS ////////////////////////////
sc_time Switch_IF::getFirstArrival(){
	return firstArrival;
}

sc_time Switch_IF::getLastDeparture(){
	return lastDeparture;
}

uint64_t Switch_IF::getProcessedData(){
	return processedData;
}

uint64_t Switch_IF::getCrossedData(){
	return crossedData;
}

bool Switch_IF::getFirstVisited(){
	return firstVisited;
}
