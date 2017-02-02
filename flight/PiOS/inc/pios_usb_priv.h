 /**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup   PIOS_USB USB Setup Functions
 * @brief PIOS interface for USB device driver
 * @{
 *
 * @file       pios_usb_priv.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      USB private definitions.
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

#ifndef PIOS_USB_PRIV_H
#define PIOS_USB_PRIV_H

#include <pios.h>
#include <pios_stm32.h>

struct pios_usb_cfg {
	struct stm32_irq irq;
	struct stm32_gpio vsense;
	struct stm32_gpio disconnect;
};

extern int32_t PIOS_USB_Init(uintptr_t * usb_id, const struct pios_usb_cfg * cfg);

#endif /* PIOS_USB_PRIV_H */

/**
  * @}
  * @}
  */

