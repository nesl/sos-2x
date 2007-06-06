
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
	-monitors=sos.monitor.SOSPacketMonitor,serial,real-time,sos.monitor.HeapPrintMonitor,sos.monitor.CallTimeMonitor \
	-ports=0:0:8314 \
	-topology="./grid5x5.top"   \
	-sensor-data="light":0:light.dat \
	-nodecount=2 \
	-update-node-id=true    \
	-colors=false \
	-seconds=10.0 \
	-base-addr=415 \
	-methods=sos_blk_mem_alloc,sos_blk_mem_free,ker_codemem_read,ker_fntable_subscribe,sos_blk_mem_change_own,sos_blk_mem_realloc,post_long,ker_msg_take_data,ker_sys_post_value_do,post_link,ker_sensor_get_data,ker_shm_open,ker_shm_update,ker_shm_close,ker_shm_get,ker_shm_wait,ker_shm_stopwait,ker_timer_start,ker_timer_restart,ker_timer_stop,malloc_gc_kernel,malloc_gc_module \
${SOSROOT}/config/test_all/module_test_app.od

