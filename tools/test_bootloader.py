#!/usr/bin/env python3
"""Compile and exercise the actual custom entry policy with SDK API spies."""
from pathlib import Path
import subprocess
import tempfile

PROJECT = Path(__file__).resolve().parent.parent


def main():
    with tempfile.TemporaryDirectory(prefix="adv-home-boot-") as directory:
        root = Path(directory)
        (root / "sys").mkdir()
        (root / "sys/reent.h").write_text("struct _reent { int unused; };\nextern struct _reent fake_reent;\n#define _GLOBAL_REENT (&fake_reent)\n")
        (root / "sdkconfig.h").write_text("")
        (root / "bootloader_init.h").write_text("#define ESP_OK 0\nint bootloader_init(void);\n")
        (root / "bootloader_common.h").write_text("#pragma once\n#include <stdint.h>\ntypedef struct {uint8_t custom[32];} rtc_retain_mem_t;\nrtc_retain_mem_t *bootloader_common_get_rtc_retain_mem(void);\n")
        (root / "bootloader_utility.h").write_text("#pragma once\n#include <stdbool.h>\n#define FACTORY_INDEX (-1)\n#define MAX_OTA_SLOTS 16\ntypedef struct {unsigned offset,size;} position;\ntypedef struct {position factory,ota[16],test;} bootloader_state_t;\nbool bootloader_utility_load_partition_table(bootloader_state_t*);\n__attribute__((noreturn)) void bootloader_reset(void);\n__attribute__((noreturn)) void bootloader_utility_load_boot_image(const bootloader_state_t*,int);\n")
        harness = r'''
#include <assert.h>
#include <setjmp.h>
#include <string.h>
#include "bootloader_utility.h"
#include "bootloader_common.h"
#include "boot_request.h"
#include "sys/reent.h"
struct _reent fake_reent;
static rtc_retain_mem_t rtc;
static bootloader_state_t saved;
static jmp_buf jump;
static int chosen;
static bool table_ok=true,bad_slot=false;
int bootloader_init(void){return 0;}
rtc_retain_mem_t *bootloader_common_get_rtc_retain_mem(void){return &rtc;}
bool bootloader_utility_load_partition_table(bootloader_state_t *state){
 state->factory=(position){0x10000,0xf0000};
 state->ota[0]=(position){bad_slot?0x200000:0x100000,0x300000};
 state->ota[1]=(position){0x400000,0x300000};
 state->test=(position){0x700000,0x10000};return table_ok;
}
void bootloader_reset(void){longjmp(jump,2);}
void bootloader_utility_load_boot_image(const bootloader_state_t *state,int index){saved=*state;chosen=index;longjmp(jump,1);}
void call_start_cpu0(void);
int main(void){
 for(int slot=-1;slot<2;++slot){
  memset(&rtc,0,sizeof(rtc));if(slot>=0)home_boot_prepare((home_boot_request*)rtc.custom,(unsigned)slot);
  int status=setjmp(jump);if(status==0)call_start_cpu0();assert(status==1&&chosen==slot);
  assert(((home_boot_request*)rtc.custom)->magic==0);
  assert(saved.factory.offset==0x10000&&saved.test.size==0);
  for(int i=0;i<16;++i)assert(saved.ota[i].size==(i==slot?0x300000U:0U));
  status=setjmp(jump);if(status==0)call_start_cpu0();assert(status==1&&chosen==-1);
 }
 home_boot_prepare((home_boot_request*)rtc.custom,0);bad_slot=true;
 int status=setjmp(jump);if(status==0)call_start_cpu0();assert(status==1&&chosen==-1);
 table_ok=false;status=setjmp(jump);if(status==0)call_start_cpu0();assert(status==2);
}
'''
        source = root / "test.c"
        source.write_text(harness)
        executable = root / "test"
        subprocess.run(["clang", "-std=c11", "-fsanitize=address,undefined", "-I"+str(root),
                        "-I"+str(PROJECT/"core"), str(PROJECT/"bootloader_components/main/entry.c"),
                        str(source), "-o", str(executable)], check=True)
        subprocess.run([str(executable)], check=True)
    print("PASS: actual boot entry defaults to Home, consumes app A/B once, masks all unrequested apps, rejects changed layout and invalid tables (ASan/UBSan)")


if __name__ == "__main__":
    main()
