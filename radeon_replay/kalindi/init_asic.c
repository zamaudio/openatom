#include "kalindi/kalindi.h"
#include "kalindi/cikd.h"

#define VGA_MEMORY_BASE_ADDRESS               0x310
#define VGA_MEMORY_BASE_ADDRESS_HIGH          0x324

extern const uint32_t kalindi_on_a68n_5200_vco_freq;

// command_table  0000c394  #47  (ClockSource):
static void clock_source(struct radeon_device *rdev)
{
	radeon_mask(rdev, 0x82 << 2, 0, 0xc0200000);
	radeon_mask(rdev, 0x83 << 2, 0xff03ffff, 0);
}

// command_table  0000bc92  #02  (ASIC_RegistersInit):
static void asic_registers_init(struct radeon_device *rdev)
{
	radeon_write(rdev, HDP_HOST_PATH_CNTL, 0x0f200029);
	radeon_write(rdev, 0x2f30, 0x00000001);
	radeon_mask(rdev, HDP_MISC_CNTL, 0x1fffff, 0x00321be0);
	//   0025: SET_DATA_BLOCK  1e  (IntegratedSystemInfo)
	//   0027: MOVE   WS_REMIND/HI32 [...X]  <-  data[0038] [...X]
	//   002c: AND    WS_REMIND/HI32 [...X]  <-  3c
	//   0030: MASK   reg[1847]  [...X]  &  c3  |  WS_REMIND/HI32 [...X]
	// FIXME: GPU (un)capability bits ulGPUCapInfo 
}


// command_table  0000bd58  #05  (MemoryControllerInit):
static void memory_init(struct radeon_device *rdev)
{
	uint16_t reg16;
	radeon_mask(rdev, BIF_FB_EN, 0xff, 0);
	reg16 = radeon_read(rdev, MC_VM_FB_LOCATION);
	radeon_write(rdev, HDP_NONSURFACE_BASE, reg16 << 16);
	radeon_write(rdev, VGA_MEMORY_BASE_ADDRESS_HIGH, reg16 >> 8);
	radeon_write(rdev, VGA_MEMORY_BASE_ADDRESS, reg16 << 24);
	radeon_mask(rdev, MC_SHARED_BLACKOUT_CNTL, 0xff, 0);
	radeon_mask(rdev, BIF_FB_EN, 0, FB_READ_EN | FB_WRITE_EN);
}

// command_table  0000be8a  #0a  (SetEngineClock):
static void set_engine_clock(struct radeon_device *rdev, uint32_t clock_khz)
{
	uint8_t pll;
	pll = kalindi_compute_engine_pll(kalindi_on_a68n_5200_vco_freq, &clock_khz);  // vco_freq=0x02 ?
	do {
		radeon_write(rdev, 0x82 << 2, 0xc0500090);
	} while (!(radeon_read(rdev, 0x83 << 2) & 1));

	radeon_write(rdev, 0x82 << 2, 0xc050008c);
	radeon_mask(rdev, 0x83 << 2, 0xff, pll);

	do {
		radeon_write(rdev, 0x82 << 2, 0xc0500090);
	} while (!(radeon_read(rdev, 0x83 << 2) & 1));
}

// command_table  0000b856  #00  (ASIC_Init):
void kalindi_asic_init(struct radeon_device *rdev)
{
	uint8_t crtc_id;
	clock_source(rdev);

	fprintf(stderr, "\t/* asic_registers_init */\n");
	asic_registers_init(rdev);
	//dynamic_clock_gating_init_a68n_5200(rdev); XXX probably from crtc functions 

	for (crtc_id = 0; crtc_id < 2; crtc_id++) {
		kalindi_powergate_crtc(rdev, 0, crtc_id);
		kalindi_enable_crtc(rdev, crtc_id, 0);
	}
	udelay(2);

	memory_init(rdev);

	fprintf(stderr, "\t/* set_engine_clock */\n");
	set_engine_clock(rdev, 200000);

	//dig2encoder_control(rdev);
	radeon_mask(rdev, 0x1740, 0, BIT(1));
	udelay(1000);
	fprintf(stderr, "\t/* asic_init done */\n");
}
