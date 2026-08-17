#include "time.h"
#include "emu/drive_probe.h" // di_wait: the bounded TSTART spin every DI user shares

volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;

int dvd_cover_status() {
  return (dvd[1] & 1); // 0: cover closed, 1: cover opened
}

int dvd_read_id()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = 0;
	dvd[6] = 0x20;
	dvd[7] = 3; // enable reading!
	while (dvd[7] & 1)
    ;
	if (dvd[0] & 0x4)
		return 1;
	return 0;
}

unsigned int dvd_get_error(void)
{
	dvd[2] = 0xE0000000;
	dvd[8] = 0;
	dvd[7] = 1; // IMM
	while (dvd[7] & 1);
	return dvd[8];
}


void dvd_reset()
{
	dvd[1] = 2;
	volatile unsigned long v = *(volatile unsigned long*)0xcc003024;
	*(volatile unsigned long*)0xcc003024 = (v & ~4) | 1;
	udelay(12);
	*(volatile unsigned long*)0xcc003024 = v | 5;
}

void dvd_stop_motor()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xE3000000;  // Stop motor command
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1;  // Execute command

	// Bounded, unlike the plain TSTART spin this was: dvd_custom_bypass_exit() runs this
	// on the way OUT of the disc screen, and on a bus where nothing answers -- a console
	// with no optical drive, or a FlippyDrive in bypass with no drive behind it -- the
	// command never completes. Spinning forever there hangs the console on the one path
	// whose whole job is returning to the menu. A motor that never got the stop command
	// spins itself down; a console that never got the menu back is a power cycle.
	di_wait();
}
