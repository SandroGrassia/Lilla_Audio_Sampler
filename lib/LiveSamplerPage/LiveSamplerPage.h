/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

// Return false when capture processing must end the current loop iteration.
bool Handle_Live_sampler(void);
void Golive_with_LIVE_SAMPLING(void); // Enter Live Sampler and initialize the page controls.
void LS_refresh_LS_page(void); // Redraw the Live Sampler page and discard previous notices.
