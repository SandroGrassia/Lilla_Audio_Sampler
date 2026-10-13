/*
 * LILLA Audio Sampler
 * Author: Sandro Grassia, info@lillasampler.it
 *
 */

#include "SharedMixer.h"
#include "PlayersManager.h"
#include "AmpliOutMuteIn.h"

uint8_t MX_routing_source[MX_sources] = {2, 2, 2, 2, 2, 2, 2, 2, 0}; // 0-->7: Sound 8: InputDevice; 1 --> source routed to PWM output (monitor); 2 --> source routed to Audio Board output;  3 --> source routed to both
bool MX_mute[MX_sources] = {false, false, false, false, false, false, false, false, false};
void MX_Select_prelisten(int source, bool enabled)
{
    for (int track = 0; track < MX_sources; ++track)
    {
        MX_routing_source[track] &= 2u;
    }
    if (enabled)
    {
        MX_routing_source[source] |= 1u;
    }
}

void MX_Only_prelisten(int source)
{
    MX_Select_prelisten(source, true);
    MX_routing_source[source] = 1;
    MX_mute[source] = false;
}

void MX_Send_to_lineout(int source)
{
    MX_routing_source[source] |= 2u;
    MX_mute[source] = false;
}

void MX_Apply_routing(PlayersManager &players, AmpliOutMuteIn &main_left, AmpliOutMuteIn &main_right, AmpliOutMuteIn &prelisten_left, AmpliOutMuteIn &prelisten_right)
{
    for (int instrument = 0; instrument < LINE_IN_source; ++instrument)
    {
        players.MX_multicast_change_routing(instrument);
    }
    const float main_gain = (!MX_mute[LINE_IN_source] && (MX_routing_source[LINE_IN_source] & 2u)) ? 1.0f : 0.0f;
    const float prelisten_gain = (!MX_mute[LINE_IN_source] && (MX_routing_source[LINE_IN_source] & 1u)) ? 1.0f : 0.0f;
    main_left.gain(1, main_gain);
    main_right.gain(1, main_gain);
    prelisten_left.gain(1, prelisten_gain);
    prelisten_right.gain(1, prelisten_gain);
}

bool MX_Mute_allowed(int source, bool sampler_patch, int solo_instrument)
{
    return source == LINE_IN_source || (!sampler_patch && source != solo_instrument);
}
