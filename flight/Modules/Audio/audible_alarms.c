/**
 ******************************************************************************
 * @addtogroup dronin Modules
 *
 * @file       audible_alerts.c
 * @author     dRonin, http://dronin.org Copyright (C) 2016
 * @brief      provides select tones on DAC output channel for system
 *             warnings, errors, and critical events
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

// ****************

#include "openpilot.h"
#include "stm32f4xx_dac.h"
#include "systemalarms.h"
#include "pios_thread.h"
#include "pios_semaphore.h"

#include "flightstatus.h"

// ****************
// Private functions
int32_t AudibleAlarmsStart(void);
static void AudibleAlarmsTask(void *parameters);
int32_t AudibleAlarmsInitialize(void);
uint8_t getTopAlarmSeverity(SystemAlarmsData *alarm, uint32_t ignores);

// ****************
// Private constants
#define STACK_SIZE_BYTES 512
#define TASK_PRIORITY    PIOS_THREAD_PRIO_LOW
#define UPDATE_PERIOD    120


// ****************
// Private variables
static bool module_enabled = true;
static struct pios_thread *taskHandle;
struct pios_semaphore * audiblealarmsSemaphore = NULL;


// ****************
// The variables should be accessible in GCS for user selection
uint32_t DAC_Channel = DAC_Channel_1;
uint32_t alarm_ignores = 0x90; // no audible alarm for SYSTEMEVENT and ACTUATOR alerts
                               // (see alarm_names[] in flight/Libraries/alarms.c)
uint8_t audible_alarms_when_disarmed = false; // audible alarms when disarmed can be annoying,
                                              // give user chance to disable



/**
 * Start the audible alarms module
 */
int32_t AudibleAlarmsStart(void)
{
	if (module_enabled) {
		audiblealarmsSemaphore = PIOS_Semaphore_Create();

		taskHandle = PIOS_Thread_Create(AudibleAlarmsTask, "AudibleAlarms", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
		TaskMonitorAdd(TASKINFO_RUNNING_AUDIBLEALARMS, taskHandle);

		return 0;
	}
	return -1;
}


/**
 * Initialize the audio output DAC and timer 6 to generate triangle
 * waves for audio alerts
 */
int32_t AudibleAlarmsInitialize(void)
{
	DAC_InitTypeDef DAC_Struct;
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef    TIM_TimeBaseStructure;


	DAC_Struct.DAC_Trigger = DAC_Trigger_T6_TRGO;	// DAC clocked by TIM6
	DAC_Struct.DAC_WaveGeneration = DAC_WaveGeneration_Triangle;

	// this parameter sets the amplitude of the DAC output (1023 is a reasonable volume)
	// may want this configurable in the furture
	DAC_Struct.DAC_LFSRUnmask_TriangleAmplitude = DAC_TriangleAmplitude_1023;

	// may want output drive strength to be configurable by the user in the future
	DAC_Struct.DAC_OutputBuffer = DAC_OutputBuffer_Enable;

	// set the DAC1 pin to analog with no pull up/down
	if (DAC_Channel == DAC_Channel_1)
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	else
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	// enable the internal clocks in order to configure the DAC and GPIO
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);

	// configure the DAC and GPIO
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	DAC_Init( DAC_Channel, &DAC_Struct );

	// disable the DAC output, when an alarm occurs, DAC will be enabled
	DAC_Cmd( DAC_Channel, DISABLE );

	/* ensure interrupts disabled for the DAC */
	DAC_ITConfig( DAC_Channel, DAC_IT_DMAUDR, DISABLE );

	/* TIM6 Periph clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	     
	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = 0x40; // 168MHz/TriangleAmplitude/4/((prescaler+1)*Period) =~ 635Hz
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
	    
	/* TIM6 TRGO selection */
	TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);
			   
	/* ensure interrupts disabled for the timer */
	TIM_ITConfig( TIM6, TIM_IT_Update, DISABLE );

	/* let timer free run */
	TIM_Cmd(TIM6, ENABLE);

	return 0;
}




MODULE_INITCALL(AudibleAlarmsInitialize, AudibleAlarmsStart);

/**
 * audible alarm task. It does not return.
 */
static void AudibleAlarmsTask(__attribute__((unused)) void *parameters)
{
	SystemAlarmsData alarm;
	uint8_t arm_status = FLIGHTSTATUS_ARMED_DISARMED;
	uint8_t shift_count = 0;
	uint8_t shift_pattern = 0;
	uint8_t current_pattern = 0;
	uint8_t audiblePattern = 0;
	uint8_t current_alarm_severity = SYSTEMALARMS_ALARM_OK;

	while (1) {
		PIOS_Thread_Sleep(UPDATE_PERIOD);

        	// get the current audible alarm severity
		SystemAlarmsGet(&alarm);
        	current_alarm_severity = getTopAlarmSeverity (&alarm, alarm_ignores);


		// update the audible pattern based on current alert levels
		if (current_alarm_severity == SYSTEMALARMS_ALARM_CRITICAL)
			audiblePattern = 0x55;
		else if (current_alarm_severity == SYSTEMALARMS_ALARM_ERROR)
			audiblePattern = 0x11;
		else if (current_alarm_severity == SYSTEMALARMS_ALARM_WARNING)
			audiblePattern = 0x01;
		else
			audiblePattern = 0x00;

		// if pattern changed, force it to be used on this frame count
		if (audiblePattern != current_pattern) {
			shift_count = 7;
			current_pattern = audiblePattern;
		}

		// shift pattern and reload if max shift count reached
		if (++shift_count >= 8) {
			shift_count = 0;
			shift_pattern = current_pattern;
		}
		else
			shift_pattern >>= 1;

		// enable DAC output when LSB is set
		FlightStatusArmedGet(&arm_status);
		if (shift_pattern & 0x01
				&& (audible_alarms_when_disarmed || arm_status == FLIGHTSTATUS_ARMED_ARMED))
			DAC_Cmd( DAC_Channel, ENABLE );
		else
			DAC_Cmd( DAC_Channel, DISABLE );
	}
}

/**
 * @}
 */

