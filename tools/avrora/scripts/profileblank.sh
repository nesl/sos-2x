
#!/bin/bash

if [ -z "${SOSROOT}" ]; then
	echo "SOSROOT environment variable is not defined"
	echo "Please add SOSROOT to environment variable and point to the top level"
	echo "SOS directory"
	exit 1
fi

java -Xms512m -Xmx512m avrora/Main \
	-simulation=sensor-network \
	-sections=.data,.text,.sos_bls \
	-random-seed=151079 \
	-update-node-id=true \
	-monitors=sleep,sos.monitor.SOSPacketMonitor,serial,real-time,sos.monitor.HeapPrintMonitor,sos.monitor.CallTimeMonitor \
	-ports=0:0:8314 \
	-topology="./grid5x5.top"   \
	-sensor-data="light":0:light.dat \
	-nodecount=25 \
	-update-node-id=true    \
	-colors=false \
	-seconds=500.0 \
	-methods=ker_slab_alloc,ker_slab_free,ker_gc_mark,sos_blk_mem_alloc,sos_blk_mem_free,ker_codemem_read,ker_fntable_subscribe,sos_blk_mem_change_own,sos_blk_mem_realloc,post_long,ker_msg_take_data,post_link,ker_sensor_get_data,ker_shm_open,ker_shm_update,ker_shm_close,ker_shm_get,ker_shm_wait,ker_shm_stopwait,ker_timer_start,ker_timer_restart,ker_timer_stop,malloc_gc_kernel,malloc_gc_module,malloc_gc,post_short,0x8c,0x90,0x94,0x98,0x9c,0xa0,0xa4,0xa8,0xac,0xb0,0xb4,0xb8,0xbc,0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0xd8,0xdc,0xe0,0xe4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc,0x100,0x104 \
${SOSROOT}/config/test_all/module_test_app.od

