// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdint>
#include <cstdio>
#include <bit>
extern "C" uint32_t melee_uwp_endian_probe();
extern "C" uint32_t melee_uwp_stack_probe();
extern "C" uint32_t melee_uwp_stdio_probe();
extern "C" uint64_t melee_uwp_call_probe(uint64_t,uint64_t,uint64_t,uint64_t,uint64_t,uint64_t);
extern "C" int32_t HSD_Rand();
extern "C" float HSD_Randf();
extern "C" int32_t HSD_Randi(int32_t);
extern "C" uint32_t* HSD_RandSeedPtr;
int melee_uwp_run_abi_checks() {
    auto failures=melee_uwp_endian_probe();
    if(failures){std::printf("FAIL GCC disc data: mask=%u\n",failures);return 1;}
    std::puts("PASS GCC big-endian scalars, floats, arrays, bitfields and stores");
    if(melee_uwp_call_probe(1,2,3,4,5,6)!=183){std::puts("FAIL x64 register/stack ABI");return 2;}
    std::puts("PASS GCC-to-MSVC x64 register/stack ABI");
    if(melee_uwp_stack_probe()!=4177920u){std::puts("FAIL GCC stack probing");return 6;}
    std::puts("PASS GCC stack probing across 32 KiB");
    if(auto error=melee_uwp_stdio_probe()){std::printf("FAIL GCC-to-Store varargs: %u\n",error);return 7;}
    std::puts("PASS GCC-to-Store formatting, scanning and truncation");
    uint32_t expected=0x12345678;*HSD_RandSeedPtr=expected;
    for(unsigned i=0;i<10000;++i){expected=expected*214013u+2531011u;if(uint32_t(HSD_Rand())!=(expected>>16))return 3;}
    for(unsigned i=0;i<10000;++i){expected=expected*214013u+2531011u;float value=float(expected>>16)/65536.0f;if(std::bit_cast<uint32_t>(HSD_Randf())!=std::bit_cast<uint32_t>(value))return 4;}
    for(unsigned i=0;i<10000;++i){expected=expected*214013u+2531011u;if(HSD_Randi(17)!=int32_t(17*(expected>>16)/65536u))return 5;}
    std::puts("PASS original Melee RNG: 30000 integer/float/range steps");
    return 0;
}

#ifndef MELEE_UWP_ABI_APP
int main(){return melee_uwp_run_abi_checks();}
#endif
