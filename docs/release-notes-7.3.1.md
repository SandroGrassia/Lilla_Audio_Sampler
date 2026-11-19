# Firmware 7.3.1 - Live Sampler display updates

## Changes

- Update only changed portions of the Live Sampler waveform canvas to reduce unnecessary display transfers and address flicker.
- Restore the complete canvas after page redraws and confirmation popups.
- Avoid redrawing the MONO/LEFT/RIGHT label when its value has not changed.
- Refresh the README and illustrated User Guide for firmware 7.3.1.
- Include the existing website-reference and hardware-comment corrections.

The display comparison copy uses 60,140 bytes of previously free PSRAM. Recording capacity, audio buffer sizes and the 200 ms display refresh interval are unchanged by this update.

## Compatibility

The layered MIDI Loop format introduced in 7.3.0 is retained. Loop files saved before 7.3.0 remain incompatible.

## Validation

The PlatformIO `teensy41` build passed. The comparison buffer was verified in PSRAM, and the exported firmware was checked for integrity. The regenerated PDF was visually reviewed. The reduction in flicker still requires confirmation on Speed_shift hardware.

## Installation

Upload `Lilla_v7_3_1.hex` using Teensy Loader, or build and upload the `teensy41` environment from VSCode. See the User Guide for the complete procedure.
