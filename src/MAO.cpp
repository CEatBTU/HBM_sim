#include "MAO.hpp"

MAO_IF::MAO_IF(sc_module_name name)
    : sc_module(name), tSocket("tSocket"), iSocket("iSocket"), 
    innerTarget("s_tSocket"), innerInitiator("s_iSocket")
{
    tSocket.register_nb_transport_fw(this, &MAO_IF::nb_transport_fw);
    iSocket.register_nb_transport_bw(this, &MAO_IF::nb_transport_bw);
    innerTarget.register_nb_transport_fw(this, &MAO_IF::innerTarget_nb_transport_fw);
    innerInitiator.register_nb_transport_bw(this, &MAO_IF::innerInitiator_nb_transport_bw);
}
