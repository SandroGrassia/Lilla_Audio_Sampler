"""Run host regressions against production Delay code using minimal hardware stubs."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def source(path):  # Read production code without firmware-only includes.
    return re.sub(r'^\s*#(?:include|pragma)[^\n]*', '', (ROOT / path).read_text(encoding='utf-8'), flags=re.M)

def method(code, signature):  # Extract one complete production method for the hardware-free fixture.
    match = re.search(r'^' + re.escape(signature) + r'.*?^}', code, re.M | re.S)
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
shared = source('lib/SharedDelay/SharedDelay.h') + source('lib/SharedDelay/SharedDelay.cpp')
gain_header = source('lib/AudioGain/AudioGain.h').replace('private:', 'public:')
gain_cpp = source('lib/AudioGain/AudioGain.cpp')
gain = gain_header + '\nvoid AudioGain::update() {}\n' + '\n'.join(method(gain_cpp, 'void AudioGain::' + name) for name in ['Set_gain(', 'Mute(', 'Unmute(', 'Get_mults('])
stereo = source('lib/StereoDelay/StereoDelay.h').replace('private:', 'public:') + source('lib/StereoDelay/StereoDelay.cpp')
manager = source('lib/DelayManager/DelayManager.h').replace('private:', 'public:') + source('lib/DelayManager/DelayManager.cpp')
tests = r'''
void DrainGain(AudioGain &gain) { int count=0; while(gain.gain_flag) { gain.Get_mults(); assert(++count<10000); } } // Finish the real gain ramp.
int main() {
 StereoDelay left,right;
 std::vector<int16_t> fifo(DELAY_CACHE_CHANNEL_SAMPLES,100);
 left.DELAY_fifo=fifo.data(); right.DELAY_fifo=fifo.data();
 WaveLFO lfoL,lfoR; left.LFO_ptr=&lfoL; right.LFO_ptr=&lfoR;
 AudioGain gainL,gainR;
 PlayersManager players;
 DelayManager m;
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
 m.Set_value(MODULATION_DEPTH,400); assert(Delay_data.modulation_depth==39); m.Set_value(LOOP_GAIN,10); assert(Delay_data.loop_gain==9); assert(Delay_feedback(10)==Delay_feedback(9)); m.Stop();
 AudioGain tiny; tiny.Set_gain(0.5001f); DrainGain(tiny); assert(tiny.gain_runtime==0.5001f); tiny.Mute(); DrainGain(tiny); assert(tiny.multiplier==0); tiny.Unmute(); DrainGain(tiny); assert(tiny.gain_runtime==0.5001f);
 left.Setup_delay(0); left.Set_delay_central_value(501); int blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<3000); } assert(left.delay_value==501);
 left.Set_delay_central_value(3); blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<3000); } assert(left.delay_value==3);
 left.Set_delay_central_value(2200); for(int i=0;i<5;++i) left.update(); left.Set_delay_central_value(100); blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<3000); } assert(left.delay_value==100);
 left.Set_delay_central_value(500); left.queued[0]=&left.input[0]; left.queued[1]=&left.input[1]; left.fail=true; int read=left.sample_read,write=left.sample_write,releases=left.releases,counter=left.J_delay_central_value_counter; left.update(); assert(left.releases==releases+2 && left.sample_read==read && left.sample_write==write && left.J_delay_central_value_counter==counter); left.fail=false; left.update();
 left.Set_delay_central_value(99); left.Setup_delay(300); assert(left.J_delay_central_value_counter==0 && left.delay_value==300);
 left.Set_delay_central_value(DELAY_CACHE_CHANNEL_SAMPLES); blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<DELAY_CACHE_CHANNEL_SAMPLES); } assert(left.delay_value==DELAY_CACHE_CHANNEL_SAMPLES-AUDIO_BLOCK_SAMPLES);
 left.Set_delay_central_value(-1); blocks=0; while(left.J_delay_central_value_counter) { left.update(); assert(++blocks<DELAY_CACHE_CHANNEL_SAMPLES); } assert(left.delay_value==0);
 Delay_data_struct same=Delay_data; assert(!m.New_values(&same)); m.Set_value(MODULATION_DEPTH,0); m.Update(); auto remaining=m.remaining[MODULATION_DEPTH]; same=Delay_data; assert(m.New_values(&same)); assert(m.remaining[MODULATION_DEPTH]==remaining); m.Stop();
 Serial.println(F("PASS: routing 0-7, LR zero/sign changes, patch/UI retargeting, exact endpoints, stale flags, feedback bounds/zero, gain mute, time ramps, allocation failure"));
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
