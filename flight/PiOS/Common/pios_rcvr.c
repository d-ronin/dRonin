/* Project Includes */
#include "pios.h"

#include "pios_semaphore.h"
#include "pios_thread.h"

#if defined(PIOS_INCLUDE_RCVR)

#include <pios_rcvr_priv.h>

enum pios_rcvr_dev_magic {
  PIOS_RCVR_DEV_MAGIC = 0x99aabbcc,
};

struct pios_rcvr_dev {
  enum pios_rcvr_dev_magic        magic;
  uintptr_t                       lower_id;
  const struct pios_rcvr_driver * driver;
};

static bool PIOS_RCVR_validate(struct pios_rcvr_dev * rcvr_dev)
{
  return (rcvr_dev->magic == PIOS_RCVR_DEV_MAGIC);
}

static struct pios_rcvr_dev * PIOS_RCVR_alloc(void)
{
  struct pios_rcvr_dev * rcvr_dev;

  rcvr_dev = (struct pios_rcvr_dev *)PIOS_malloc(sizeof(*rcvr_dev));
  if (!rcvr_dev) return (NULL);

  rcvr_dev->magic = PIOS_RCVR_DEV_MAGIC;
  return(rcvr_dev);
}

static struct pios_semaphore *rcvr_activity;
static uint32_t rcvr_last_wake;

/**
  * Initialises RCVR layer
  * \param[out] handle
  * \param[in] driver
  * \param[in] id
  * \return < 0 if initialisation failed
  */
int32_t PIOS_RCVR_Init(uintptr_t * rcvr_id, const struct pios_rcvr_driver * driver, uintptr_t lower_id)
{
  PIOS_DEBUG_Assert(rcvr_id);
  PIOS_DEBUG_Assert(driver);

  struct pios_rcvr_dev * rcvr_dev;

  // For idempotency in all environments...
  PIOS_IRQ_Disable();
  if (!rcvr_activity) {
    rcvr_activity = PIOS_Semaphore_Create();
  }
  PIOS_IRQ_Enable();

  rcvr_dev = (struct pios_rcvr_dev *) PIOS_RCVR_alloc();
  if (!rcvr_dev) goto out_fail;

  rcvr_dev->driver   = driver;
  rcvr_dev->lower_id = lower_id;

  *rcvr_id = (uintptr_t)rcvr_dev;
  return(0);

out_fail:
  return(-1);
}

/**
  * Gets the encapsulated receiver driver.
  * \param[in] rcvr_id Driver to query.
  * \return 0 on failure, otherwise the lower device.
  */
uintptr_t PIOS_RCVR_GetLowerDevice(uintptr_t rcvr_id)
{
  struct pios_rcvr_dev *dev = (struct pios_rcvr_dev*)rcvr_id;

  if (!PIOS_RCVR_validate(dev))
    return 0;

  return dev->lower_id;
}

/**
 * @brief Reads an input channel from the appropriate driver
 * @param[in] rcvr_id driver to read from
 * @param[in] channel channel to read
 * @returns Unitless input value
 *  @retval PIOS_RCVR_TIMEOUT indicates a failsafe or timeout from that channel
 *  @retval PIOS_RCVR_INVALID invalid channel for this driver (usually out of range supported)
 *  @retval PIOS_RCVR_NODRIVER driver was not initialized
 */
int32_t PIOS_RCVR_Read(uintptr_t rcvr_id, uint8_t channel)
{
	// Publicly facing API uses channel 1 for first channel
	if(channel == 0)
		return PIOS_RCVR_INVALID;
	else
		channel--;

  if (rcvr_id == 0) 
    return PIOS_RCVR_NODRIVER;

  struct pios_rcvr_dev * rcvr_dev = (struct pios_rcvr_dev *)rcvr_id;

  if (!PIOS_RCVR_validate(rcvr_dev)) {
    /* Undefined RCVR port for this board (see pios_board.c) */
    PIOS_Assert(0);
  }

  PIOS_DEBUG_Assert(rcvr_dev->driver->read);

  return rcvr_dev->driver->read(rcvr_dev->lower_id, channel);
}

#define MIN_WAKE_INTERVAL_uS 4000	/* 250Hz ought to be enough for anyone*/

bool PIOS_RCVR_WaitActivity(uint32_t timeout_ms) {
  if (rcvr_activity) {
    bool result = PIOS_Semaphore_Take(rcvr_activity, timeout_ms);
    if(!result) {
      // If the semaphore times out, update last wake, to try to avoid
      // resonance due to jitter, when the update rate of the receiver
      // matches the timeout.
      rcvr_last_wake = PIOS_DELAY_GetRaw();
    }
    return result;
  } else {
    PIOS_Thread_Sleep(timeout_ms);

    return false;
  }
}

void PIOS_RCVR_Active() {
  if (PIOS_DELAY_DiffuS(rcvr_last_wake) >= MIN_WAKE_INTERVAL_uS) {
    if (rcvr_activity) {
      rcvr_last_wake = PIOS_DELAY_GetRaw();
      PIOS_Semaphore_Give(rcvr_activity);
    }
#ifdef FLIGHT_POSIX
    if (PIOS_Thread_FakeClock_IsActive()) {
      PIOS_Thread_FakeClock_UpdateBarrier(100);
    }
#endif
  }
}

void PIOS_RCVR_ActiveFromISR() {
  if (PIOS_DELAY_DiffuS(rcvr_last_wake) >= MIN_WAKE_INTERVAL_uS) {
    bool dont_care;

    if (rcvr_activity) {
      rcvr_last_wake = PIOS_DELAY_GetRaw();
      PIOS_Semaphore_Give_FromISR(rcvr_activity, &dont_care);
    }
  }
}

#endif

/**
 * @}
 * @}
 */
