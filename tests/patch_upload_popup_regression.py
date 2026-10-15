"""Exercise pending cache status and the Performance upload popup lifecycle."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]

# The synchronous SD load must display the same managed popup before blocking.
main_source = (root / 'src/main.cpp').read_text(encoding='utf-8')
prepare = main_source[main_source.index('FLASHMEM bool P_Prepare_audio_tables('):]
sd_branch = prepare[:prepare.index('const bool audio_interrupts_enabled')]
assert sd_branch.index('if (!PatchCache_Manager.Sd_sources_ready(patch_id))') < sd_branch.index('Display_Performance.P_update_upload_popup(true, P_pointer);') < sd_branch.index('PatchCache_Manager.Load_sd_sources(patch_id);')
def method(name, signature):
    return re.search(r'^' + re.escape(signature) + r'\n\{.*?^\}', (root / name).read_text(encoding='utf-8'), re.M | re.S).group(0)
code = r"""
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
constexpr int PERFORMANCE = 1, ILI9341_BLACK = 0, ILI9341_GREEN = 2;
int Lilla_state = PERFORMANCE, draws = 0, pages = 0, frames = 0;
uint32_t now_ms = 0;
uint32_t millis() { return now_ms; }
void Show_popup_text(const char *text, int foreground, int background)
{
    assert(std::strcmp(text, "PATCH UPLOADING") == 0);
    assert(foreground == ILI9341_BLACK && background == ILI9341_GREEN);
    ++draws;
}
struct P_field_description_struct { int element = 0; };
struct DisplayPerformance
{
    bool upload_popup_visible = false;
    uint32_t upload_popup_drawn_ms = 0;
    int restored = -1;
    bool P_update_upload_popup(bool loading, P_field_description_struct pointer);
    void P_show_PERFORMANCE_page(bool, bool) { ++pages; upload_popup_visible = false; }
    void P_show_pointer_frame(P_field_description_struct pointer, bool show) { assert(show); ++frames; restored = pointer.element; }
};
struct PatchCacheManager
{
    struct Request { int file_id; uint32_t samples; bool failed; } required[3] = {{4, 100, false}, {7, 200, false}, {9, 0, true}};
    struct Cache { bool valid; int file_id; uint32_t samples; } cache[3] = {};
    static constexpr int PATCH_CACHE_ARRAY_COUNT = 3;
    uint8_t required_count = 0;
    int Find_complete(int16_t file_id, uint32_t samples) const;
    bool Has_pending_copy(void) const;
};
"""
code += method('lib/PatchCacheManager/PatchCacheManager.cpp', 'int PatchCacheManager::Find_complete(int16_t file_id, uint32_t samples) const')
code += '\n' + method('lib/PatchCacheManager/PatchCacheManager.cpp', 'bool PatchCacheManager::Has_pending_copy(void) const')
code += '\n' + method('lib/DisplayPerformance/DisplayPerformance.cpp', 'bool DisplayPerformance::P_update_upload_popup(bool loading, P_field_description_struct pointer)')
code += r"""
int main()
{
    PatchCacheManager cache;
    DisplayPerformance display;
    const P_field_description_struct pointer{6};
    assert(!cache.Has_pending_copy());
    assert(!display.P_update_upload_popup(false, pointer) && draws == 0);
    cache.required_count = 3;
    assert(cache.Has_pending_copy());
    display.P_update_upload_popup(cache.Has_pending_copy(), pointer);
    assert(draws == 1 && pages == 0);
    now_ms = 99;
    display.P_update_upload_popup(true, pointer);
    assert(draws == 1);
    now_ms = 100;
    display.P_update_upload_popup(true, pointer);
    assert(draws == 2);
    cache.cache[0] = {false, 4, 100}; // Partial copy must keep the popup visible.
    assert(cache.Has_pending_copy());
    cache.cache[0].valid = true;
    assert(cache.Has_pending_copy()); // The second file is still queued.
    cache.required[1].failed = true; // Flash fallback must not leave a stuck popup.
    assert(!cache.Has_pending_copy());
    assert(display.P_update_upload_popup(false, pointer));
    assert(pages == 1 && frames == 1 && display.restored == 6);
    assert(!display.P_update_upload_popup(false, pointer) && pages == 1);
    cache.required[1].failed = false;
    cache.cache[1] = {true, 7, 200};
    assert(!cache.Has_pending_copy()); // Fully cached patch requires no popup.
    cache.cache[1].valid = false;
    display.P_update_upload_popup(cache.Has_pending_copy(), pointer);
    Lilla_state = 2;
    assert(!display.P_update_upload_popup(false, pointer));
    assert(!display.upload_popup_visible && pages == 1); // Never redraw Performance over another page.
    Lilla_state = PERFORMANCE;
    display.P_update_upload_popup(true, pointer);
    cache.required_count = 0; // Switching to an empty patch cancels the notice.
    assert(display.P_update_upload_popup(cache.Has_pending_copy(), pointer));
    display.P_update_upload_popup(true, pointer);
    display.P_show_PERFORMANCE_page(true, true);
    const int old_draws = draws;
    display.P_update_upload_popup(true, pointer);
    assert(draws == old_draws + 1); // Redraw immediately after a page refresh.
    std::cout << "PASS: green upload popup, cache completion/failure, page transitions and cursor restoration\n";
}
"""
compiler = shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe'
env = os.environ.copy()
env['PATH'] = str(Path(compiler).parent) + os.pathsep + env.get('PATH', '')
with tempfile.TemporaryDirectory(prefix='patch-upload-') as folder:
    cpp = Path(folder) / 'test.cpp'
    exe = Path(folder) / 'test.exe'
    cpp.write_text(code, encoding='utf-8', newline='\r\n')
    subprocess.run([compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)], check=True, env=env)
    subprocess.run([str(exe)], check=True, env=env)
