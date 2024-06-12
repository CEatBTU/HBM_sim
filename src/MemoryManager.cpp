#include "MemoryManager.hpp"

MemoryManager::~MemoryManager()
{
    if(num_alloc != 0){
	std::string msg = std::to_string(num_alloc);
	msg += " more calls of allocate()";
	std::cerr << msg << std::endl;
	//SC_REPORT_ERROR("MemoryManager", msg.c_str());
    }

    for(unsigned i = 0; i <  allocatedPayloadPool.size(); i++){
	delete allocatedPayloadPool[i];
    }
}

void MemoryManager::free(tlm::tlm_generic_payload* gp)
{
    freePayloadPool.push_back(gp);
    num_alloc--;
}

tlm::tlm_generic_payload* MemoryManager::allocate()
{
    tlm::tlm_generic_payload* gp = nullptr;
    if(freePayloadPool.size() == 0){ // no free elements in the pool
	gp = new tlm::tlm_generic_payload();
	gp->set_mm(this);
	allocatedPayloadPool.push_back(gp);
    }
    else{ // release a free element from the pool
	gp = freePayloadPool.front();
	freePayloadPool.pop_front();
    }
    num_alloc++;
    return gp;
}
