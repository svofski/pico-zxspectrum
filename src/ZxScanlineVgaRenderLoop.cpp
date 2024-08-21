#include "ZxScanlineVgaRenderLoop.h"
#include "ZxSpectrumDisplay.h"
#include "ZxSpectrumPrepareRgbScanline.h"
#include "PicoCharRendererVga.h"
#include "ZxSpectrumPrepareScanvideoScanline.h"
#include "PicoCharRendererScanvideo.h"
#include "hardware/clocks.h"

#ifdef VGA_VSYNC
#define VGA_H_SYNC_POLARITY 1
#else
#define VGA_H_SYNC_POLARITY 5
#endif

const scanvideo_timing_t vga_timing_640x480_60 =
{
    .clock_freq = 25000000,
    .h_active = 640,
    .v_active = 480,

    .h_front_porch = 16,
    .h_pulse = 96,
    .h_total = 800,
    .h_sync_polarity = VGA_H_SYNC_POLARITY, // 1=HSYNC & VSYNC, 5=CSYNC
    .v_front_porch = 10,
    .v_pulse = 2,
    .v_total = 525,
    .v_sync_polarity = 1,

    .enable_clock = 0,
    .clock_polarity = 0,

    .enable_den = 0
};

// https://tomverbeure.github.io/video_timings_calculator
// CEA-861
const scanvideo_timing_t vga_timing_720x576_50 =
{
  .clock_freq = 27000000,

  .h_active = 720,
  .v_active = 576,

  .h_front_porch = 12,
  .h_pulse = 64,
  .h_total = 864,
  .h_sync_polarity = VGA_H_SYNC_POLARITY, // 1=HSYNC & VSYNC, 5=CSYNC

  .v_front_porch = 5,
  .v_pulse = 5,
  .v_total = 625,
  .v_sync_polarity = 1,

  .enable_clock = 0,
  .clock_polarity = 0,

  .enable_den = 0
};

const scanvideo_mode_t vga_mode_640x240_60 =
{
  .default_timing = &vga_timing_640x480_60,
  .pio_program = &video_24mhz_composable,
  .width = 640,
  .height = 240,
  .xscale = 1,
  .yscale = 2,
};

const scanvideo_mode_t vga_mode_720x288_50 =
{
  .default_timing = &vga_timing_720x576_50,
  .pio_program = &video_24mhz_composable,
  .width = 720,
  .height = 576,
  .xscale = 1,
  .yscale = 2,
};

#ifndef VGA_MODE
#define VGA_MODE vga_mode_640x240_60
#endif  

void ZxScanlineVgaRenderLoopInit()
{
  set_sys_clock_khz(VGA_MODE.default_timing->clock_freq / 100, true);
  sleep_ms(10);
}

void __not_in_flash_func(ZxScanlineVgaRenderLoop)(
  ZxSpectrum &zxSpectrum, 
  volatile uint &frames,
  bool &showMenu,
  bool &toggleMenu
) {
  scanvideo_setup(&VGA_MODE);
  scanvideo_timing_enable(true);

  unsigned char* screenPtr = zxSpectrum.screenPtr();
  unsigned char* attrPtr = screenPtr + (32 * 24 * 8);

  while (true) {
    struct scanvideo_scanline_buffer *scanline_buffer = scanvideo_begin_scanline_generation(true);
    uint32_t frame_num = scanvideo_frame_number(scanline_buffer->scanline_id);
    uint32_t y = scanvideo_scanline_number(scanline_buffer->scanline_id);

    const uint32_t fpf = zxSpectrum.flipsPerFrame();
    const bool ringoMode = fpf > 46 && fpf < 52;
    if (ringoMode) {
      screenPtr = zxSpectrum.memPtr(y & 4 ? 7 : 5);
      attrPtr = screenPtr + (32 * 24 * 8);
    }
    const uint32_t blankTopLines = (DISPLAY_BLANK_LINES/2);

    if (y == blankTopLines) {
      
      if (!ringoMode) {
        screenPtr = zxSpectrum.screenPtr();
        attrPtr = screenPtr + (32 * 24 * 8);
      }
      
      if (toggleMenu) {
        showMenu = !showMenu;
        toggleMenu = false;
        ZxScanlineVgaRenderLoopCallbackMenu(showMenu);
      }
      
      frames = frame_num;
    }

    if (y < blankTopLines || y >= (blankTopLines + ZX_SPECTRUM_SCREEN_HEIGHT)) {
      zx_prepare_scanvideo_blankline(
        scanline_buffer
      );
    }
    else if (showMenu) {
      pcw_prepare_scanvideo_scanline_80(
        scanline_buffer,
        y - blankTopLines,
        frame_num);
    }
    else { 
      zx_prepare_scanvideo_scanline(
        scanline_buffer, 
        y - blankTopLines, 
        frame_num,
        screenPtr,
        attrPtr,
        zxSpectrum.borderColour(y - blankTopLines)
      );
    }

    // Release the rendered buffer into the wild
    scanvideo_end_scanline_generation(scanline_buffer);

    ZxScanlineVgaRenderLoopCallbackLine(y);
  }

  __builtin_unreachable();
}