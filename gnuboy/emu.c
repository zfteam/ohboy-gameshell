#include "gnuboy.h"
#include "defs.h"
#include "regs.h"
#include "hw.h"
#include "cpu.h"
#include "sound.h"
#include "mem.h"
#include "lcd.h"
#include "rtc.h"
#include "rc.h"


static int framelen = 16743;
static int framecount;

rcvar_t emu_exports[] =
{
	RCV_INT("framelen", &framelen),
	RCV_INT("framecount", &framecount),
	RCV_END
};







void emu_init()
{
	
}


/*
 * emu_reset is called to initialize the state of the emulated
 * system. It should set cpu registers, hardware registers, etc. to
 * their appropriate values at powerup time.
 */

void emu_reset()
{
	hw_reset();
	lcd_reset();
	cpu_reset();
	mbc_reset();
	sound_reset();
	
	lcdc_trans(); /* Set lcdc ahead */
}




/* emu_step()
	make CPU catch up with LCDC
*/
void emu_step()
{
	cpu_emulate(cpu.lcdc);
}


/*
	Time intervals throughout the code, unless otherwise noted, are
	specified in double-speed machine cycles (2MHz), each unit
	roughly corresponds to 0.477us.
	
	For CPU each cycle takes 2dsc (0.954us) in single-speed mode
	and 1dsc (0.477us) in double speed mode.
	
	Although hardware gbc LCDC would operate at completely different
	and fixed frequency, for emulation purposes timings for it are
	also specified in double-speed cycles.
	
	line = 228 dsc (109us)
	frame (154 lines) = 35112 dsc (16.7ms)
	of which
		visible lines x144 = 32832 dsc (15.66ms)
		vblank lines x10 = 2280 dsc (1.08ms)
*/
void emu_run()
{
	void *timer = sys_timer();
	int delay;

	vid_begin();
	//lcd_begin();
	for (;;)
	{
		/* FRAME BEGIN */
		
		while (R_LY < 144)
		{
			/* Step through visible line scanning phase */
			emu_step();
		}
		
		/* VBLANK BEGIN */
		
		vid_end();
		rtc_tick();
		sound_mix();
		/* pcm_submit() introduces delay, if it fails we use
		sys_sleep() instead */
		if (!pcm_submit())
		{
			delay = framelen - sys_elapsed(timer);
			sys_sleep(delay);
			sys_elapsed(timer);
		}
		doevents();
		vid_begin();
		if (framecount) { if (!--framecount) die("finished\n"); }
		
		while (R_LY > 0) {
			/* Step through vblank phase */
			emu_step();
		}
		/* VBLANK END */
		/* FRAME END */
	}
}
