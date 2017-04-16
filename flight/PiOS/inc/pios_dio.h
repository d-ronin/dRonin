#ifndef _PIOS_DIO_H
#define _PIOS_DIO_H

/* Default implementation of PIOS_DIO.  Used by device-agnostic libraries and
 * the posix env now.
 */

#if !defined(SIM_POSIX) && !defined(PIOS_NO_HW)
#error Device-agnostic PIOS_DIO is only for sim or libs.
#endif

typedef void * dio_tag_t;

#endif
