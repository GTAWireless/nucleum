
#include <stdint.h>
#include <stdbool.h>
#include "simple_logger.h"
#include "simple_timer.h"
#include "mem-ffs.h"
#include "stdarg.h"

static uint32_t simple_logger_timer = 0;
static uint8_t simple_logger_inited = 0;
static uint8_t simple_logger_file_exists = 0;

static uint8_t busy = 0;

#ifdef SIMPLE_LOGGER_BUFFER_SIZE
	static char buffer[SIMPLE_LOGGER_BUFFER_SIZE];
	static uint32_t buffer_size = SIMPLE_LOGGER_BUFFER_SIZE;
#else
	static char buffer[256];
	static uint32_t buffer_size = 256;
#endif


static FFS_FILE* simple_logger_fpointer;

static void heartbeat (void* p_context) {
	if(ffs_10ms_timer) {
		ffs_10ms_timer--;
	}
	if(simple_logger_timer) {
		simple_logger_timer--;
	}
}

uint8_t simple_logger_ready(void) {
	return ffs_is_card_available();
}

uint8_t simple_logger_init(const char *filename, const char *permissions) {

	if(busy) {
		return 1;
	} else {
		busy = 1;
	}

	if(simple_logger_inited) {
		busy = 0;
		return 1; //can only initialize once 
	}

	ffs_init();

	//initialize a simple timer
	simple_timer_init();
	simple_timer_start (10, heartbeat);

	//call ffs_process for 1 second and try to initalize a card
	simple_logger_timer = 100;
	while(!ffs_is_card_available()) {
		ffs_process();
		if(!simple_logger_timer) {
			busy = 0;
			return 1; //no card found - timeout - return error
		}
	}

	//we must have not timed out and a card is available
	if((permissions[0] != 'w' && permissions[0] != 'a') || permissions[1] != '\0') {
		//the person didn't use the right permissions
		busy = 0;
		return 1;
	}
	
	//see if the file exists already
	FFS_FILE* temp;
	temp = ffs_fopen(filename,"r");
	if(temp) {
		//the file exists
		ffs_fclose(temp);
		simple_logger_file_exists = 1;
	}

	simple_logger_fpointer = ffs_fopen(filename,permissions);
	if(!simple_logger_fpointer) {
		busy = 0;
		return 1; //idk what's up
	}

	busy = 0;
	return 0; //we are good - file opened.
}

void simple_logger_update() {
	ffs_process();	
}

//the function meant to log data
uint8_t simple_logger_log(const char *format, ...) {

	if(busy) {
		return 1;
	} else {
		busy = 1;
	}

	va_list argptr;
	va_start(argptr, format);
	vsnprintf(buffer, buffer_size, format, argptr);
	va_end(argptr);

	if(simple_logger_fpointer) {
		ffs_fputs(buffer, simple_logger_fpointer);
		ffs_fflush(simple_logger_fpointer);
		busy = 0;
		return 0;
	} else {
		busy = 0;
		return 1;
	}

}

uint8_t simple_logger_log_header(const char *format, ...) {

	if(busy) {
		return 1;
	} else {
		busy = 1;
	}

	if(!simple_logger_file_exists) {
		va_list argptr;
		va_start(argptr, format);
		vsnprintf(buffer, buffer_size, format, argptr);
		va_end(argptr);

		ffs_fputs(buffer, simple_logger_fpointer);
		ffs_fflush(simple_logger_fpointer);
		
		if(simple_logger_fpointer) {
			ffs_fputs(buffer, simple_logger_fpointer);
			ffs_fflush(simple_logger_fpointer);
			busy = 0;
			return 0;
		} else {
			busy = 0;
			return 1;
		}
	}

	busy = 0;
	return 1;
}
