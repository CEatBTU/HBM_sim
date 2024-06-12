#include "cpu/Terminator.hpp"

Terminator::Terminator(sc_module_name name)
    :sc_module(name), countdown(0)
{
    //SC_METHOD(stop);
    //sensitive(terminateEvent);
    //dont_initialize();
    SC_THREAD(stop);
}

void Terminator::add()
{
    countdown++;
}

void Terminator::terminate()
{
    if(countdown == 0) // in case add() was never called
	terminateEvent.notify(5, SC_PS);
    countdown--;
    if(countdown == 0)
	terminateEvent.notify(5, SC_PS);
}

void Terminator::stop()
{
    wait(terminateEvent);

	std::cout << "Terminator stop()" << std::endl;
	sc_stop();
}
