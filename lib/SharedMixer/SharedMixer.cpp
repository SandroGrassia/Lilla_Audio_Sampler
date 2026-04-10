/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedMixer.h"

uint8_t MX_routing_source[MX_sources] = {3, 3, 3, 3, 3, 3, 3, 3, 3}; // 0-->7: Sound 8: InputDevice; 1 --> source routed to PWM output (monitor); 2 --> source routed to Audio Board output;  3 --> source routed to both
bool MX_mute[MX_sources] = {false, false, false, false, false, false, false, false, false};