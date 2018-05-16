//#include "evergreend.h"
#include "kalindi/cikd.h"
#include "kalindi/kalindi.h"
#include "external_encoders/external_encoder.h"
#include "radeon_util.h"
#include "radeon_init_native.h"
#include "linux_glue.h"	// For kalindi_ IO accessor ONLY!!!!!!!!!
// And we also fucking cheat and use the drm_display_mode as well */
#include "ObjectID.h"
#include "drm_dp_helper.h"
#include "edid.h"

/*
 * Use this fucking regexp to clean tables
 *
 * ([0-9a-f]{4}: )[0-9a-f ]*
 * \1
 */

#define VGA_MEMORY_BASE_ADDRESS               0x310
#define VGA_MEMORY_BASE_ADDRESS_HIGH          0x324

#define TIMED_OUT(str) \
	do {						\
		DRM_ERROR("Timed out on %s", str);	\
		return;					\
	} while(0)

static const struct big_ass_init_constants {
	uint32_t vco_freq_mhz;
	uint32_t default_clock_khz;
	uint16_t pcie_ss_percent;
} gpu_init_stuff = {
	.vco_freq_mhz = 3600,
	.default_clock_khz = 20000,
	.pcie_ss_percent = 36,
};

static const struct big_ass_init_constants *gpu = & gpu_init_stuff;
const uint32_t kalindi_on_hp_1035dx_vco_freq;

// command_table  0000e3d0  #38  (ReadHWAssistedI2CStatus/HPDInterruptService):
//
//   Size         005f
//   Format Rev.  02
//   Param Rev.   00
//   Content Rev. 02
//   Attributes:  Work space size        00 longs
//                Parameter space size   01 longs
//                Table update indicator 0
//
uint8_t kalindi_hpd_interrupt_status(struct radeon_device *rdev, uint8_t hpd_id)
{
	uint8_t status;
	uint32_t off;
	//   0006: 370000            SET_ATI_PORT  0000  (INDIRECT_IO_MM)
	//   0009: 3d650001          COMP   param[00]  [..X.]  <-  01
	//   000d: 445400            JUMP_Equal  0054
		/* Should have called kalindi_sw_interrupt() */
	//   0010: 553000            CLEAR  param[00]  [.X..]
	//   0013: 3d250006          COMP   param[00]  [...X]  <-  06
	//   0017: 465000            JUMP_Above  0050
	if (hpd_id > 5)
		return 0;
	//   001a: 32250001          SUB    param[00]  [...X]  <-  01
	//   001e: 20250003          MUL    param[00]  [...X]  <-  03
	//   0022: 560848            CLEAR  WS_REGPTR [..XX]
	//   0025: 03224840          MOVE   WS_REGPTR [...X]  <-  WS_QUOT/LOW32 [...X]
	off = hpd_id * 0xc;
	//   0029: 0da5081801        OR     reg[1808]  [.X..]  <-  01
	radeon_mask(rdev, DC_HPD1_INT_CONTROL + off, 0, DC_HPDx_INT_EN);
	//   002e: 02a0000718        MOVE   param[00]  [.X..]  <-  reg[1807]  [...X]
	//   0033: 08a50003          AND    param[00]  [.X..]  <-  03
	status = radeon_read(rdev, DC_HPD1_INT_STATUS + off) & 0x3;
	//   0037: 4a25071802        TEST   reg[1807]  [...X]  <-  02
	//   003c: 444a00            JUMP_Equal  004a
	if (status & DC_HPDx_SENSE)
		//   003f: 5c0d0818fffe0100  MASK   reg[1808]  [..XX]  &  feff  |  0001
		radeon_mask(rdev, DC_HPD1_INT_CONTROL + off, DC_HPDx_INT_POLARITY, DC_HPDx_INT_ACK);
		//   0047: 435000            JUMP   0050
	else
		//   004a: 0d0d08180101      OR     reg[1808]  [..XX]  <-  0101
		radeon_mask(rdev, DC_HPD1_INT_CONTROL + off, 0, DC_HPDx_INT_POLARITY | DC_HPDx_INT_ACK);
	//   0050: 3a0000            SET_REG_BLOCK  0000
	//   0053: 5b                EOT
	return status;
}
void kalindi_sw_interrupt(struct radeon_device *rdev)
{
	//   0054: 0d65cf0508        OR     reg[05cf]  [..X.]  <-  08
	/* ^^^ That's a scratch register. Fuck it. ^^^ */
	//   0059: 0d65131401        OR     reg[1413]  [..X.]  <-  01
	radeon_mask(rdev, 0x504c, 0, BIT(0));
	//   005e: 5b                EOT
}

void set_video_mode_motherfucker(struct radeon_device *rdev, struct edid *edid);
/*
 * Yes. This is it. The master plan. The master function. This is where we run
 * through all the hoops and loops and actually initialize the display, not just
 * GPU. This is the culminating event of all that bullshit work
 */
void execute_master_plan(struct radeon_device * rdev)
{
	int ret;
	uint8_t edid_raw[256];
	struct edid edid;
	if (rdev == NULL)
		rdev = my_aux.parent;

	//kalindi_asic_init(rdev);

	radeon_read(rdev, CONFIG_MEMSIZE);

	/* > Some scratch reg shit we're skipping <*/

	/* This bit is part of UNIPHY transmitter init. What does it do? */
	fprintf(stderr, "\t/* transmitter_init */\n");
	kalindi_backlight_init(rdev, CONNECTOR_OBJECT_ID_LVDS);

	fprintf(stderr, "\t/* hpd_interrupt_status */\n");
	kalindi_hpd_interrupt_status(rdev, 0);

	fprintf(stderr, "\t/* EDID read */\n");
	ret = radeon_read_dp_aux_i2c(0, 0x50, edid_raw, 0, 128);
	if (ret < 0) {
		printf("Sorry. Could not get an EDID. Aborting this shit\n");
		return;
	}

	ret = decode_edid(edid_raw, sizeof(edid_raw), &edid);
	if (ret < 0) {
		printf("Something's wrong with the EDID. farting...\n");
		return;
	}

	struct radeon_panel_timings timings_nowait = {
		.digon_to_de_ms = 0,
		.de_to_vary_bl_ms = 0,
		.vary_bl_to_blon_ms = 0,
		.de_to_digon_ms = 0,
		.vary_bl_to_de_ms = 0,
		.blon_to_vary_bl_ms = 0,
		.off_delay_ms = 0,
		.flags = PANEL_6_BPC,
	};
	struct radeon_panel_timings timings_real = {
		.digon_to_de_ms = 28,
		.de_to_vary_bl_ms = 360,
		.vary_bl_to_blon_ms = 12,
		.de_to_digon_ms = 28,
		.vary_bl_to_de_ms = 360,
		.blon_to_vary_bl_ms = 12,
		.off_delay_ms = 500,
		.flags = PANEL_6_BPC,
	};
	struct radeon_panel_timings *t;
	t = (1) ? &timings_real : &timings_nowait;
	//kalindi_lcd_bloff(rdev);
	fprintf(stderr, "\t/* brightness_control */\n");
	kalindi_backlight_control(rdev, 200, 255);
	fprintf(stderr, "\t/* brightness_control done */\n");
	fprintf(stderr, "\t/* travis_init */\n");
	travis_init(rdev, t);
	fprintf(stderr, "\t/* travis should now work */\n");

	//vga_pre_c1();

	set_video_mode_motherfucker(rdev, &edid);
}

static void edid_to_mode(struct drm_display_mode *mode, struct edid *edid)
{
	printf( "pclk %d\n"
		"x_mm %d\n"
		"y_mm %d\n"
		"ha!  %d\n"
		"hbl  %d\n"
		"hso  %d\n"
		"hspw %d\n"
		"hbo  %d\n"
		"va!  %d\n"
		"vbl  %d\n"
		"vso  %d\n"
		"vspw %d\n"
		"vbo  %d\n",
		edid->pixel_clock,
		edid->x_mm,
		edid->y_mm,
		edid->ha,
		edid->hbl,
		edid->hso,
		edid->hspw,
		edid->hborder,
		edid->va,
		edid->vbl,
		edid->vso,
		edid->vspw,
		edid->vborder);

	mode->crtc_clock = edid->pixel_clock;
	mode->crtc_hdisplay = edid->ha;
	mode->crtc_hblank_start = edid->ha;
	mode->crtc_hblank_end = edid->ha + edid->hbl;

	mode->crtc_hsync_start = edid->ha + edid->hso;
	mode->crtc_hsync_end = edid->ha + edid->hso + edid->hspw;
	mode->crtc_htotal = edid->ha + edid->hbl;
	mode->crtc_hskew = 0;

	mode->crtc_vdisplay = edid->va;
	mode->crtc_vblank_start = edid->va;
	mode->crtc_vblank_end = edid->va + edid->vbl;
	mode->crtc_vsync_start = edid->va + edid->vso;

	mode->crtc_vsync_end = edid->va + edid->vso + edid->vspw;
	mode->crtc_vtotal = edid->va + edid->vbl;

	mode->flags = 0;
	mode->flags |= (edid->pvsync == '-') ? DRM_MODE_FLAG_NVSYNC : 0;
	mode->flags |= (edid->phsync == '-') ? DRM_MODE_FLAG_NHSYNC : 0;

	printf( "Detailed mode (IN FUCKING DECIMAL): Clock %d KHz,\n"
		"               %4d %4d %4d %4d hborder %x\n"
		"               %4d %4d %4d %4d vborder %x\n"
		"               %chsync %cvsync%s\n",
		edid->pixel_clock,
		mode->crtc_hdisplay,
		mode->crtc_hsync_start,
		mode->crtc_hsync_end,
		mode->crtc_htotal,
		edid->hborder,
		mode->crtc_vdisplay,
		mode->crtc_vsync_start,
		mode->crtc_vsync_end,
		mode->crtc_vtotal,
		edid->vborder,
		edid->phsync, edid->pvsync,
		edid->syncmethod
	);

	printf( "The wrong shit that the EDID handler decided for us\n"
		"  screen must be %dx%d\n"
		"  you must have %d bits per pixel\n"
		"  and the framenuffer stride must be %d bytes\n",
		edid->x_resolution, edid->y_resolution,
		edid->framebuffer_bits_per_pixel,
		edid->bytes_per_line
	);

}

extern void kalindi_fuck_my_lute(struct radeon_device *rdev, uint8_t lut_id);
void kalindi_enable_grph_srfc(struct radeon_device *rdev, uint8_t surf, uint8_t enable,
			    uint16_t h, uint16_t w, uint16_t pizditch);

static const uint8_t my_bpc = 6;
static const uint8_t my_crtc = 0;
static const uint8_t my_enc = 4;
static const uint8_t my_pll = 2;
static const uint8_t my_hpd = 0 + 1;
static const uint8_t my_phy = 4;
static const uint8_t my_lane_count = 1;

static void xmit_drive(struct radeon_transmitter *self,
		       uint8_t *drive, uint8_t num_lanes)
{
	kalindi_transmitter_vsemph(self->parent, self->id, drive[0]);
}

static void enc_pattern(struct radeon_encoder *self, uint8_t pattern)
{
	if (pattern == DP_TRAINING_PATTERN_DISABLE) {
		kalindi_encoder_link_training_finish(self->parent, self->id);
		return;
	}

	kalindi_encoder_link_training_start(self->parent, self->id);
	kalindi_encoder_link_training_pattern(self->parent, self->id, pattern);
}

void set_video_mode_motherfucker(struct radeon_device *rdev, struct edid *edid)
{
	int err;
	struct drm_display_mode mode;
	uint8_t dpcd[DP_RECEIVER_CAP_SIZE];
	const uint32_t dp_rate = 270000;

	memset(&mode, 0, sizeof(mode));

	err = drm_dp_dpcd_read(&my_aux, DP_DPCD_REV, dpcd, sizeof(dpcd));
	if (err < 0) {
		DRM_ERROR("Could not probe DP link via AUX. Abortioning");
		return;
	}

	fprintf(stderr, "void replay_int10_c3(void)\t// vbe_set_mode()\n{\n");
	//c3_mambojumbo();
	fprintf(stderr, "\t/* blank_crtc */\n");
	if (kalindi_blank_crtc(rdev, my_crtc, true) < 0)
		TIMED_OUT("crtc blanking 1");
	fprintf(stderr, "\t/* set_disp_eng_pll */\n");
	if (kalindi_set_disp_eng_pll(rdev, gpu->vco_freq_mhz, 800000,
				   gpu->default_clock_khz, gpu->pcie_ss_percent))
		TIMED_OUT("engine PLL");
	fprintf(stderr, "\t/* update_crtc_x2_buf */\n");
	if (kalindi_update_crtc_x2_buf(rdev, my_crtc, true))
		TIMED_OUT("x2 buf");
	fprintf(stderr, "\t/* disable_grph_srfc */\n");
	kalindi_disable_grph_srfc(rdev, 0);

	fprintf(stderr, "\t/* init_pixel_pll */\n");
	kalindi_init_pixel_pll(rdev, my_pll);
	//fprintf(stderr, "\t/* fuck_with_pll stage 2 */\n");
	//kalindi_more_init_pixel_pll(rdev, my_pll);
	fprintf(stderr, "\t/* disable_pll__ss */\n");
	kalindi_disable_pll_ss(rdev, my_pll);
	fprintf(stderr, "\t/* set_crtc_dtd_timing */\n");
	edid_to_mode(&mode, edid);
	kalindi_set_crtc_dtd_timing(rdev, my_crtc, &mode);
	fprintf(stderr, "\t/* enable_scaler */\n");
	kalindi_scaler_setup(rdev, my_crtc, RMX_ASPECT);
	fprintf(stderr, "\t/* enable_grph_srfc */\n");
	kalindi_enable_grph_srfc(rdev, 0, true, mode.crtc_vdisplay, mode.crtc_hdisplay, mode.crtc_hdisplay);
	fprintf(stderr, "\t/* lut_setup */\n");
	kalindi_fuck_my_lute(rdev, 0);
	fprintf(stderr, "\t/* enable_grph_srfc_more */\n");
	//kalindi_enable_grph_srfc(rdev, 1, true, mode.crtc_vdisplay, mode.crtc_hdisplay, mode.crtc_hdisplay);
	fprintf(stderr, "\t/* lut_setup_more */\n");
	kalindi_fuck_my_lute(rdev, 1);
	//kalindi_mcleanup(rdev, 0, 200);
	fprintf(stderr, "\t/* enable_crtc */\n");
	if (kalindi_enable_crtc(rdev, my_crtc, true))
		TIMED_OUT("crtc diable");
	fprintf(stderr, "\t/* update_crtc_x2_buf */\n");
	if (kalindi_update_crtc_x2_buf(rdev, my_crtc, false))
		TIMED_OUT("c2 buf again");
	fprintf(stderr, "\t/* set_encoder_crtc_source */\n");
	kalindi_set_encoder_crtc_source(rdev, my_crtc, 0xc, 1);
	kalindi_encoder_video_off(rdev, my_enc);	// <- VBIOS doesn't do this
	fprintf(stderr, "\t/* encoder_setup_dp */\n");
	kalindi_encoder_setup_dp(rdev, my_enc, edid->pixel_clock, 1, my_bpc, dp_rate);
	fprintf(stderr, "\t/* encoder_setup_panel_mode */\n");
	kalindi_encoder_setup_panel_mode(rdev, my_enc, 0x1);

	fprintf(stderr, "\t/* transmitter_enable */\n");
	kalindi_transmitter_enable(rdev, my_phy, my_lane_count, dp_rate, my_pll,
			       0, 0x10, my_hpd + 1, 0);



	struct radeon_encoder enc = {
		.id = my_enc,
		.pattern3_supported = true,
		.parent = rdev,
		.set_dp_training_pattern = &enc_pattern,
	};
	struct radeon_transmitter xmit = {
		.id = my_phy,
		.parent = rdev,
		.max_link_rate = 540000,
		.update_drive_strength = &xmit_drive,
	};
	fprintf(stderr, "\t/* quick_link_training */\n");
	radeon_dp_link_train(rdev, &enc, &xmit, dpcd);

	/* OK, this is it. We should  have something by now */
	kalindi_encoder_video_on(rdev, my_enc);

	//kalindi_lcd_blon(rdev);
	/* extrenal encoder video enable */
	radeon_mask(rdev, 0x6464, 0, 3 << 24);

	if (kalindi_blank_crtc(rdev, 0, false))
		TIMED_OUT("when we were about to enable CRTC");
}
