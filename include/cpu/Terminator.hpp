#ifndef TERMINATOR_HPP
#define TERMINATOR_HPP

#include <systemc.h>

class Terminator : sc_module{
    public:	
	SC_HAS_PROCESS(Terminator);
	Terminator(sc_module_name name);
	~Terminator() = default;

	void add();
	void terminate();
    private:
	void stop();

    private:
	unsigned countdown;
	sc_event terminateEvent;
};
#endif /* TERMINATOR_HPP */
