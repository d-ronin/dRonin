/*
    ChibiOS - Copyright (C) 2006..2016 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/**
 * @file    hal_compass.h
 * @brief   Generic compass interface header.
 *
 * @addtogroup HAL_COMPASS
 * @{
 */

#ifndef HAL_COMPASS_H
#define HAL_COMPASS_H

#include "hal_sensors.h"

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/**
 * @brief   BaseCompass specific methods.
 */
#define _base_compass_methods_alone                                         \
  /* Invoke the set bias procedure.*/                                       \
  msg_t (*set_bias)(void *instance, float biases[]);                        \
  /* Remove bias stored data.*/                                             \
  msg_t (*reset_bias)(void *instance);                                      \
  /* Invoke the set sensitivity procedure.*/                                \
  msg_t (*set_sensitivity)(void *instance, float sensitivities[]);          \
  /* Restore sensitivity stored data to default.*/                          \
  msg_t (*reset_sensitivity)(void *instance);


/**
 * @brief   BaseCompass specific methods with inherited ones.
 */
#define _base_compass_methods                                               \
  _base_sensor_methods                                                      \
  _base_compass_methods_alone

/**
 * @brief   @p BaseCompass virtual methods table.
 */
struct BaseCompassVMT {
  _base_compass_methods
};

/**
 * @brief   @p BaseCompass specific data.
 */
#define _base_compass_data                                                  \
  _base_sensor_data
	
/**
 * @brief   Base compass class.
 * @details This class represents a generic compass.
 */
typedef struct {
  /** @brief Virtual Methods Table.*/
  const struct BaseCompassVMT *vmt_basecompass;
  _base_compass_data
} BaseCompass;

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/
/**
 * @name    Macro Functions (BaseCompass)
 * @{
 */
/**
 * @brief   Compass get axes number.
 *
 * @param[in] ip        pointer to a @p BaseCompass class.
 * @return              The number of axes of the BaseCompass
 *
 * @api
 */
#define compassGetAxesNumber(ip)                                            \
        (ip)->vmt_basecompass->get_channels_number(ip)

/**
 * @brief   Compass read raw data.
 *
 * @param[in] ip        pointer to a @p BaseCompass class.
 * @param[in] dp        pointer to a data array.
 * 
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more errors occurred.
 *
 * @api
 */
#define compassReadRaw(ip, dp)                                              \
        (ip)->vmt_basecompass->read_raw(ip, dp)

/**
 * @brief   Compass read cooked data.
 *
 * @param[in] ip        pointer to a @p BaseCompass class.
 * @param[in] dp        pointer to a data array.
 * 
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more errors occurred.
 *
 * @api
 */
#define compassReadCooked(ip, dp)                                           \
        (ip)->vmt_basecompass->read_cooked(ip, dp)

/**
 * @brief   Updates compass bias data from received buffer.
 * @note    The bias buffer must have the same length of the
 *          the compass axes number.
 *
 * @param[in] ip        pointer to a @p BaseCompass class.
 * @param[in] bp        pointer to a buffer of bias values.
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more errors occurred.
 *
 * @api
 */
#define compassSetBias(ip, bp)                                            \
        (ip)->vmt_basecompass->set_bias(ip, bp)

/**
 * @brief   Reset compass bias data restoring it to zero.
 *
 * @param[in] ip        pointer to a @p BaseCompass class.
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more errors occurred.
 *
 * @api
 */
#define compassResetBias(ip)                                               \
        (ip)->vmt_basecompass->reset_bias(ip)

/**
 * @brief   Updates compass sensitivity data from received buffer.
 * @note    The sensitivity buffer must have the same length of the
 *          the compass axes number.
 *
 * @param[in] ip        pointer to a @p BaseCompass class.
 * @param[in] sp        pointer to a buffer of sensitivity values.
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more errors occurred.
 *
 * @api
 */
#define compassSetSensitivity(ip, sp)                                     \
        (ip)->vmt_basecompass->set_sensitivity(ip, sp)

/**
 * @brief   Reset compass sensitivity data restoring it to its typical
 *          value.
 *
 * @param[in] ip        pointer to a @p BaseCompass class.
 *
 * @return              The operation status.
 * @retval MSG_OK       if the function succeeded.
 * @retval MSG_RESET    if one or more errors occurred.
 *
 * @api
 */
#define compassResetSensitivity(ip)                                       \
        (ip)->vmt_basecompass->reset_sensitivity(ip)
/** @} */

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* HAL_COMPASS_H */

/** @} */
