/**
 * @file surge_dump.c
 * @breif dumps surge messages into a file. A catcher for sos_dump.exe.
 * @author Peter Mawhorter (pmawhorter@cs.hmc.edu)
 */

#include <stdio.h>

#include "../sos_catch.h"

#include <message_types.h>

// Include file for new sensor data strcuture
#include <sensor_system.h>

// Where to connect to the sos server:
const char *ADDRESS="127.0.0.1";
const char *PORT="7915";
const char* FILENAME = "new_sensor.dat";

// Configuration parameters set by the user
#define TIME_CLOCK_SOURCE	32768L
#define SAMPLE_RATE			250

#define TIME_CONV_MS			(TIME_CLOCK_SOURCE/(float)1000.0)
#define INTER_SAMPLE_TIME_MS	((float)1000.0/SAMPLE_RATE)
//#define INTER_SAMPLE_ADJUST_MS	((float)1.0/(float)32.0)

/*
 * Conversion function to display actual sensor reading.
 */
float convert(int adc, sensor_id_t id) {
	float ret;
	switch (id) {
		case ACCEL_X_SENSOR: {
			ret = (adc - 2362L) * 0.0111;
			break;
		}
		case ACCEL_Y_SENSOR: {
			ret = (adc - 2362L) * 0.0111;
			break;
		}
		default: {
			ret = 0;
		}
	}

	return ret;
}

/*
 * Catches surge messages and dumps them into surge.dat.
 */
int catch(Message* msg) {
	int i; // Loop variable.
	FILE *fout; // The file we'll be writing to.
	float timestamp;

	// We only care about messages containing sensor data.
	// Currently, it is hardcoded to 0x81 in test modules.
	if (msg->type == 0x81) {
		sensor_data_msg_t *sensordata = (sensor_data_msg_t *)msg->data;
		sensordata->timestamp = ehtonl(sensordata->timestamp);
		sensordata->num_samples = ehtons(sensordata->num_samples);
		// Initialize the timestamp
		timestamp = (float)sensordata->timestamp / TIME_CONV_MS;

		printf("Sensor Data ::\n");
		printf("Header -\n");
		printf("Status = %d\n", sensordata->status);
		printf("Sensor = %d\n", sensordata->sensor);
		printf("Buffer timestamp = %d %f\n", sensordata->timestamp, timestamp);
		printf("Number of samples = %d\n", sensordata->num_samples);
		printf("Data -\n");
		printf("Timestamp\tADC\tConverted\n");

		// Open the file:
		fout = fopen(FILENAME,"a+");

		for (i = 0; i < sensordata->num_samples; i++) {
			uint16_t raw = ehtons(sensordata->buf[i]);
			float value = convert(raw, sensordata->sensor);
			printf("%f\t%d\t%f\n", timestamp, raw, value);
			// Write (timestamp, ADC value, converted value) to file
			fprintf(fout, "%f,%d,%f\n", timestamp, raw, value);
			// Increment the timestamp for each sample
			// by (1000/SAMPLE_RATE) ms
			timestamp += INTER_SAMPLE_TIME_MS;
			//timestamp += ((float)1.0/32.0);
		}

		fclose(fout);
	}
}

// Subscribe the catch function and let it do it's thing.
int main(int argc, char **argv)
{
  int ret; // Did subscription succeed?

  ret = sos_subscribe(ADDRESS, PORT, (recv_msg_func_t)catch);

  if (ret) {
    return ret;
  }

  // Let things run:
  while (1){
    sleep(1);
  }

  return 0;
}
