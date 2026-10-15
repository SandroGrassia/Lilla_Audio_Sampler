"""Repository availability must be initialized before the first Sound page draw."""
from pathlib import Path
import re

source = (Path(__file__).resolve().parents[1] / 'src/main.cpp').read_text(encoding='utf-8')
entries = 0
for match in re.finditer(r'Lilla_state = SOUND_EDIT;', source):
    end = source.index('Display_Sound.Show_SOUND_page(', match.end())
    entry = source[match.start():end]
    # Direct Sampling uses its recording, independently of repository browsing.
    if 'Sound_id = SOUNDS_MAX + channel;' in source[max(0, match.start()-200):end]:
        continue
    assert 'S_flash_first = Get_next_raw_file_in_flash(-1);' in entry, 'Flash availability is stale at entry'
    assert 'S_sd_files = SdSoundSource::Catalog_count();' in entry, 'SD availability is stale at entry'
    entries += 1
assert entries == 4, entries
print('PASS: all four patch/MIDI-loop Sound entry paths refresh repositories before rendering')
