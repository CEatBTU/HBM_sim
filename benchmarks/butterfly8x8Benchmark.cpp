#include <string>
#include <vector>
#include <memory> // std::unique_ptr
#include <sstream>

#include <systemc.h>

//#define MY_RECORDABLE_BENCHMARK
#ifdef MY_RECORDABLE_BENCHMARK
#include <simulation/DRAMSysRecordable.h>
#else
#include <simulation/DRAMSys.h>
#endif

#include <LengthConverter.h>

#include "butterfly/Butterfly8x8MAO.hpp"
#include "cpu/StrideCPU.hpp"

#include "cpu/RW_StrideCPU.hpp"
#include "cpu/RandomCPU.hpp"
#include "cpu/RW_RandomCPU.hpp"

int sc_main(int argc, char** argv)
{
    if(argc !=  6){
	std::cerr << "Invalid number of arguments." << std::endl;
	std::cerr << "Usage: [benchmark] [transactionType] [burstlength] [transactionsPerCPU] [chunkSize]" << std::endl;
	exit(EXIT_FAILURE);
    }
    int bursts = 0, transactionsPerCPU = 0, chunkSize = 0;
    std::string benchmark, transactionType;
    // read input
    benchmark = std::string(argv[1]);
    transactionType = std::string(argv[2]);
    bursts = std::stoi(argv[3]);
    transactionsPerCPU = std::stoi(argv[4]);
    chunkSize = std::stoi(argv[5]);
    // check input
    if(bursts <= 0 || transactionsPerCPU <= 0 || chunkSize <= 0){
	std::cerr << "Invalid 'bursts', 'transactionsPerCPU' or 'chunkSize'." << std::endl;
	exit(EXIT_FAILURE);
    }
    //////////////////////////// INITIATE MODULES ///////////////////////////////
    bool base_protocol_check = true, useLengthConverter = true;
    base_protocol_check = false;
    //useLengthConverter = false;
    // ------------------- XILINX_MAO ---------------------
    std::unique_ptr<Butterfly8x8MAO> MAO = std::make_unique<Butterfly8x8MAO>("Butterfly8x8MAO",
	    "/home/khamo/hbm-accelerator/config/configuration.json");
    unsigned BMs = MAO->numberOfSwitches * MAO->numberOfVerticalConnections;

    // ------------------- CPU ---------------------
    std::unique_ptr<Terminator> term = std::make_unique<Terminator>("Terminator");
    std::vector<std::unique_ptr<CPU_IF>> CPUs;
    uint64_t START_ADDRESS = 0, STRIDE = 0, BURST_LENGTH = 0, WIDTH = 0, REP = 0,
	     CHUNK_SIZE = chunkSize;
    unsigned CPUfreqInMHz = MAO->frequencyInMHz;
    unsigned CPUbuswidthInByte = MAO->buswidthInByte;
    WIDTH = MAO->memorySizeInByte / BMs;
    BURST_LENGTH = CPUbuswidthInByte * bursts;
    STRIDE = BURST_LENGTH;
    for(unsigned i = 0; i < BMs; i++){
	const char* name = (std::string("CPU_") + std::to_string(i)).c_str();
	if(benchmark == "SCS"){
	    REP = i > 0 ? 0 : transactionsPerCPU;
	    //REP = transactionsPerCPU;
	    START_ADDRESS = i * WIDTH;
	    if(transactionType == "R"){
		CPUs.emplace_back(
			std::make_unique<StrideCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    true));
	    }
	    else if(transactionType == "W"){
		CPUs.emplace_back(
			std::make_unique<StrideCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    false));
	    }
	    else if(transactionType == "RW"){
		CPUs.emplace_back(
			std::make_unique<RW_StrideCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    CHUNK_SIZE, i % 2 == 0));
	    }
	    else{
		std::cerr << "Invalid 'transactionType'" << std::endl;
		exit(EXIT_FAILURE);
	    }
	}
	else if(benchmark == "SCRA"){
	    REP = i > 0 ? 0 : transactionsPerCPU;
	    //REP = transactionsPerCPU;
	    START_ADDRESS = i * WIDTH;
	    if(transactionType == "R"){
		CPUs.emplace_back(
			std::make_unique<RandomCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    true));
	    }
	    else if(transactionType == "W"){
		CPUs.emplace_back(
			std::make_unique<RandomCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    false));
	    }
	    else if(transactionType == "RW"){
		CPUs.emplace_back(
			std::make_unique<RW_RandomCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    CHUNK_SIZE, i % 2 == 0));
	    }
	    else{
		std::cerr << "Invalid 'transactionType'" << std::endl;
		exit(EXIT_FAILURE);
	    }
	}
	else if(benchmark == "CCS"){
	    REP = transactionsPerCPU;
	    START_ADDRESS = i * STRIDE;
	    if(transactionType == "R"){
		CPUs.emplace_back(
			std::make_unique<StrideCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    true));
	    }
	    else if(transactionType == "W"){
		CPUs.emplace_back(
			std::make_unique<StrideCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    false));
	    }
	    else if(transactionType == "RW"){
		CPUs.emplace_back(
			std::make_unique<RW_StrideCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    CHUNK_SIZE, i % 2 == 0));
	    }
	    else{
		std::cerr << "Invalid 'transactionType'" << std::endl;
		exit(EXIT_FAILURE);
	    }
	}
	else if(benchmark == "CCRA"){
	    REP = transactionsPerCPU;
	    START_ADDRESS = 0;
	    WIDTH = MAO->memorySizeInByte;
	    if(transactionType == "R"){
		CPUs.emplace_back(
			std::make_unique<RandomCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    true));
	    }
	    else if(transactionType == "W"){
		CPUs.emplace_back(
			std::make_unique<RandomCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    false));
	    }
	    else if(transactionType == "RW"){
		CPUs.emplace_back(
			std::make_unique<RW_RandomCPU>(
			    name, CPUbuswidthInByte, CPUfreqInMHz, *term,
			    START_ADDRESS, STRIDE, BURST_LENGTH, WIDTH, REP,
			    CHUNK_SIZE, i % 2 == 0));
	    }
	    else{
		std::cerr << "Invalid 'transactionType'" << std::endl;
		exit(EXIT_FAILURE);
	    }
	}
	else{
	    std::cerr << "Invalid 'benchmark'." << std::endl;
	    exit(EXIT_FAILURE);
	}
    }
    // ------------------- DRAM_SYS ---------------------
    std::vector<std::unique_ptr<DRAMSys>> DRAMs;
    unsigned numDRAMSys = BMs / 2; // one DRAMSys gets two input channels
    std::string resources = "/home/khamo/hbm-accelerator/config/DRAMSys/";
    for(unsigned i = 0; i < numDRAMSys; i++){
	std::string simulationJson  = resources + "simulations/hbm2-example_"
	    + std::to_string(i) + ".json";
	DRAMSysConfiguration::Configuration configLib =
	    DRAMSysConfiguration::from_path(simulationJson, resources);

	const char* name = (std::string("DRAMs_") + std::to_string(i * 2)
		+ "-" + std::to_string(i * 2 + 1)).c_str();
#ifdef MY_RECORDABLE_BENCHMARK
	DRAMs.emplace_back(std::make_unique<DRAMSysRecordable>(name, configLib));
#else
	DRAMs.emplace_back(std::make_unique<DRAMSys>(name, configLib));
#endif
    }
    // ------------------- LENGTH CONVERTER ---------------------
    std::vector<std::unique_ptr<LengthConverter>> lengthConverters;
    if(useLengthConverter){
	for(unsigned i = 0; i < BMs; i++){
	    std::string name = std::string("LengthConverter_") + std::to_string(i);
	    lengthConverters.emplace_back(std::make_unique<LengthConverter>(
			name.c_str(),
			DRAMs[i / 2]->getConfig().memSpec->maxBytesPerBurst,
			DRAMs[i / 2]->getConfig().storeMode !=
			Configuration::StoreMode::NoStorage));
	}
    }
    // ------------------- TLM2_CHECKER ---------------------
    std::vector<std::unique_ptr<tlm_utils::tlm2_base_protocol_checker<>>>
	checkersTop, checkersBottom;
    if(base_protocol_check){
	for(unsigned i = 0; i < BMs; i++){
	    std::string name = std::string("checkerTop_") + std::to_string(i);
	    checkersTop.emplace_back(std::make_unique<
		    tlm_utils::tlm2_base_protocol_checker<>>(name.c_str()));
	}
	for(unsigned i = 0; i < BMs; i++){
	    std::string name = std::string("checkerBottom_") + std::to_string(i);
	    checkersBottom.emplace_back(std::make_unique<
		    tlm_utils::tlm2_base_protocol_checker<>>(name.c_str()));
	}
    }
    //////////////////////////// BIND MODULES ///////////////////////////////
    if(base_protocol_check){
	if(useLengthConverter){
	    for(unsigned i = 0; i < BMs; i++){
		CPUs[i]->iSocket.bind(checkersTop[i]->target_socket);
		checkersTop[i]->initiator_socket.bind(MAO->tSocket);
		MAO->iSocket.bind(checkersBottom[i]->target_socket);
		checkersBottom[i]->initiator_socket.bind(
			lengthConverters[i]->tSocket);
	    }
	    for(unsigned i = 0; i < numDRAMSys; i++){
		lengthConverters[i * 2]->iSocket.bind(DRAMs[i]->tSocket);
		lengthConverters[i * 2 + 1]->iSocket.bind(DRAMs[i]->tSocket);
	    }
	}
	else{
	    for(unsigned i = 0; i < BMs; i++){
		CPUs[i]->iSocket.bind(checkersTop[i]->target_socket);
		checkersTop[i]->initiator_socket.bind(MAO->tSocket);
		MAO->iSocket.bind(checkersBottom[i]->target_socket);
	    }
	    for(unsigned i = 0; i < numDRAMSys; i++){
		checkersBottom[i*2]->initiator_socket.bind(DRAMs[i]->tSocket);
		checkersBottom[i*2+1]->initiator_socket.bind(DRAMs[i]->tSocket);
	    }

	}
    }
    else{
	if(useLengthConverter){
	    for(unsigned i = 0; i < BMs; i++){
		CPUs[i]->iSocket.bind(MAO->tSocket);
		MAO->iSocket.bind(lengthConverters[i]->tSocket);
	    }
	    for(unsigned i = 0; i < numDRAMSys; i++){
		lengthConverters[i * 2]->iSocket.bind(DRAMs[i]->tSocket);
		lengthConverters[i * 2 + 1]->iSocket.bind(DRAMs[i]->tSocket);
	    }
	}
	else{
		for(unsigned i = 0; i < BMs; i++){
	    	CPUs[i]->iSocket.bind(MAO->tSocket);
		}
		for(unsigned i = 0; i < DRAMs.size(); i++){
	    	MAO->iSocket.bind(DRAMs[i]->tSocket);
	    	MAO->iSocket.bind(DRAMs[i]->tSocket);
		}
    	}
    }
    //////////////////////////// SIMULATION ///////////////////////////////
    std::cout << "-------------SIMULATION START-------------" << std::endl;
    sc_start();
    std::cout << "-------------SIMULATION END at " << sc_time_stamp() << "-------------" << std::endl;
    exit(EXIT_SUCCESS);
}
