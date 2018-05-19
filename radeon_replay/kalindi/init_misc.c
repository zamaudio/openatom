/*
 * Miscellaneous init helpers that are used by asic_init only.
 */
#include "kalindi.h"
#include "linux_glue.h"

void dynamic_clock_gating_init_a68n_5200(struct radeon_device *rdev)
{
	radeon_write(rdev, 0x0c68, 0x06186186);
	radeon_write(rdev, 0x0c74, 0x06186186);
	radeon_write(rdev, 0x0c44, 0x002bc006);
	radeon_mask(rdev, 0x0bd8, 0x6000f700, 0x2000f404);
	radeon_write(rdev, 0x0be4, 0x00025161);
	radeon_write(rdev, 0x0c50, 0x10009000);
	radeon_mask(rdev, 0x0be0, 0xff0000, 0x40000);
	radeon_mask(rdev, 0x5c1c, BIT(12), BIT(11));	/* PLL WARNING!!! */
	radeon_mask(rdev, 0x5c6c, BIT(12), BIT(11));	/* PLL WARNING!!! */
	radeon_mask(rdev, 0x5c90, 0, BIT(0));
	radeon_write(rdev, 0x04ec, 0x00120230);
	radeon_write(rdev, 0x04c0, 0x0010bb80);
	radeon_mask(rdev, 0x5ee8, 0xffff << 16, 0x0030 << 16);
	radeon_mask(rdev, 0x5ee4, 0xff, 1);
	radeon_write(rdev, 0x04c4, 0x0801003c);
	radeon_mask(rdev, 0x0350, 0xff << 8, 2 << 8);
	radeon_mask(rdev, 0x03f0, 0xffff, 0x0100);
	radeon_write(rdev, 0x0370, 0x00190013);
	radeon_mask(rdev, 0x0380, 0xff, 0x2f);
	radeon_mask(rdev, 0x0360, 0xffff, 1);
	radeon_write(rdev, 0x0390, 0x9d1eb744);
	radeon_write(rdev, 0x0374, 0x00190012);
	radeon_mask(rdev, 0x0384, 0xff, 0x2a);
	radeon_mask(rdev, 0x0364, 0xffff, 1);
	radeon_write(rdev, 0x0394, 0x9d1eb744);
	radeon_write(rdev, 0x6428, 0x0000000f);
	radeon_write(rdev, 0x60e0, 0x30000000);
	radeon_mask(rdev, 0x5de0, 0xff, 0);
	/* Just continue if this step times out. Nothing we can do about it. */
	radeon_wait_clear(rdev, 0x5df8, BIT(0), 10);
	radeon_write(rdev, 0x5df4, 0x00002000);
	radeon_write(rdev, 0x5df0, 0x100010ff);
}
