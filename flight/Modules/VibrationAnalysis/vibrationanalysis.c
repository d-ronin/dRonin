/**
 ******************************************************************************
 * @addtogroup TauLabsModules Tau Labs Modules
 * @{
 * @addtogroup VibrationAnalysisModule Vibration analysis module
 * @{
 *
 * @file       vibrationanalysis.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013-2014
 * @brief      Gathers data on the accels to estimate vibration
 *
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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

/**
 * Input objects: @ref Accels, @ref VibrationAnalysisSettings
 * Output object: @ref VibrationAnalysisOutput
 *
 * This module executes on a timer trigger. When the module is
 * triggered it will update the data of VibrationAnalysiOutput,
 * with the accumulated accelerometer samples. 
 */

#include "openpilot.h"
#include "physical_constants.h"
#include "pios_thread.h"
#include "pios_queue.h"

#include "accels.h"
#include "modulesettings.h"
#include "vibrationanalysisoutput.h"
#include "vibrationanalysissettings.h"


// Private constants

#define MAX_QUEUE_SIZE 2

#define STACK_SIZE_BYTES (200 + 448 + 16 + (2*3*window_size)*0) // The memory requirement grows linearly 
																				  // with window size. The constant is multiplied
																				  // by 0 in order to reflect the fact that the
																				  // malloc'ed memory is not taken from the module 
																				  // but instead from the heap. Nonetheless, we 
																				  // can know a priori how much RAM this module 
																				  // will take.
#define TASK_PRIORITY PIOS_THREAD_PRIO_LOW
#define SETTINGS_THROTTLING_MS 100

#define MAX_ACCEL_RANGE 16                          // Maximum accelerometer resolution in [g]
#define FLOAT_TO_FIXED (32768/(MAX_ACCEL_RANGE*2)-1) // This is the scaling constant that scales input floats
#define VIBRATION_ELEMENTS_COUNT 16  // Number of elements per object Instance

#define MAX_WINDOW_SIZE 1024

// Comment for larger smaller buffers and much better accuracy. The maximum window size will be allocated.
#define USE_SINGLE_INSTANCE_BUFFERS 1

// Uncomment to enable freeing buffer memory if the PIOS_free method does something useful
// #define PIOS_FREE_IMPLEMENTED 1

// Private variables
static struct pios_thread *taskHandle;
static TaskInfoRunningElem task;
static struct pios_queue *queue;
static bool module_enabled = false;

static struct VibrationAnalysis_data {
	uint16_t accels_sum_count;
	uint16_t window_size;
    uint16_t buffers_size;
	uint16_t instances;

	float accels_data_sum_x;
	float accels_data_sum_y;
	float accels_data_sum_z;
	
	float accels_static_bias_x; // In all likelyhood, the initial values will be close to 
	float accels_static_bias_y; // (0,0,-g). In the case where they are not, this will still  
	float accels_static_bias_z; // converge to the true bias in a few thousand measurements.	
	
	int16_t *accel_buffer_x;
	int16_t *accel_buffer_y;
	int16_t *accel_buffer_z;
} *vtd;


// Private functions
static void VibrationAnalysisTask(void *parameters);

/*
*   Releases any memory dinamically allocated
*   Because this module can have a big footprint this will ensure we can
*   reduce it when something fails.
*/
static void VibrationAnalysisCleanup(void) {
    // Cleanup allocated memory
    module_enabled = false;

    // Stop main task
    if (taskHandle != NULL) {
        TaskMonitorRemove(task);
        PIOS_Thread_Delete(taskHandle);
        taskHandle = NULL;
    }

#ifdef PIOS_FREE_IMPLEMENTED
    // Cleanup
    if (vtd != NULL) {
        PIOS_free(vtd);
        vtd = NULL;        
    }

    if (vtd->accel_buffer_x != NULL){
        PIOS_free(vtd->accel_buffer_x);
        vtd->accel_buffer_x = NULL;
    }
    
    if (vtd->accel_buffer_y != NULL) {
        PIOS_free(vtd->accel_buffer_y);
        vtd->accel_buffer_y = NULL;

    }

    if (vtd->accel_buffer_z != NULL) {
        PIOS_free(vtd->accel_buffer_z);
        vtd->accel_buffer_z = NULL;
    }
#endif

}

/**
 * Start the module, called on startup
 */
static int32_t VibrationAnalysisStart(void)
{
	
	if (!module_enabled)
		return -1;

    //Get the window size
    uint16_t window_size; // Make a local copy in order to check settings before allocating memory
    uint16_t instances = 1;

    // Allocate and initialize the static data storage only if module is enabled the first time
    if (vtd == NULL){
        vtd = (struct VibrationAnalysis_data *) PIOS_malloc(sizeof(struct VibrationAnalysis_data));
        if (vtd == NULL) {
            module_enabled = false;
            return -1;
        }
        
        // make sure that all struct values are zeroed...
        memset(vtd, 0, sizeof(struct VibrationAnalysis_data));
        //... except for Z axis static bias
        vtd->accels_static_bias_z -= GRAVITY; // [See note in definition of VibrationAnalysis_data structure]
    }

    VibrationAnalysisSettingsFFTWindowSizeOptions window_size_enum;
    VibrationAnalysisSettingsFFTWindowSizeGet(&window_size_enum);
    switch (window_size_enum) {
        case VIBRATIONANALYSISSETTINGS_FFTWINDOWSIZE_16:
            window_size = 16;
            break;
        case VIBRATIONANALYSISSETTINGS_FFTWINDOWSIZE_64:
            window_size = 64;
            break;
        case VIBRATIONANALYSISSETTINGS_FFTWINDOWSIZE_256:
            window_size = 256;
            break;
        case VIBRATIONANALYSISSETTINGS_FFTWINDOWSIZE_1024:
            window_size = 1024;
            break;
        default:
            //This represents a serious configuration error. Do not start module.
            module_enabled = false;
            return -1;
            break;
    }

    // Is the new window size different?
    // Will happen upon initialization and when the window size changes
    if (window_size != vtd->window_size) {

        instances = window_size / VIBRATION_ELEMENTS_COUNT;

#ifndef USE_SINGLE_INSTANCE_BUFFERS
        // Check number of existing instances
        uint16_t existing_instances = VibrationAnalysisOutputGetNumInstances();
        if(existing_instances < instances) {
            //Create missing instances
            for (int i = existing_instances; i < instances; i++) {
                uint16_t ret = VibrationAnalysisOutputCreateInstance();
                if (ret == 0) {
                    // This fails when it's a metaobject. Not a very helpful test.
                    module_enabled = false;
                    return -1;
                }
            }
        }

        // We need at least these instances
        if (VibrationAnalysisOutputGetNumInstances() < instances) {
            return -1;
        }
#endif

#ifdef PIOS_FREE_IMPLEMENTED        
        // Delete existing buffers
        if (vtd->accel_buffer_x != NULL)
            PIOS_free(vtd->accel_buffer_x);
        if (vtd->accel_buffer_y != NULL)
            PIOS_free(vtd->accel_buffer_y);
        if (vtd->accel_buffer_z != NULL)
            PIOS_free(vtd->accel_buffer_z);
#endif

        // Clear buffers
        memset(vtd, 0, sizeof(struct VibrationAnalysis_data));
        vtd->accels_static_bias_z -= GRAVITY; // [See note in definition of VibrationAnalysis_data structure]

        // Now place the window size into the buffer
        vtd->window_size = window_size;
        vtd->instances = instances;
        
#ifdef USE_SINGLE_INSTANCE_BUFFERS
        vtd->buffers_size = VIBRATION_ELEMENTS_COUNT; 
#else

    #ifdef PIOS_FREE_IMPLEMENTED        
        vtd->buffers_size = window_size;
    #else
        vtd->buffers_size = MAX_WINDOW_SIZE; 

    #endif

#endif


        //Create new buffers if needed.
        if (vtd->accel_buffer_x == NULL) {
            vtd->accel_buffer_x = (int16_t *) PIOS_malloc(vtd->buffers_size*sizeof(typeof(*vtd->accel_buffer_x)));
            if (vtd->accel_buffer_x == NULL) {
                VibrationAnalysisCleanup();

                module_enabled = false;
                return -1;
            }
        }

        if (vtd->accel_buffer_y == NULL) {
            vtd->accel_buffer_y = (int16_t *) PIOS_malloc(vtd->buffers_size*sizeof(typeof(*vtd->accel_buffer_y)));
            if (vtd->accel_buffer_y == NULL) {
                VibrationAnalysisCleanup();

                module_enabled = false;
                return -1;
            }
        }

        if (vtd->accel_buffer_z == NULL) {
            vtd->accel_buffer_z = (int16_t *) PIOS_malloc(vtd->buffers_size*sizeof(typeof(*vtd->accel_buffer_z)));
            if (vtd->accel_buffer_z == NULL) {
                VibrationAnalysisCleanup();

                module_enabled = false;
                return -1;
            }
        }
    }
    
    // Start main task
    if (taskHandle == NULL) {
        taskHandle = PIOS_Thread_Create(VibrationAnalysisTask, "VibrationAnalysis", STACK_SIZE_BYTES, NULL, TASK_PRIORITY);
        task = TaskMonitorAdd(TASKINFO_RUNNING_VIBRATIONANALYSIS, taskHandle);
    }

    return 0;
}


/**
 * Initialise the module, called on startup
 */
static int32_t VibrationAnalysisInitialize(void)
{
	if (ModuleSettingsInitialize() == -1) {
        module_enabled = false;
        return -1;
    }
	
#ifdef MODULE_VibrationAnalysis_BUILTIN
	module_enabled = true;
#else
	uint8_t module_state[MODULESETTINGS_ADMINSTATE_NUMELEM];
	ModuleSettingsAdminStateGet(module_state);
	if (module_state[MODULESETTINGS_ADMINSTATE_VIBRATIONANALYSIS] == MODULESETTINGS_ADMINSTATE_ENABLED) {
		module_enabled = true;
	} else {
		module_enabled = false;
	}
#endif
	
	if (!module_enabled) //If module not enabled...
		return -1;

	// Initialize UAVOs
	if (VibrationAnalysisSettingsInitialize() == -1 || VibrationAnalysisOutputInitialize() == -1) {
        module_enabled = false;
        return -1;
    }
		
	// Create object queue
	queue = PIOS_Queue_Create(MAX_QUEUE_SIZE, sizeof(UAVObjEvent));
		
	return 0;
	
}
MODULE_INITCALL(VibrationAnalysisInitialize, VibrationAnalysisStart)


static void VibrationAnalysisTask(void *parameters)
{
    uint32_t lastSysTime;
    uint32_t lastSettingsUpdateTime;
    uint8_t runAnalysisFlag = VIBRATIONANALYSISSETTINGS_TESTINGSTATUS_OFF; // By default, turn analysis off
    uint16_t sampleRate_ms = 100; // Default sample rate of 100ms
    uint16_t sample_count;
    
    UAVObjEvent ev;
    
    // Listen for updates.
    AccelsConnectQueue(queue);

    // Main task loop
    VibrationAnalysisOutputData vibrationAnalysisOutputData;
    sample_count = 0;
    lastSysTime = PIOS_Thread_Systime();
    lastSettingsUpdateTime = PIOS_Thread_Systime() - SETTINGS_THROTTLING_MS;
    
    vibrationAnalysisOutputData.samples = vtd->window_size;
    vibrationAnalysisOutputData.scale = FLOAT_TO_FIXED;

#ifdef USE_SINGLE_INSTANCE_BUFFERS
    uint16_t instance_number = 0;
#endif

    uint8_t  runningAcquisition = 0;

    // Main module task, never exit from while loop
    while (module_enabled)
    {

        // Only check settings once every 100ms and not in the middle of a buffer accumulation to reduce artifacts
        if (PIOS_Thread_Systime() - lastSettingsUpdateTime > SETTINGS_THROTTLING_MS && runningAcquisition == 0) {

            //First check if the analysis is active
            VibrationAnalysisSettingsTestingStatusGet(&runAnalysisFlag);
            
            // If analysis is turned off, delay and then loop.
            if (runAnalysisFlag == VIBRATIONANALYSISSETTINGS_TESTINGSTATUS_OFF) {
                PIOS_Thread_Sleep(200);
                continue;
            }
            // Get sample rate
            VibrationAnalysisSettingsSampleRateGet(&sampleRate_ms);
            sampleRate_ms = sampleRate_ms > 0 ? sampleRate_ms : 1; //Ensure sampleRate never is 0.
            
            //Reconfigure any parameter
            VibrationAnalysisStart();
            
            vibrationAnalysisOutputData.samples = vtd->window_size;

            lastSettingsUpdateTime = PIOS_Thread_Systime();

            runningAcquisition = 1;
        }
        

        // Wait until the Accels object is updated, and never time out
        if (PIOS_Queue_Receive(queue, &ev, PIOS_QUEUE_TIMEOUT_MAX) == true) {
            /**
             * Accumulate accelerometer data. This would be a great place to add a 
             * high-pass filter, in order to eliminate the DC bias from gravity.
             * Until then, a DC bias subtraction has been added in the main loop.
             */
            
            AccelsData accels_data;
            AccelsGet(&accels_data);
            
            vtd->accels_data_sum_x += accels_data.x;
            vtd->accels_data_sum_y += accels_data.y;
            vtd->accels_data_sum_z += accels_data.z;
            
            vtd->accels_sum_count++;
        }
        
        // If not enough time has passed, keep accumulating data
        if (PIOS_Thread_Systime() - lastSysTime < sampleRate_ms) {
            continue;
        }
        
        
        lastSysTime = PIOS_Thread_Systime();
        
        //Calculate averaged values
        float accels_avg_x = vtd->accels_data_sum_x / vtd->accels_sum_count;
        float accels_avg_y = vtd->accels_data_sum_y / vtd->accels_sum_count;
        float accels_avg_z = vtd->accels_data_sum_z / vtd->accels_sum_count;
        
        //Calculate DC bias
        float alpha = .005; //Hard-coded to drift very slowly
        vtd->accels_static_bias_x = alpha*accels_avg_x + (1-alpha)*vtd->accels_static_bias_x;
        vtd->accels_static_bias_y = alpha*accels_avg_y + (1-alpha)*vtd->accels_static_bias_y;
        vtd->accels_static_bias_z = alpha*accels_avg_z + (1-alpha)*vtd->accels_static_bias_z;
        
        // Add averaged values to the buffer, and remove DC bias.
        vtd->accel_buffer_x[sample_count] = (accels_avg_x - vtd->accels_static_bias_x)*FLOAT_TO_FIXED;
        vtd->accel_buffer_y[sample_count] = (accels_avg_y - vtd->accels_static_bias_y)*FLOAT_TO_FIXED;
        vtd->accel_buffer_z[sample_count] = (accels_avg_z - vtd->accels_static_bias_z)*FLOAT_TO_FIXED;
        
        //Reset the accumulators
        vtd->accels_data_sum_x = 0;
        vtd->accels_data_sum_y = 0;
        vtd->accels_data_sum_z = 0;
        vtd->accels_sum_count = 0;

        // Advance sample and reset when at buffer end
        sample_count++;

        // Process and dump an instance at a time
#ifdef USE_SINGLE_INSTANCE_BUFFERS
        if (sample_count == vtd->buffers_size) {
            // Dump an instance

            for (uint16_t k = 0; k < VIBRATION_ELEMENTS_COUNT; k++) {
                vibrationAnalysisOutputData.index = instance_number;
                vibrationAnalysisOutputData.x[k] = vtd->accel_buffer_x[k];
                vibrationAnalysisOutputData.y[k] = vtd->accel_buffer_y[k];
                vibrationAnalysisOutputData.z[k] = vtd->accel_buffer_z[k];
            }
            
            VibrationAnalysisOutputInstSet(0, &vibrationAnalysisOutputData);
            VibrationAnalysisOutputInstUpdated(0);

            // Increase the instance number or reset it
            instance_number = (instance_number + 1) % vtd->instances;
#else
        // Or process and dump the full window using multiple instances
        // This will probably introduce less artifacts at the cost of higher memory consumption
        if (sample_count == vtd->window_size) {
            for (uint16_t i = 0; i < vtd->instances; i++) {
                for (uint16_t k = 0; k < VIBRATION_ELEMENTS_COUNT; k++) {
                    vibrationAnalysisOutputData.index = i;
                    vibrationAnalysisOutputData.x[k] = vtd->accel_buffer_x[k + VIBRATION_ELEMENTS_COUNT * i];
                    vibrationAnalysisOutputData.y[k] = vtd->accel_buffer_y[k + VIBRATION_ELEMENTS_COUNT * i];
                    vibrationAnalysisOutputData.z[k] = vtd->accel_buffer_z[k + VIBRATION_ELEMENTS_COUNT * i];
                }

                VibrationAnalysisOutputInstSet(i, &vibrationAnalysisOutputData);
                VibrationAnalysisOutputInstUpdated(i);
            }
            
#endif
            // Erase buffer
            memset(vtd->accel_buffer_x, 0, vtd->buffers_size*sizeof(typeof(*(vtd->accel_buffer_x))));
            memset(vtd->accel_buffer_y, 0, vtd->buffers_size*sizeof(typeof(*(vtd->accel_buffer_y))));
            memset(vtd->accel_buffer_z, 0, vtd->buffers_size*sizeof(typeof(*(vtd->accel_buffer_z))));

            sample_count = 0;
            runningAcquisition = 0;
        }
    }
}

/**
 * @}
 * @}
 */
