#ifndef BUTTERFLY_2x2_MAO_HPP
#define BUTTERFLY_2x2_MAO_HPP

#include <vector>
#include <memory> // std::unique_ptr
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip> // std::setprecision
#include <math.h>  // log2   

#include "json.hpp"

#include "MAO.hpp"
#include "ButterflySwitch.hpp"
#include "XilinxMAO.hpp"

class Butterfly2x2MAO : public MAO_IF{
    public:
	SC_HAS_PROCESS(Butterfly2x2MAO);
	Butterfly2x2MAO(sc_module_name name, std::string configFile);

    private:
	void end_of_elaboration() override;
	void end_of_simulation() override;
	tlm::tlm_sync_enum nb_transport_fw(int id, tlm::tlm_generic_payload& gp,
	       	tlm::tlm_phase& phase, sc_time& delay) override;
	tlm::tlm_sync_enum nb_transport_bw(int id,tlm::tlm_generic_payload& gp,
		tlm::tlm_phase& phase, sc_time& delay) override;
	tlm::tlm_sync_enum innerTarget_nb_transport_fw(int id, 
		tlm::tlm_generic_payload& gp, 
		tlm::tlm_phase& phase, sc_time& delay) override;
	tlm::tlm_sync_enum innerInitiator_nb_transport_bw(int id, 
		tlm::tlm_generic_payload& gp, 
		tlm::tlm_phase& phase, sc_time& delay) override;

	void build(); // build the structure
    private:
	std::vector<std::vector<std::unique_ptr<ButterflySwitch>>> switchesB;
	std::unique_ptr<XilinxMAO> xilinx;
    public:
	uint64_t numberOfSwitches,
		 frequencyInMHz,
		 frequencyMC,
		 numberOfBilateralConnections,
		 buswidthInByte,
		 memorySizeInByte,
		 numberOfVerticalConnections,
		 requestQueueSize,
		 responseQueueSize,
		 requestQueueSizeMC,
		 responseQueueSizeMC;
};
#endif // BUTTERFLY_2x2_MAO_HPP
