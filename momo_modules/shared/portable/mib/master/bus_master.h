#ifndef __bus_master_h__
#define __bus_master_h__

#include "bus.h"

//Master Routines
void 			bus_master_callback();
void 			bus_master_compose_params(uint8 spec);
#ifndef _PIC12
void 			bus_master_rpc_async(mib_rpc_function callback, unsigned char address, unsigned char feature, unsigned char cmd);
#else
uint8			bus_master_rpc_sync(unsigned char address, unsigned char feature, unsigned char cmd);
#endif

#endif
