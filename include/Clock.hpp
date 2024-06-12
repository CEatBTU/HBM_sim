#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <systemc.h>

class Clock : public sc_module{
    private:
	unsigned frequencyMHz;
	sc_time period;

    public:
	Clock(sc_module_name name, unsigned freqMHz);
	sc_time nextPosedge(unsigned cycles);
	sc_time nextPosedgeDelayed(sc_time delay, unsigned cycles);
	sc_time nextPosedgeAt(sc_time delay, unsigned cycles);
	
	sc_time getPeriod();
};
#endif // CLOCK_HPP
