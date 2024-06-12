#ifndef MEMORY_MANAGER_HPP
#define MEMORY_MANAGER_HPP

#include <deque>
#include <tlm>

class MemoryManager : public tlm::tlm_mm_interface{
    private:
	std::deque<tlm::tlm_generic_payload*> freePayloadPool;
	std::deque<tlm::tlm_generic_payload*> allocatedPayloadPool;
	int num_alloc = 0;

    public:
	MemoryManager(){}
	~MemoryManager(void) override;
	virtual void free(tlm::tlm_generic_payload* gp);	    
	virtual tlm::tlm_generic_payload* allocate();		    
};
#endif /* MEMORY_MANAGER_HPP */
