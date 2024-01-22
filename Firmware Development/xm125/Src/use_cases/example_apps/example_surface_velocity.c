// Copyright (c) Acconeer AB, 2023
// All rights reserved
// This file is subject to the terms and conditions defined in the file
// 'LICENSES/license_acconeer.txt', (BSD 3-Clause License) which is part
// of this source code package.

/** \example example_processing_fft_speed.c
 * @brief example_processing_fft_speed.c
 * Example program that shows how to calculate speed by making an fft of the
 * IQ data values sampled at a signgle distance point.
 */

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "i2c_slave.h"
#include "acc_config.h"
#include "acc_hal_definitions_a121.h"
#include "acc_hal_integration_a121.h"
#include "acc_integration_log.h"
#include "acc_rss_a121.h"
#include "acc_version.h"

#include "acc_processing_helpers.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "acc_control_helper.h"

#include "acc_definitions_a121.h"
#include "acc_detector_distance.h"
#include "acc_hal_definitions_a121.h"
#include "acc_hal_integration_a121.h"
#include "acc_integration.h"
#include "acc_integration_log.h"
#include "acc_rss_a121.h"
#include "acc_sensor.h"
#include "acc_version.h"
#define SENSOR_ID (1U)

typedef enum
{
	DISTANCE_PRESET_CONFIG_NONE = 0,
	DISTANCE_PRESET_CONFIG_BALANCED,
	DISTANCE_PRESET_CONFIG_HIGH_ACCURACY,
} distance_preset_config_t;


#define SENSOR_ID (1U)
// 2 seconds should be enough even for long ranges and high signal quality
#define SENSOR_TIMEOUT_MS (2000U)

typedef struct
{
	acc_sensor_t                   *sensor;
	acc_detector_distance_config_t *config;
	acc_detector_distance_handle_t *handle;
	void                           *buffer;
	uint32_t                       buffer_size;
	void                           *calibration_buffer;
	uint32_t                       calibration_buffer_size;
} distance_detector_resources_t;


static void cleanup(distance_detector_resources_t *resources);


static bool do_sensor_calibration(acc_sensor_t     *sensor,
                                  acc_cal_result_t *sensor_cal_result,
                                  void             *buffer,
                                  uint32_t         buffer_size);


static void set_config(acc_detector_distance_config_t *detector_config, distance_preset_config_t preset);


static bool initialize_detector_resources(distance_detector_resources_t *resources);


static bool do_full_detector_calibration(distance_detector_resources_t *resources,
                                         const acc_cal_result_t        *sensor_cal_result);


static bool do_detector_recalibration(distance_detector_resources_t *resources,
                                      const acc_cal_result_t        *sensor_cal_result);


static bool do_detector_get_next(distance_detector_resources_t  *resources,
                                 const acc_cal_result_t         *sensor_cal_result,
                                 acc_detector_distance_result_t *result);


static void print_distance_result(const acc_detector_distance_result_t *result);
#define HOURS_TO_SEC (float)(3600)
#define KM_TO_M   (float)(1000)

#define SWEEPS_PER_FRAME 512
#define PI (3.14159265358979323846f)
extern I2C_HandleTypeDef hi2c2;


static void update_configuration(acc_config_t *config);

static void acc_vector_iq_fft_inline(acc_vector_iq_t *vector_iq);

static void fft_inline(complex float *z, uint32_t stride, uint32_t n);

static void acc_vector_iq_init_hanning_window(acc_vector_iq_t *vector_iq);

static void acc_vector_iq_subtract_mean_inline(acc_vector_iq_t *vector_iq);

int acconeer_main(int argc, char *argv[]);


struct rxData
{
	float speed;
	float magnitude;
	int32_t speedIndex;
};

extern I2C_HandleTypeDef hi2c2;
volatile float speed;
volatile int32_t max_magnitude_index;
volatile acc_detector_distance_result_t result;
int acconeer_main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	acc_control_helper_t control_helper_state = {0};
	distance_detector_resources_t resources = { 0 };

	printf("Acconeer software version %s\n", acc_version_get());

	const acc_hal_a121_t *hal = acc_hal_rss_integration_get_implementation();

	if (!acc_rss_hal_register(hal))
	{
		return EXIT_FAILURE;
	}

	bool res = acc_control_helper_create(&control_helper_state, SENSOR_ID);

	if (!res)
	{
		printf("acc_control_helper_create() failed\n");
		return EXIT_FAILURE;
	}

	update_configuration(control_helper_state.config);

	res = acc_control_helper_activate(&control_helper_state);

	if (!res)
	{
		printf("acc_control_helper_activate() failed\n");
		acc_control_helper_destroy(&control_helper_state);
		// TODO: Deallocate any allocated memory before exit
		return EXIT_FAILURE;
	}

	uint32_t point_vector_length = SWEEPS_PER_FRAME;

	acc_vector_iq_t *point_vector = acc_vector_iq_alloc(point_vector_length);
	acc_vector_iq_t *fft_vector = acc_vector_iq_alloc(point_vector_length);
	acc_vector_iq_t *hanning_window = acc_vector_iq_alloc(point_vector_length);
	acc_vector_float_t *fft_magnitudes = acc_vector_float_alloc(point_vector_length);
	acc_vector_uint32_t *mHannWindow = acc_vector_uint32_alloc(point_vector_length);

	acc_vector_iq_t *fFT = acc_vector_iq_alloc(point_vector_length);
	acc_vector_iq_t *dataBuffer = acc_vector_iq_alloc(point_vector_length);
//	acc_vector_float_t *windowMagnitude = acc_vector_float_alloc(point_vector_length);


	bool mem_ok = (point_vector != NULL) && (fft_vector != NULL) && (hanning_window != NULL);
	if (!mem_ok)
	{
		printf("Memory allocation for vectors failed\n");
		goto clean_up;
	}

	acc_vector_iq_init_hanning_window(hanning_window);
	acc_algorithm_hann(point_vector_length, mHannWindow->data); // function surface velocity ki example se use kiya hai window nikaalne k liye

	uint32_t sweep_rate = control_helper_state.proc_meta.max_sweep_rate;

	float speed_step_kmh = sweep_rate / 2.0f * 0.0025f * 3.6f / (SWEEPS_PER_FRAME / 2);
	float min_speed_kmh = -(SWEEPS_PER_FRAME / 2) * speed_step_kmh;
	float max_speed_kmh = (SWEEPS_PER_FRAME / 2 - 1) * speed_step_kmh;

	printf("Sweep rate: %" PRIu32 "\n", sweep_rate);
	printf("Speed range: %" PRIfloat " km/h ", ACC_LOG_FLOAT_TO_INTEGER(min_speed_kmh));
	printf("to %" PRIfloat " km/h.\n", ACC_LOG_FLOAT_TO_INTEGER(max_speed_kmh));
	printf("Speed resolution: %" PRIfloat " km/h\n", ACC_LOG_FLOAT_TO_INTEGER(speed_step_kmh));

	if (!control_helper_state.proc_meta.high_speed_mode)
	{
		printf("Not using high speed mode. Sweep rate and max speed are limited.");
	}
	resources.config = acc_detector_distance_config_create();
	if (resources.config == NULL)
	{
		printf("acc_detector_distance_config_create() failed\n");
		cleanup(&resources);
		return EXIT_FAILURE;
	}
//
	set_config(resources.config, DISTANCE_PRESET_CONFIG_BALANCED);
//
	if (!initialize_detector_resources(&resources))
	{
		printf("Initializing detector resources failed\n");
		cleanup(&resources);
		return EXIT_FAILURE;
	}


//
//	// Print the configuration
//	acc_detector_distance_config_log(resources.handle, resources.config);
//
	/* Turn the sensor on */
	acc_hal_integration_sensor_supply_on(SENSOR_ID);
	acc_hal_integration_sensor_enable(SENSOR_ID);

	bool handling = false;
	bool handlingDistancePart = false;
    bool handlingSpeedPart = false;
    acc_cal_result_t sensor_cal_result;
//	resources.sensor = acc_sensor_create(SENSOR_ID);
//	if (resources.sensor == NULL)
//	{
//		printf("acc_sensor_create() failed\n");
//		cleanup(&resources);
//		return EXIT_FAILURE;
//	}

	uint8_t counter = 0;

	while(true)

	{
		if((0 == (counter % 2)))
		{

		acc_hal_integration_sensor_disable(SENSOR_ID);
		acc_hal_integration_sensor_enable(SENSOR_ID);
		resources.sensor = acc_sensor_create(SENSOR_ID);

		if (resources.sensor == NULL)
		{
			printf("acc_sensor_create() failed\n");
			cleanup(&resources);
			return EXIT_FAILURE;
		}

		   if( handlingDistancePart == false)
	       {

				if (!do_sensor_calibration(resources.sensor, &sensor_cal_result, resources.buffer, resources.buffer_size))
				{
					printf("Sensor calibration failed\n");
					cleanup(&resources);
					return EXIT_FAILURE;
				}
			//
				if (!do_full_detector_calibration(&resources, &sensor_cal_result))
				{
					printf("Detector calibration failed\n");
					cleanup(&resources);
					return EXIT_FAILURE;
				}
//				handlingDistancePart = true;
	       }
		//	uint32_t iterations = 25;
		//	for (uint32_t i = 0U; i < iterations; i++)
		//	while (true)
		//	{
//		    handlingDistancePart = true;

			acc_detector_distance_result_t result = { 0 };

			if (!do_detector_get_next(&resources, &sensor_cal_result, &result))
			{
				printf("Could not get next result\n");
				cleanup(&resources);
				return EXIT_FAILURE;
			}

			print_distance_result(&result);
//			acc_hal_integration_sensor_disable(SENSOR_ID);
			acc_sensor_destroy(resources.sensor);

		}

		else
		{
			acc_hal_integration_sensor_disable(SENSOR_ID);
			acc_hal_integration_sensor_enable(SENSOR_ID);
			control_helper_state.sensor = acc_sensor_create(control_helper_state.sensor_id);
			if (control_helper_state.sensor == NULL)
			{
				fprintf(stderr, "Failed to create sensor\n");
				return EXIT_FAILURE;
			}

			if(handlingSpeedPart == false)
			{

				if (!acc_control_helper_calibrate(&control_helper_state))
				{
					fprintf(stderr, "calibration failed\n");
	//				return EXIT_FAILURE;
				}

				acc_hal_integration_sensor_disable(SENSOR_ID);
				acc_hal_integration_sensor_enable(SENSOR_ID);

				if (!acc_sensor_prepare(control_helper_state.sensor, control_helper_state.config, &control_helper_state.cal_result, control_helper_state.buffer, control_helper_state.buffer_size))
				{
					fprintf(stderr, "acc_sensor_prepare failed\n");
					acc_sensor_status(control_helper_state.sensor);
					return EXIT_FAILURE;
				}


//				handlingSpeedPart = true;
			}



			res = acc_control_helper_get_next(&control_helper_state);
			if (!res)
			{
				printf("acc_control_helper_get_next() failed\n");
				break;
			}

			acc_get_iq_point_vector(&control_helper_state, 0, point_vector);
			acc_vector_iq_subtract_mean_inline(point_vector);
			acc_vector_iq_mult(point_vector, hanning_window, fft_vector);
			acc_vector_iq_fft_inline(fft_vector);
			acc_vector_iq_amplitude(fft_vector, fft_magnitudes);

			max_magnitude_index = acc_vector_float_argmax(fft_magnitudes); //Data ka max index return kr raha hai

	//		float speed;
			float thresholValue = 3000.00;

			if(fft_magnitudes->data[max_magnitude_index] > thresholValue)
			{
					if (max_magnitude_index >= SWEEPS_PER_FRAME / 2)
					{
						speed = ((max_magnitude_index - SWEEPS_PER_FRAME ) * speed_step_kmh) * (KM_TO_M/HOURS_TO_SEC);
					}
					else
					{
						speed = (max_magnitude_index * speed_step_kmh) * (KM_TO_M/HOURS_TO_SEC);  //* 0.27778 km/h to m/s
					}
			}

			else
			{
				speed = 0;
			}
//			printf("speed: %f\t magnitudeValue: %f\t speedIndex: %d\n" , speed , fft_magnitudes->data[max_magnitude_index], max_magnitude_index);


//			printf("Speed index: %" PRIi32 ",  Magnitude: %" PRIfloat " Speed: %" PRIfloat " km/h\n",
//			       max_magnitude_index,
//			       ACC_LOG_FLOAT_TO_INTEGER(fft_magnitudes->data[max_magnitude_index]),
//			       ACC_LOG_FLOAT_TO_INTEGER(speed));
//		  acc_hal_integration_sensor_disable(SENSOR_ID);
//		  free(control_helper_state.sensor);
			acc_sensor_destroy(control_helper_state.sensor);

		}
		counter++;

        if(10 == counter)
        {
            counter = 0;
            continue;
        }
//		handling = true;

//		HAL_Delay(20);
	}

clean_up:

	acc_vector_iq_free(point_vector);
	acc_vector_iq_free(fft_vector);
	acc_vector_iq_free(hanning_window);
	acc_control_helper_destroy(&control_helper_state);

	if (res && mem_ok)
	{
		printf("Application finished OK\n");
		return EXIT_SUCCESS;
	}
	else
	{
		return EXIT_FAILURE;
	}
}
static void cleanup(distance_detector_resources_t *resources)
{
	acc_hal_integration_sensor_disable(SENSOR_ID);
	acc_hal_integration_sensor_supply_off(SENSOR_ID);

	acc_detector_distance_config_destroy(resources->config);
	acc_detector_distance_destroy(resources->handle);

	acc_integration_mem_free(resources->buffer);
	acc_integration_mem_free(resources->calibration_buffer);

	if (resources->sensor != NULL)
	{
		acc_sensor_destroy(resources->sensor);
	}
}


static void set_config(acc_detector_distance_config_t *detector_config, distance_preset_config_t preset)
{
	// Add configuration of the detector here
	switch (preset)
	{
		case DISTANCE_PRESET_CONFIG_NONE:
			// Add configuration of the detector here
			break;

		case DISTANCE_PRESET_CONFIG_BALANCED:
			acc_detector_distance_config_start_set(detector_config, 0.25f);
			acc_detector_distance_config_end_set(detector_config, 3.0f);
			acc_detector_distance_config_max_step_length_set(detector_config, 0U);
			acc_detector_distance_config_max_profile_set(detector_config, ACC_CONFIG_PROFILE_5);
			acc_detector_distance_config_reflector_shape_set(detector_config, ACC_DETECTOR_DISTANCE_REFLECTOR_SHAPE_GENERIC);
			acc_detector_distance_config_peak_sorting_set(detector_config, ACC_DETECTOR_DISTANCE_PEAK_SORTING_STRONGEST);
			acc_detector_distance_config_threshold_method_set(detector_config, ACC_DETECTOR_DISTANCE_THRESHOLD_METHOD_FIXED_AMPLITUDE);
			acc_detector_distance_config_num_frames_recorded_threshold_set(detector_config, 100U);
			acc_detector_distance_config_threshold_sensitivity_set(detector_config, 0.5f);
			acc_detector_distance_config_signal_quality_set(detector_config, 15.0f);
			acc_detector_distance_config_close_range_leakage_cancellation_set(detector_config, false);
			break;

		case DISTANCE_PRESET_CONFIG_HIGH_ACCURACY:
			acc_detector_distance_config_start_set(detector_config, 0.25f);
			acc_detector_distance_config_end_set(detector_config, 3.0f);
			acc_detector_distance_config_max_step_length_set(detector_config, 2U);
			acc_detector_distance_config_max_profile_set(detector_config, ACC_CONFIG_PROFILE_3);
			acc_detector_distance_config_reflector_shape_set(detector_config, ACC_DETECTOR_DISTANCE_REFLECTOR_SHAPE_GENERIC);
			acc_detector_distance_config_peak_sorting_set(detector_config, ACC_DETECTOR_DISTANCE_PEAK_SORTING_STRONGEST);
			acc_detector_distance_config_threshold_method_set(detector_config, ACC_DETECTOR_DISTANCE_THRESHOLD_METHOD_CFAR);
			acc_detector_distance_config_num_frames_recorded_threshold_set(detector_config, 100U);
			acc_detector_distance_config_threshold_sensitivity_set(detector_config, 0.5f);
			acc_detector_distance_config_signal_quality_set(detector_config, 20.0f);
			acc_detector_distance_config_close_range_leakage_cancellation_set(detector_config, false);
			break;
	}
}


static bool initialize_detector_resources(distance_detector_resources_t *resources)
{
	resources->handle = acc_detector_distance_create(resources->config);
	if (resources->handle == NULL)
	{
		printf("acc_detector_distance_create() failed\n");
		return false;
	}

	if (!acc_detector_distance_get_buffer_sizes(resources->handle, &(resources->buffer_size), &(resources->calibration_buffer_size)))
	{
		printf("acc_detector_distance_get_buffer_sizes() failed\n");
		return false;
	}

	resources->buffer = acc_integration_mem_alloc(resources->buffer_size);
	if (resources->buffer == NULL)
	{
		printf("sensor buffer allocation failed\n");
		return false;
	}

	resources->calibration_buffer = acc_integration_mem_alloc(resources->calibration_buffer_size);
	if (resources->calibration_buffer == NULL)
	{
		printf("calibration buffer allocation failed\n");
		return false;
	}

	return true;
}


static bool do_sensor_calibration(acc_sensor_t     *sensor,
                                  acc_cal_result_t *sensor_cal_result,
                                  void             *buffer,
                                  uint32_t         buffer_size)
{
	bool           status              = false;
	bool           cal_complete        = false;
	const uint16_t calibration_retries = 1U;

	// Random disturbances may cause the calibration to fail. At failure, retry at least once.
	for (uint16_t i = 0; !status && (i <= calibration_retries); i++)
	{
		// Reset sensor before calibration by disabling/enabling it
		acc_hal_integration_sensor_disable(SENSOR_ID);
		acc_hal_integration_sensor_enable(SENSOR_ID);

		do
		{
			status = acc_sensor_calibrate(sensor, &cal_complete, sensor_cal_result, buffer, buffer_size);

			if (status && !cal_complete)
			{
				status = acc_hal_integration_wait_for_sensor_interrupt(SENSOR_ID, SENSOR_TIMEOUT_MS);
			}
		} while (status && !cal_complete);
	}

	if (status)
	{
		/* Reset sensor after calibration by disabling/enabling it */
		acc_hal_integration_sensor_disable(SENSOR_ID);
		acc_hal_integration_sensor_enable(SENSOR_ID);
	}

	return status;
}


static bool do_full_detector_calibration(distance_detector_resources_t *resources,
                                         const acc_cal_result_t        *sensor_cal_result)
{
	bool done = false;
	bool status;

	do
	{
		status = acc_detector_distance_calibrate(resources->sensor,
		                                         resources->handle,
		                                         sensor_cal_result,
		                                         resources->buffer,
		                                         resources->buffer_size,
		                                         resources->calibration_buffer,
		                                         resources->calibration_buffer_size,
		                                         &done);

		if (status && !done)
		{
			status = acc_hal_integration_wait_for_sensor_interrupt(SENSOR_ID, SENSOR_TIMEOUT_MS);
		}
	} while (status && !done);

	return status;
}


static bool do_detector_recalibration(distance_detector_resources_t *resources,
                                      const acc_cal_result_t        *sensor_cal_result)
{
	bool done = false;
	bool status;

	do
	{
		status = acc_detector_distance_recalibrate(resources->sensor,
		                                           resources->handle,
		                                           sensor_cal_result,
		                                           resources->buffer,
		                                           resources->buffer_size,
		                                           resources->calibration_buffer,
		                                           resources->calibration_buffer_size,
		                                           &done);

		if (status && !done)
		{
			status = acc_hal_integration_wait_for_sensor_interrupt(SENSOR_ID, SENSOR_TIMEOUT_MS);
		}
	} while (status && !done);

	return status;
}


static bool do_detector_get_next(distance_detector_resources_t  *resources,
                                 const acc_cal_result_t         *sensor_cal_result,
                                 acc_detector_distance_result_t *result)
{
	bool result_available = false;

	do
	{
		if (!acc_detector_distance_prepare(resources->handle, resources->config, resources->sensor, sensor_cal_result, resources->buffer,
		                                   resources->buffer_size))
		{
			printf("acc_detector_distance_prepare() failed\n");
			return false;
		}

		if (!acc_sensor_measure(resources->sensor))
		{
			printf("acc_sensor_measure() failed\n");
			return false;
		}

		if (!acc_hal_integration_wait_for_sensor_interrupt(SENSOR_ID, SENSOR_TIMEOUT_MS))
		{
			printf("Sensor interrupt timeout\n");
			return false;
		}

		if (!acc_sensor_read(resources->sensor, resources->buffer, resources->buffer_size))
		{
			printf("acc_sensor_read() failed\n");
			return false;
		}

		if (!acc_detector_distance_process(resources->handle, resources->buffer, resources->calibration_buffer, &result_available,
		                                   result))
		{
			printf("acc_detector_distance_process() failed\n");
			return false;
		}
	} while (!result_available);

	return true;
}

volatile uint8_t numDistances;
volatile float distances[ACC_DETECTOR_DISTANCE_RESULT_MAX_NUM_DISTANCES];

static void print_distance_result(const acc_detector_distance_result_t *result)
{
	printf("%d detected distances", result->num_distances);
	numDistances = result->num_distances;
	if (result->num_distances > 0)
	{
		printf(": ");

		for (uint8_t i = 0; i < result->num_distances; i++)
		{
			printf("%" PRIfloat " m ", ACC_LOG_FLOAT_TO_INTEGER(result->distances[i]));
			distances[i] = result->distances[i];

		}
	}

	printf("\n");
}

static void update_configuration(acc_config_t *config)
{
	int32_t start_point = 360; // measure at 900 mm

	acc_config_start_point_set(config, start_point);
	acc_config_num_points_set(config, 1);
	acc_config_profile_set(config, ACC_CONFIG_PROFILE_5);
	acc_config_hwaas_set(config, 8); // Lower this value to get a higher max speed
	// The processing in this example assumes that sweeps_per_frame is a power of two > 1
	acc_config_sweeps_per_frame_set(config, SWEEPS_PER_FRAME);
	acc_config_prf_set(config, ACC_CONFIG_PRF_15_6_MHZ);
	acc_config_sweep_rate_set(config, 0); // 0 means max sweep rate
	acc_config_inter_sweep_idle_state_set(config, ACC_CONFIG_IDLE_STATE_READY);
	acc_config_inter_frame_idle_state_set(config, ACC_CONFIG_IDLE_STATE_READY);
	acc_config_double_buffering_set(config, true);
}

// TODO: Move the following functions to acc_processing_helpers.c

static void acc_vector_iq_init_hanning_window(acc_vector_iq_t *vector_iq)
{
	uint32_t n = vector_iq->data_length;

	for (uint32_t i = 0; i < n; i++)
	{
		vector_iq->data[i] = 0.5f - 0.5f * cosf(2 * PI * i / (n - 1.0f));
//		printf("Data[%d]: %f + %fi\n", i, crealf(vector_iq->data[i]), cimagf(vector_iq->data[i]));
	}
}

static void acc_vector_iq_subtract_mean_inline(acc_vector_iq_t *vector_iq)
{
	complex float sum = 0;

	for (uint32_t i = 0; i < vector_iq->data_length; i++)
	{
		sum += vector_iq->data[i];
	}

	complex float mean = sum / vector_iq->data_length;

	for (uint32_t i = 0; i < vector_iq->data_length; i++)
	{
		vector_iq->data[i] -= mean;
//		printf("Data[%d] - mean : %f + %fi\n", i, crealf(vector_iq->data[i]), cimagf(vector_iq->data[i]));
	}
}

static void acc_vector_iq_fft_inline(acc_vector_iq_t *vector_iq)
{
	assert(vector_iq->data_length >= 4);	// Only vectors with length four or longer are supported
	assert(vector_iq->data_length <= 4096); // Precision degrades with increasing length,
	// more than 2 ** 12 elements is not recommended.
	assert((vector_iq->data_length & (vector_iq->data_length - 1)) == 0); // Length must be a power of two

	fft_inline(vector_iq->data, 1, vector_iq->data_length);


}

static void fft_inline(complex float *z, uint32_t stride, uint32_t n)
{
	assert(n >= 4);				// Number of elements should be at least four
	assert((n & (n - 1)) == 0); // Number of elements must be a power of two

	// Perform element reordering
	uint16_t reverse_i = 0;
	for (uint16_t i = 0; i < n; i++)
	{
		if (i < reverse_i)
		{
			float complex tmp = z[i * stride];
			z[i * stride] = z[reverse_i * stride];
			z[reverse_i * stride] = tmp;
		}

		uint16_t bit = n >> 1;
		while (bit & reverse_i)
		{
			reverse_i &= ~bit;
			bit >>= 1;
		}
		reverse_i |= bit;
	}

	// Perform 4-element base transformations
	for (uint16_t i = 0; i < n; i += 4)
	{
		float complex s0 = z[i * stride] + z[(i + 1) * stride];
		float complex d0 = z[i * stride] - z[(i + 1) * stride];
		float complex s1 = z[(i + 2) * stride] + z[(i + 3) * stride];
		float complex d1 = z[(i + 2) * stride] - z[(i + 3) * stride];

		d1 = cimagf(d1) - I * crealf(d1); // d1 = -I*d1;

		z[(i + 0) * stride] = s0 + s1;
		z[(i + 2) * stride] = s0 - s1;
		z[(i + 1) * stride] = d0 + d1;
		z[(i + 3) * stride] = d0 - d1;
	}

	// Main part of the FFT computation
	uint16_t block_length = 4;
	float complex phase_incr = -I;

	while (block_length < n)
	{
		// Update the phase_incr unit vector to have half phase angle
		phase_incr = (phase_incr + 1) / cabsf(phase_incr + 1);
		// phase_incr = complex_phase_half_angle(phase_incr);

		float complex phase = 1.0f;
		for (uint16_t m = 0; m < block_length; m++)
		{
			for (uint16_t i = m; i < n; i += block_length << 1)
			{
				float complex delta = z[(i + block_length) * stride] * phase;

				z[(i + block_length) * stride] = z[i * stride] - delta;

				z[i * stride] += delta;
			}

			// This phase increment is the leading error source for large transforms
			phase = phase * phase_incr;
		}

		block_length <<= 1;
	}
}
