# LILLA Audio Sampler 7.2.10

## Smoother Live Sampler playback while recording

This release reduces clicks and rough transitions when the Live Sampler playback position meets the recording position.

- Safer forward-playback starts avoid problematic read/write overlap, with protections for both FIXED and moving start points.
- Above the original pitch, short fade-out/fade-in transitions and a saved forward restart position smooth crossings of the recording boundary.
- Below the original pitch, the player anticipates the next overlap and fades the current safe block before restarting farther back. The complete interpolation window is kept outside the latest written area during these transitions.
- Pitch changes are deferred until the paired fade-in is complete, keeping the transition consistent.
- Loop modes are available after recording stops, avoiding repeated read/write collisions during live recording.
