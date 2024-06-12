#include "Clock.hpp"

Clock::Clock(sc_module_name name, unsigned freqMHz)
    : sc_module(name), frequencyMHz(freqMHz), 
    period(1000.0 / frequencyMHz, SC_NS)
{
    if(frequencyMHz == 0){
	SC_REPORT_FATAL(this->name(), "invalid frequency");
    }
}

sc_time Clock::nextPosedge(unsigned cycles){
    if(cycles == 0)
	return SC_ZERO_TIME;
    sc_time current = sc_time_stamp();
    unsigned long long current_val = current.value();
    unsigned long long period_val = period.value();
    unsigned long long remainder = (current_val / period_val + 1) * 
	period_val - current_val;
    unsigned long long res = remainder >= period_val ? period_val * cycles : remainder + period_val * (cycles - 1);
    //std::cerr << "Clk: " << res << std::endl;
    return sc_time::from_value(res);
}

sc_time Clock::nextPosedgeDelayed(sc_time delay, unsigned cycles)
{
    if(delay == SC_ZERO_TIME)
	return nextPosedge(cycles);
    sc_time current = sc_time_stamp() + delay;
    unsigned long long current_val = current.value();
    unsigned long long period_val = period.value();
    unsigned long long remainder = (current_val / period_val + 1) * 
	period_val - current_val;
    unsigned long long res = remainder >= period_val ? period_val * cycles : remainder + period_val * (cycles - 1);
    return sc_time::from_value(res);
}

sc_time Clock::nextPosedgeAt(sc_time current, unsigned cycles)
{
    if(cycles == 0)
	return SC_ZERO_TIME;
    unsigned long long current_val = current.value();
    unsigned long long period_val = period.value();
    unsigned long long remainder = (current_val / period_val + 1) * 
	period_val - current_val;
    unsigned long long res = remainder >= period_val ? period_val * cycles : remainder + period_val * (cycles - 1);
    return sc_time::from_value(res);
}

sc_time Clock::getPeriod()
{
    return period;
}
