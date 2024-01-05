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

#include "acc_config.h"
#include "acc_hal_definitions_a121.h"
#include "acc_hal_integration_a121.h"
#include "acc_integration_log.h"
#include "acc_rss_a121.h"
#include "acc_version.h"

#include "acc_processing_helpers.h"

#define SENSOR_ID (1U)

#define SWEEPS_PER_FRAME 512
#define PI (3.14159265358979323846f)

static void update_configuration(acc_config_t *config);

static void acc_vector_iq_fft_inline(acc_vector_iq_t *vector_iq);

static void fft_inline(complex float *z, uint32_t stride, uint32_t n);

static void acc_vector_iq_init_hanning_window(acc_vector_iq_t *vector_iq);

static void acc_vector_iq_subtract_mean_inline(acc_vector_iq_t *vector_iq);

int acconeer_main(int argc, char *argv[]);

int acconeer_main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	acc_control_helper_t control_helper_state = {0};

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

	bool mem_ok = (point_vector != NULL) && (fft_vector != NULL) && (hanning_window != NULL);
	if (!mem_ok)
	{
		printf("Memory allocation for vectors failed\n");
		goto clean_up;
	}

	acc_vector_iq_init_hanning_window(hanning_window);

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

	uint32_t iterations = 25;
//	for (uint32_t i = 0U; i < iterations; i++)
	while (true)
	{
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

		int32_t max_magnitude_index = acc_vector_float_argmax(fft_magnitudes);

		float speed;
		if (max_magnitude_index >= SWEEPS_PER_FRAME / 2)
		{
			speed = (max_magnitude_index - SWEEPS_PER_FRAME) * speed_step_kmh;
		}
		else
		{
			speed = max_magnitude_index * speed_step_kmh;
		}

		printf("Speed index: %" PRIi32 ",  Magnitude: %" PRIfloat " Speed: %" PRIfloat " km/h\n",
			   max_magnitude_index,
			   ACC_LOG_FLOAT_TO_INTEGER(fft_magnitudes->data[max_magnitude_index]),
			   ACC_LOG_FLOAT_TO_INTEGER(speed));
//		HAL_Delay(200);
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
