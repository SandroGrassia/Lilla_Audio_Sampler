# LILLA Audio Sampler 7.2.9

## Switchable pitch antialiasing

In Performance, select an Instrument's **TONE** field, choose a cutoff from **3.0 to 8.0 kHz**, then press **Value** to switch antialiasing on or off.

- The **A** suffix (for example, `5.0A`) indicates that antialiasing is enabled.
- Filtering acts above pitch **1** for base pitches up to and including **2** (+12 semitones, one octave). At pitch 1 the filter is bypassed.
- Base pitch includes note transposition and tuning. Pitch bend and vibrato do not change eligibility at the upper limit.
- With A off, the TONE timbre low-pass filter remains active. TONE **OFF** bypasses both upward-pitch filters and clears the antialiasing option; selecting a cutoff again leaves the option off until Value is pressed.
- **Save the patch** to retain each Instrument's setting. Existing patches without the option load with antialiasing off.

Low-pitch anti-imaging remains automatic and independent of TONE.

See the [User Guide](https://github.com/SandroGrassia/Lilla_Audio_Sampler/blob/v7.2.9/User%20Guide.md#tone-consistent-timbre-when-transposing-upwards) for operating instructions. The release includes the compiled Teensy 4.1 firmware and updated printable guide.
