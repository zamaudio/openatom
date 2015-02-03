#ifndef __ARUBA_H
#define __ARUBA_H

#include "linux_glue.h"

#include <stdint.h>
#include <stdbool.h>

enum pll_ids {
	PPLL_1 = 0,
	PPLL_2 = 1,
	PPLL_0 = 2,
	DCPLL = PPLL_0,
};

enum encoder_phy {
	ENCODER_UNIPHY_A,
	ENCODER_UNIPHY_B,
	ENCODER_UNIPHY_C,
	ENCODER_UNIPHY_D,
	ENCODER_UNIPHY_E,
	ENCODER_UNIPHY_F,
	ENCODER_UNIPHY_G,
};

enum transmitter_mode {
	TRANSMITTER_MODE_DP,
	TRANSMITTER_MODE_LVDS,
	TRANSMITTER_MODE_DVI,
	TRANSMITTER_MODE_HDMI,
	TRANSMITTER_MODE_DP_MST,
};

enum XMIT_FLAGS {
	XMIT_FLAG_BLACKMAGIC = (1 << 0),
	XMIT_FLAG_COHERENT = (1 << 1),
};

struct pll_settings {
	uint32_t fb_div_frac;
	uint16_t fb_div_int;
	uint8_t ref_div;
	uint8_t post_div;
};

/* backlight.c */
void aruba_backlight_init(struct radeon_device *rdev, bool controls_edp);
void aruba_backlight_control(struct radeon_device *rdev,
			     uint16_t bl_pwm_freq_hz, uint8_t bl_level);
void aruba_backlight_blon(struct radeon_device *rdev);
void aruba_backlight_bloff(struct radeon_device *rdev);

/* crtc.c */
int aruba_enable_crtc(struct radeon_device *rdev, uint8_t crtc_id, bool enable);
int aruba_update_crtc_x2_buf(struct radeon_device *rdev,
			     uint8_t crtc_id, bool enable);
int aruba_blank_crtc(struct radeon_device *rdev, uint8_t crtc_id, bool enable);
void aruba_set_crtc_dtd_timing(struct radeon_device *rdev, uint8_t crtc_id,
			       struct drm_display_mode *mode);
void aruba_overscan_setup(struct radeon_device *rdev, uint8_t crtc_id,
			  uint8_t h_border, uint8_t v_border);

/* dp_aux.c */
int radeon_read_dpcd(uint8_t bus, uint8_t *dest, uint16_t start, uint16_t len);
int radeon_read_dp_aux_i2c(uint8_t bus, uint8_t addr,
			   uint8_t *dest, uint8_t start, uint16_t len);

/* encoder.c */
void aruba_encoder_setup_dp(struct radeon_device *rdev, uint8_t id,
			    uint16_t pixel_clock_khz, uint8_t lane_num,
			    uint8_t bpc, uint32_t dp_link_rate);
void aruba_encoder_setup_other(struct radeon_device *rdev, uint8_t id,
			       uint8_t mode, uint8_t lane_num);
void aruba_encoder_setup_panel_mode(struct radeon_device *rdev,
				    uint8_t encoder_id, uint8_t panel_mode);
void aruba_encoder_video_on(struct radeon_device *rdev, uint8_t encoder_id);
void aruba_encoder_video_off(struct radeon_device *rdev, uint8_t encoder_id);
void aruba_encoder_link_training_start(struct radeon_device *rdev,
				       uint8_t encoder_id);
void aruba_encoder_link_training_pattern(struct radeon_device *rdev,
					 uint8_t encoder_id, uint8_t pattern);
void aruba_encoder_link_training_finish(struct radeon_device *rdev,
					uint8_t encoder_id);
void aruba_set_encoder_crtc_source(struct radeon_device *rdev,
				   uint8_t crtc_id, uint8_t encoder_id,
				   uint8_t encoder_mode);

/* init_misc.c */
void dynamic_clock_gating_init_hp_1035dx(struct radeon_device *rdev);

/* misc.c */
uint32_t aruba_get_block_offest(uint8_t block_id);
void aruba_powergate_crtc(struct radeon_device *rdev, uint8_t crtc_id,
			  bool enable);
void aruba_scaler_setup(struct radeon_device *rdev, uint8_t crtc_id,
			enum radeon_rmx_type rmx_type);

/* pll.c */
uint8_t aruba_compute_engine_pll(uint32_t vco_freq_mhz, uint32_t *clock_khz);
void aruba_disable_pll_ss(struct radeon_device *rdev, uint8_t pll_id);
void aruba_enable_pll_ss(struct radeon_device *rdev, uint8_t pll_id,
			 uint16_t step, uint16_t ds_frac, uint8_t fbdiv,
			 uint8_t nfrac, bool downspread);
void aruba_set_pixel_clock(struct radeon_device *rdev, enum pll_ids pll_id,
			   struct pll_settings *pll_divs, uint8_t crtc_id,
			   uint32_t px_clk, uint32_t fags, uint8_t encoder_mode);
void aruba_init_pixel_pll(struct radeon_device * rdev, uint8_t pll_id);
int aruba_set_disp_eng_pll(struct radeon_device *rdev, uint32_t vco_freq_mhz,
			   uint32_t clock_khz, uint32_t default_clock_khz,
			   uint16_t pcie_ss_percent);

/* transmitter.c */
void aruba_transmitter_enable(struct radeon_device *rdev, enum encoder_phy ephy,
			      uint8_t num_lanes, uint32_t symbol_clk,
			      enum pll_ids phy_src_pll,
			      enum transmitter_mode phy_mode,
			      uint8_t src_encoder_id,
			      uint8_t hpd_id,
			      uint32_t xmit_flags);
void aruba_transmitter_vsemph(struct radeon_device *rdev, uint8_t phy_id,
			      uint8_t lane_sel);

#endif	/* __ARUBA_H */
