//#define USE_SERIAL_4WAY_BLHELI_INTERFACE

#define MAX_SUPPORTED_MOTORS 8

typedef uint8_t ioTag_t;
typedef void* IO_t;

typedef struct serialPort_s serialPort_t;

// NONE initializer for IO_t variable
#define IO_NONE ((IO_t)0)

typedef struct {
	bool enabled;
	IO_t io;
} pwmOutputPort_t;

#if 0
02:42 < icee> so need.. an io tag implementation, IORead, IOLo, IOHi, IOConfigGPIO, 
              pwmDisableMotors, pwmGetMotors, pwmEnableMotors, serialRxBytesWaiting, 
              serialRead, serialBeginWrite, serialWrite, serialTxBytesFree, 
              serialEndWrite
#endif
