"""Run host regressions against production Delay code using minimal hardware stubs."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def source(path):  # Read production code without firmware-only includes.
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', (ROOT / path).read_text(encoding='utf-8'), flags=re.M)

def method(code, signature):  # Extract one complete production method for the hardware-free fixture.
    match = re.search(r'^' + re.escape(signature) + r'[^;\n]*\n\{.*?^}', code, re.M | re.S)
    if not match:
        raise RuntimeError('Missing production method: ' + signature)
    return match.group(0)

prefix = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
#define constrain(x,lo,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))
#define bitRead(x,n) (((x)>>(n))&1)
#define FLASHMEM
#define F(x) x
constexpr int INSTRUMENTS=8;
constexpr int AUDIO_BLOCK_SAMPLES=128;
constexpr float AUDIO_SAMPLE_RATE=44100.0f;
constexpr int DELAY_CACHE_CHANNEL_SAMPLES=4096;
struct SerialStub { template<class T> void print(T) {} template<class T> void println(T) {} void println(const char *text) { std::puts(text); } void println() {} } Serial;
struct audio_block_t { int16_t data[AUDIO_BLOCK_SAMPLES]={}; };
class AudioStream {
public:
 bool fail=false;
 int releases=0, transmissions=0;
 audio_block_t output, input[2], *queued[2]={};
 AudioStream(int, audio_block_t **) {}
 audio_block_t *allocate() { return fail?nullptr:&output; } // Simulate audio memory exhaustion.
 audio_block_t *receiveReadOnly(int channel) { auto *p=queued[channel]; queued[channel]=nullptr; return p; }
 void release(audio_block_t *) { ++releases; }
 void transmit(audio_block_t *) { ++transmissions; }
};
class WaveLFO {
public:
 int16_t block[AUDIO_BLOCK_SAMPLES]={};
 float frequency=0;
 int phase=0;
 void Update() {} // Keep modulation deterministic.
 void Set_frequency(float value) { frequency=value; }
 void Set_phase(int value) { phase=value; }
};
class PlayersManager {
public:
 std::vector<int> routes;
 void MX_multicast_change_routing(int instrument) { routes.push_back(instrument); } // Record instrument indices.
};
int16_t Lilla_saturate16(int value) { return constrain(value,-32768,32767); }
'''
size_definitions = '\n'.join(line for line in (ROOT / 'lib/SharedElements/SharedElements.h').read_text(encoding='utf-8').splitlines() if line.startswith('static constexpr int DELAY_CACHE_SECONDS =') or line.startswith('static constexpr int DELAY_CACHE_CHANNEL_SAMPLES ='))
prefix = prefix.replace('constexpr int DELAY_CACHE_CHANNEL_SAMPLES=4096;', size_definitions)
elements = (ROOT / 'lib/SharedElements/SharedElements.h').read_text(encoding='utf-8')
prefix += re.search(r'static constexpr int LPF_MAX = \d+;', elements).group(0) + '\n'
prefix += re.search(r'constexpr int lowpass_value\[.*?};', elements, re.S).group(0) + '\n'
prefix += '''
class AudioFilterBiquad {
public:
    float frequency = 20000;
    double q = 0;
    int writes = 0;
    void setLowpass(int stage, float value, double resonance)
    {
        assert(stage == 0);
        frequency = value;
        q = resonance;
        ++writes;
    }
};
'''
prefix += 'class LillaReverb { public: void Set_parameters(int, int, int) {} };\n'
prefix += 'class LillaCompressor { public: static constexpr int THRESHOLD_OFF = -2; int threshold = -6; void Set_threshold(int db) { threshold = db; enabled = db != THRESHOLD_OFF; } bool enabled = false; void Set_enabled(bool value) { enabled = value; } };\n'
shared = source('lib/SharedDelay/SharedDelay.h') + source('lib/SharedDelay/SharedDelay.cpp')
gain_header = source('lib/AudioGain/AudioGain.h').replace('private:', 'public:')
gain_cpp = source('lib/AudioGain/AudioGain.cpp')
gain = gain_header + '\nvoid AudioGain::update() {}\n' + '\n'.join(method(gain_cpp, 'void AudioGain::' + name) for name in ['Set_gain(', 'Mute(', 'Unmute(', 'Get_mults('])
stereo = source('lib/StereoDelay/StereoDelay.h').replace('private:', 'public:') + source('lib/StereoDelay/StereoDelay.cpp')
manager = source('lib/DelayManager/DelayManager.h').replace('private:', 'public:') + source('lib/DelayManager/DelayManager.cpp')
main_source = (ROOT / 'src/main.cpp').read_text(encoding='utf-8')
controls = r'''
#include <type_traits>
constexpr int EN_PB_Value = 0;
bool value_pressed = false;
bool Read_pushbutton(int)
{
    const bool pressed = value_pressed;
    value_pressed = false;
    return pressed;
}
struct EncoderStub
{
    int rotation = 0;
    int Get_rotation(int)
    {
        const int result = rotation;
        rotation = 0;
        return result;
    }
} Encoders_manager;
DelayManager Delay_manager;
void D_Set_value(int item, int value)
{
    Delay_manager.Set_value(item, value);
}
'''
controls += 'template <class T>\n' + method(main_source, 'bool Read_encoder(') + '\n'
controls += 'template <class T>\n' + method(main_source, 'bool Read_encoder_inverse(') + '\n'
controls += method(main_source[main_source.index('bool D_Read_value(int item) // Edit'):], 'bool D_Read_value(int item)') + '\n'
manager += controls

tests = r'''
void DrainGain(AudioGain &gain) { int count=0; while(gain.gain_flag) { gain.Get_mults(); assert(++count<10000); } } // Finish the real gain ramp.
int main() {
 StereoDelay left,right;
 std::vector<int16_t> fifo(DELAY_CACHE_CHANNEL_SAMPLES,100);
 left.DELAY_fifo=fifo.data(); right.DELAY_fifo=fifo.data();
 WaveLFO lfoL,lfoR; left.LFO_ptr=&lfoL; right.LFO_ptr=&lfoR;
 AudioGain gainL,gainR;
 PlayersManager players;
 DelayManager &m = Delay_manager;
 LillaReverb reverb;
 m.Reverb_ptr = &reverb;
 LillaCompressor compressor;
 m.Compressor_ptr = &compressor;
 compressor.Set_enabled(true);
 assert(m.Set_value(DELAY_THRESHOLD, -24));
 m.Update();
 assert(compressor.threshold == -24 && Delay_data.compressor_threshold == -24);
 Delay_data_struct compressor_patch = Delay_data;
 compressor_patch.compressor = false;
 compressor_patch.compressor_threshold = -3;
 m.New_values(&compressor_patch);
 m.Update();
 assert(compressor.enabled); // Loading a legacy OFF patch must not disable compression.
 assert(compressor.threshold == -3);
 AudioFilterBiquad filterL, filterR;
 m.D_lpf_L_ptr = &filterL; m.D_lpf_R_ptr = &filterR;
 m.Delay_L_ptr=&left; m.Delay_R_ptr=&right; m.LFO_D_ptr[0]=&lfoL; m.LFO_D_ptr[1]=&lfoR; m.D_gain_L_feedback_ptr=&gainL; m.D_gain_R_feedback_ptr=&gainR; m.Players_Manager_ptr=&players;
 assert(m.Set_value(INSTRUMENT_ROUTE,0xA4)); m.Update(); assert(players.routes.size()==8); for(int i=0;i<8;++i) assert(players.routes[i]==i);
 m.Set_value(SAMPLES,50);
 for(int offset: {-5,0,5,0,-10,10,0}) {
  m.Set_value(SAMPLES_LR,offset); m.Update();
  int base=Calc_delay_samples(50), delta=Calc_delay_samples_LR(offset);
  assert(left.delay_central_value_target==base+std::max(delta,0)); assert(right.delay_central_value_target==base-std::min(delta,0));
  assert(!m.flag[SAMPLES] && !m.flag[SAMPLES_LR]);
 }
 Delay_data_struct patch=Delay_data; patch.loop_gain=9; patch.modulation_frequency=90; patch.modulation_depth=39; patch.modulation_phase_LR=359;
 assert(m.New_values(&patch));
 for(int i=0;i<10;++i) { m.Update(); DrainGain(gainL); DrainGain(gainR); }
 float previous=Delay_values.modulation_depth; m.Set_value(MODULATION_DEPTH,0); assert(m.y_1[MODULATION_DEPTH]==previous); assert(m.remaining[MODULATION_FREQUENCY]==20);
 for(int i=0;i<30;++i) { m.Update(); DrainGain(gainL); DrainGain(gainR); assert(Delay_values.loop_gain>=Delay_feedback(9) && Delay_values.loop_gain<=0); }
 assert(Delay_values.modulation_depth==0); assert(Delay_values.modulation_frequency==Calc_delay_frequency(90)); assert(Delay_values.modulation_phase_LR==359); assert(Delay_values.loop_gain==Delay_feedback(9)); assert(gainL.gain_runtime==Delay_feedback(9));
 for(int i=0;i<DELAY_ITEMS;++i) assert(!m.flag[i]);
 float frequency=Delay_values.modulation_frequency; m.Set_value(INSTRUMENT_ROUTE,1); for(int i=0;i<40;++i) m.Update(); assert(Delay_values.modulation_frequency==frequency);
 m.Set_value(LOOP_GAIN,0); m.Stop(); DrainGain(gainL); DrainGain(gainR); assert(gainL.gain_runtime==0 && gainL.multiplier==0); assert(Delay_data.loop_gain==0);
 m.Set_value(MODULATION_DEPTH,400); assert(Delay_data.modulation_depth==39); m.Set_value(LOOP_GAIN,99); assert(Delay_data.loop_gain==98); assert(Delay_feedback(99)==Delay_feedback(98)); m.Stop();
 AudioGain tiny; tiny.Set_gain(0.5001f); DrainGain(tiny); assert(tiny.gain_runtime==0.5001f); tiny.Mute(); DrainGain(tiny); assert(tiny.multiplier==0); tiny.Unmute(); DrainGain(tiny); assert(tiny.gain_runtime==0.5001f);
 left.Setup_delay(0); left.Set_delay_central_value(501); int blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<3000); } assert(left.delay_value==501);
 left.Set_delay_central_value(3); blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<3000); } assert(left.delay_value==3);
 left.Set_delay_central_value(2200); for(int i=0;i<5;++i) left.update(); left.Set_delay_central_value(100); blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<3000); } assert(left.delay_value==100);
 left.Set_delay_central_value(500); left.queued[0]=&left.input[0]; left.queued[1]=&left.input[1]; left.fail=true; int read=left.sample_read,write=left.sample_write,releases=left.releases,counter=left.J_delay_central_value_counter; left.update(); assert(left.releases==releases+2 && left.sample_read==read && left.sample_write==write && left.J_delay_central_value_counter==counter); left.fail=false; left.update();
 left.Set_delay_central_value(99); left.Setup_delay(300); assert(left.J_delay_central_value_counter==0 && left.delay_value==300);
 left.Set_delay_central_value(DELAY_CACHE_CHANNEL_SAMPLES); blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<DELAY_CACHE_CHANNEL_SAMPLES); } assert(left.delay_value==DELAY_CACHE_ACTIVE_SAMPLES-AUDIO_BLOCK_SAMPLES);
 left.Set_delay_central_value(-1); blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<DELAY_CACHE_CHANNEL_SAMPLES); } assert(left.delay_value==0);
 Delay_data_struct same=Delay_data; assert(!m.New_values(&same)); m.Set_value(MODULATION_DEPTH,0); m.Update(); auto remaining=m.remaining[MODULATION_DEPTH]; same=Delay_data; assert(m.New_values(&same)); assert(m.remaining[MODULATION_DEPTH]==remaining); m.Stop();
 // A short delay must read the current input block instead of stale circular-buffer contents.
 for (int delay : {Calc_delay_samples(5), 0, 1, 126, 127, 128, 129, 441, DELAY_CACHE_ACTIVE_SAMPLES - AUDIO_BLOCK_SAMPLES})
 {
    std::fill(fifo.begin(), fifo.end(), 0);
    left.Setup_delay(delay);
    left.Set_delay_modulation_source(0);
    for (int block = 0; block < DELAY_CACHE_ACTIVE_SAMPLES / AUDIO_BLOCK_SAMPLES + 8; ++block)
    {
        for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            left.input[0].data[sample] = ((block * AUDIO_BLOCK_SAMPLES + sample) * 37 % 20001) - 10000;
        }
        left.queued[0] = &left.input[0];
        left.update();
        for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            const int source_sample = block * AUDIO_BLOCK_SAMPLES + sample - delay;
            const int expected = source_sample < 0 ? 0 : (source_sample * 37 % 20001) - 10000;
            if (left.output.data[sample] != expected)
            {
                std::printf("Short delay failure: delay=%d block=%d sample=%d expected=%d actual=%d\n", delay, block, sample, expected, left.output.data[sample]);
                return 1;
            }
        }
    }
 }
 // Exercise all UI time positions, signed offsets, patch loading and startup normalization.
 assert(DELAY_CACHE_ACTIVE_SAMPLES == 88328);
 assert(Calc_delay_samples(DELAY_TIME_MAX_INDEX) == 87759);
 assert(Calc_delay_samples_LR_limit(DELAY_TIME_MAX_INDEX) == 10);
 std::vector<int> times;
 for (int t = 0; t <= 10; ++t) { times.push_back(t); }
 for (int t = 20; t <= 100; t += 10) { times.push_back(t); }
 for (int t = 150; t <= 2000; t += 50) { times.push_back(t); }
 for (int t = 2100; t <= 5000; t += 100) { times.push_back(t); }
 for (int t = 6000; t <= 20000; t += 1000) { times.push_back(t); }
 times.back() = 19900;
 assert(times.size() == DELAY_TIME_MAX_INDEX + 1);
 const int legacy_gains[] = {0, 7, 18, 40, 60, 65, 71, 80, 92, 98};
 for (int old_index = 0; old_index <= 99; ++old_index)
 {
    Delay_data_struct legacy{};
    legacy.samples = old_index;
    legacy.loop_gain = old_index % 10;
    Convert_legacy_delay(legacy);
    assert(legacy.loop_gain == legacy_gains[old_index % 10]);
    const int old_samples = lroundf(88200 * powf(old_index / 99.0f, 2.5f));
    for (int index = 0; index <= DELAY_TIME_MAX_INDEX; ++index)
    {
        assert(abs(Calc_delay_samples(legacy.samples) - old_samples) <= abs(Calc_delay_samples(index) - old_samples));
    }
 }
 for (int gain = 0; gain <= 98; ++gain) { m.Set_value(LOOP_GAIN, gain); assert(Delay_data.loop_gain == gain); assert(std::abs(Delay_feedback(gain) + gain / 100.0f) < 0.000001f); }
 for (int time = 0; time <= DELAY_TIME_MAX_INDEX; ++time)
 {
    assert(Calc_delay_time_tenths(time) == times[time]);
    const int limit = Calc_delay_samples_LR_limit(time);
    assert(Calc_delay_samples_LR(limit) <= Calc_delay_samples(time));
    if (limit < 10)
    {
        assert(Calc_delay_samples_LR(limit + 1) > Calc_delay_samples(time) || Calc_delay_samples_LR(limit + 1) > DELAY_CACHE_ACTIVE_SAMPLES - AUDIO_BLOCK_SAMPLES - Calc_delay_samples(time));
    }
    for (int sign : {-1, 1})
    {
        Delay_data_struct loaded = Delay_data;
        loaded.samples = time;
        loaded.samples_LR = sign * 10;
        m.New_values(&loaded);
        m.Update();
        assert(Delay_data.samples_LR == sign * limit);
        assert(std::abs(Delay_values.samples_LR) <= Delay_values.samples);
        assert(left.delay_central_value_target <= DELAY_CACHE_ACTIVE_SAMPLES - AUDIO_BLOCK_SAMPLES);
        assert(right.delay_central_value_target <= DELAY_CACHE_ACTIVE_SAMPLES - AUDIO_BLOCK_SAMPLES);
        assert(right.delay_central_value_target - left.delay_central_value_target == -sign * Calc_delay_samples_LR(limit));
        Calc_Delay_values(loaded);
        assert(loaded.samples_LR == sign * limit);
    }
 }
 m.Set_value(SAMPLES, 0);
 m.Update();
 assert(Delay_data.samples_LR == 0 && Delay_values.samples_LR == 0);
 assert(left.delay_central_value_target == 0 && right.delay_central_value_target == 0);
 // Keep a sentinel in allocated storage outside the active read/write span.
 std::fill(fifo.begin(), fifo.begin() + DELAY_CACHE_ACTIVE_SAMPLES, 100);
 std::fill(fifo.begin() + DELAY_CACHE_ACTIVE_SAMPLES, fifo.end(), -12345);
 left.Setup_delay(DELAY_CACHE_ACTIVE_SAMPLES - AUDIO_BLOCK_SAMPLES);
 left.Set_delay_central_value(0);
 for (int block = 0; block < 15000; ++block)
 {
    left.update();
    assert(left.sample_read >= 0 && left.sample_read < DELAY_CACHE_ACTIVE_SAMPLES);
    assert(left.sample_write >= 0 && left.sample_write < DELAY_CACHE_ACTIVE_SAMPLES);
    for (int sample : left.output.data)
    {
        assert(sample >= 0 && sample <= 100);
    }
 }
 assert(left.delay_value == 0);
 for (int i = DELAY_CACHE_ACTIVE_SAMPLES; i < DELAY_CACHE_CHANNEL_SAMPLES; ++i)
 {
    assert(fifo[i] == -12345);
 }
 // Panic must cancel a rising target and an already-requested zero without losing other transitions.
 for (int target : {98, 0})
 {
    m.Set_value(LOOP_GAIN, 60);
    m.Stop();
    DrainGain(gainL);
    DrainGain(gainR);
    m.Set_value(LOOP_GAIN, target);
    m.Update();
    m.Set_value(MODULATION_DEPTH, Delay_data.modulation_depth == 0 ? 10 : 0);
    assert(m.flag[LOOP_GAIN]);
    m.Silence_feedback();
    assert(!m.flag[LOOP_GAIN] && m.remaining[LOOP_GAIN] == 0);
    assert(m.flag[MODULATION_DEPTH]);
    assert(Delay_data.loop_gain == target && Delay_values.loop_gain == 0);
    assert(!gainL.Is_silent() && !gainR.Is_silent());
    for (int block = 0; block < 40; ++block)
    {
        m.Update();
        assert(gainL.gain_target == 0 && gainR.gain_target == 0);
    }
    DrainGain(gainL);
    DrainGain(gainR);
    assert(gainL.Is_silent() && gainR.Is_silent());
    m.Restore_feedback();
    assert(Delay_data.loop_gain == target && Delay_values.loop_gain == Delay_feedback(target));
    assert(gainL.gain_target == Delay_feedback(target) && gainR.gain_target == Delay_feedback(target));
    assert(gainL.Is_silent() == (target == 0));
    DrainGain(gainL);
    DrainGain(gainR);
    for (int block = 0; block < 40; ++block)
    {
        m.Update();
        assert(gainL.gain_runtime == Delay_feedback(target) && gainR.gain_runtime == Delay_feedback(target));
    }
 }

 // Exercise the real UI reader and real delay processors, not only their targets.
 std::vector<int16_t> fifoR(DELAY_CACHE_CHANNEL_SAMPLES, 0);
 right.DELAY_fifo = fifoR.data();
 m.Set_value(MODULATION_SOURCE, 0);
 m.Set_value(SAMPLES, 24); // 35 ms.
 m.Set_value(SAMPLES_LR, 0);
 m.Stop();
 left.Setup_delay(Calc_delay_samples(24));
 right.Setup_delay(Calc_delay_samples(24));
 for (int requested : {-5, 0, 5, 0})
 {
    while (Delay_data.samples_LR != requested)
    {
        Encoders_manager.rotation = Delay_data.samples_LR > requested ? 1 : -1;
        assert(D_Read_value(SAMPLES_LR));
        m.Update();
    }
    int settling = 0;
    do
    {
        left.update();
        right.update();
        assert(++settling < 2000);
    } while (left.delay_value != left.delay_central_value_target || right.delay_value != right.delay_central_value_target);
    const int base = Calc_delay_samples(24);
    const int offset = Calc_delay_samples_LR(requested);
    assert(left.delay_value == base + std::max(offset, 0));
    assert(right.delay_value == base - std::min(offset, 0));
    std::fill(fifo.begin(), fifo.end(), 0);
    std::fill(fifoR.begin(), fifoR.end(), 0);
    int firstL = -1;
    int firstR = -1;
    for (int block = 0; block < 20; ++block)
    {
        std::fill(std::begin(left.input[0].data), std::end(left.input[0].data), 0);
        std::fill(std::begin(right.input[0].data), std::end(right.input[0].data), 0);
        if (block == 0)
        {
            left.input[0].data[0] = 10000;
            right.input[0].data[0] = 10000;
        }
        left.queued[0] = &left.input[0];
        right.queued[0] = &right.input[0];
        left.update();
        right.update();
        for (int sample = 0; sample < AUDIO_BLOCK_SAMPLES; ++sample)
        {
            if (left.output.data[sample] != 0 && firstL < 0)
            {
                firstL = block * AUDIO_BLOCK_SAMPLES + sample;
            }
            if (right.output.data[sample] != 0 && firstR < 0)
            {
                firstR = block * AUDIO_BLOCK_SAMPLES + sample;
            }
        }
    }
    assert(firstL == base + std::max(offset, 0));
    assert(firstR == base - std::min(offset, 0));
 }
 // Clockwise must stop at Right +10 ms and leave other parameters' direction unchanged.
 m.Set_value(SAMPLES_LR, -10);
 Encoders_manager.rotation = 1;
 assert(!D_Read_value(SAMPLES_LR));
 m.Set_value(SAMPLES_LR, 10);
 Encoders_manager.rotation = -1;
 assert(!D_Read_value(SAMPLES_LR));
 m.Set_value(SAMPLES, DELAY_TIME_MAX_INDEX);
 m.Update();
 assert(Delay_data.samples_LR == 10);
 Encoders_manager.rotation = 1;
 assert(D_Read_value(SAMPLES_LR) && Delay_data.samples_LR == 9);
 m.Set_value(SAMPLES_LR, -10);
 m.Update();
 assert(right.delay_central_value_target == 88200);
 assert(left.delay_central_value_target == 87759);
 m.Set_value(SAMPLES_LR, 10);
 m.Update();
 assert(left.delay_central_value_target == 88200);
 assert(right.delay_central_value_target == 87759);
 m.Set_value(LOOP_GAIN, 50);
 Encoders_manager.rotation = 1;
 assert(D_Read_value(LOOP_GAIN) && Delay_data.loop_gain == 51);
 m.Set_value(REVERB_LEVEL, 0);
 Encoders_manager.rotation = 1;
 assert(D_Read_value(REVERB_LEVEL) && Delay_data.reverb_level == 5);
 m.Set_value(REVERB_LEVEL, 100);
 Encoders_manager.rotation = 1;
 assert(!D_Read_value(REVERB_LEVEL));
 m.Set_value(REVERB_DECAY, 0);
 Encoders_manager.rotation = -1;
 assert(!D_Read_value(REVERB_DECAY));
 m.Set_value(REVERB_DAMPING, 50);
 Encoders_manager.rotation = -1;
 assert(D_Read_value(REVERB_DAMPING) && Delay_data.reverb_damping == 40);
 value_pressed = true;
 assert(D_Read_value(REVERB_DAMPING) && Delay_data.reverb_damping == 0);
 value_pressed = true;
 assert(D_Read_value(REVERB_LEVEL) && Delay_data.reverb_level == 0);
 value_pressed = true;
 assert(D_Read_value(REVERB_LEVEL) && Delay_data.reverb_level == 100);
 value_pressed = true;
 assert(D_Read_value(REVERB_DAMPING) && Delay_data.reverb_damping == 40);
 m.Set_value(REVERB_DECAY, 40);
 value_pressed = true;
 assert(D_Read_value(REVERB_DECAY) && Delay_data.reverb_decay == 0);
 m.Update();
 value_pressed = true;
 assert(D_Read_value(REVERB_DECAY) && Delay_data.reverb_decay == 40);
 value_pressed = true;
 assert(D_Read_value(REVERB_DECAY) && Delay_data.reverb_decay == 0);
 Encoders_manager.rotation = 1;
 assert(D_Read_value(REVERB_DECAY) && Delay_data.reverb_decay == 1);
 value_pressed = true;
 assert(D_Read_value(REVERB_DECAY) && Delay_data.reverb_decay == 0);
 value_pressed = true;
 assert(D_Read_value(REVERB_DECAY) && Delay_data.reverb_decay == 1);
 m.Set_value(REVERB_LEVEL, 98);
 Encoders_manager.rotation = 1;
 assert(D_Read_value(REVERB_LEVEL) && Delay_data.reverb_level == 100);
 m.Set_value(REVERB_DAMPING, 3);
 Encoders_manager.rotation = -1;
 assert(D_Read_value(REVERB_DAMPING) && Delay_data.reverb_damping == 0);
 // Independent, bounded feedback filter transitions reach the same cutoff on both channels.
 for (int target : {0, LPF_MAX, 20})
 {
    const float start = Delay_values.lpf_cutoff_hz;
    m.Set_value(FEEDBACK_CUTOFF, target);
    for (int block = 0; block < 30; ++block)
    {
        m.Update();
        assert(filterL.frequency == filterR.frequency);
        assert(filterL.frequency >= std::min(start, static_cast<float>(lowpass_value[target])));
        assert(filterL.frequency <= std::max(start, static_cast<float>(lowpass_value[target])));
        assert(filterL.q == 0.7071 && filterR.q == 0.7071);
    }
    assert(!m.flag[FEEDBACK_CUTOFF] && filterL.frequency == lowpass_value[target]);
 }
 m.Set_value(FEEDBACK_CUTOFF, 0);
 m.Update();
 const float intermediate = Delay_values.lpf_cutoff_hz;
 m.Set_value(FEEDBACK_CUTOFF, LPF_MAX);
 assert(m.y_1[FEEDBACK_CUTOFF] == intermediate);
 m.Stop();
 assert(filterL.frequency == 20000 && filterR.frequency == 20000);
 Serial.println(F("PASS: actual stereo impulse timing, clockwise Right offset, both bounds and zero, independent feedback cutoff ramps"));
 Serial.println(F("PASS: routing, time ramps, allocation failure, feedback bounds, Panic silences and restores the requested feedback"));
}
'''
compiler = shutil.which('g++') or r'C:\msys64\ucrt64\bin\g++.exe'
with tempfile.TemporaryDirectory(prefix='lilla-delay-') as directory:
    cpp = Path(directory) / 'delay.cpp'
    exe = Path(directory) / ('delay.exe' if os.name == 'nt' else 'delay')
    cpp.write_text(prefix + shared + gain + stereo + manager + tests, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Wno-unused-parameter', str(cpp), '-o', str(exe)], check=True)
    environment = os.environ.copy()
    environment['PATH'] = str(Path(compiler).parent) + os.pathsep + environment.get('PATH', '')
    subprocess.run([str(exe)], check=True, env=environment)
