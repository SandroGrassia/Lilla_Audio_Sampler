/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 */

#pragma once

// Return false when a recording preparation failure must end the current loop iteration.
bool Handle_Direct_sampler(bool &recording_limit_notified);
void Golive_DIRECT_SAMPLING(void); // Enter and initialize the Direct Sampler page.
