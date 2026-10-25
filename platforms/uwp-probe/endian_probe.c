/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "pc/disc.h"

_Static_assert(sizeof(void*) == 8, "x64 pointer ABI");
_Static_assert(sizeof(long) == 4, "Windows LLP64 ABI");
_Static_assert((char)-1 < 0, "Melee requires signed char");

typedef struct DISC_STRUCT {
    uint16_t marker;
    uint16_t count;
    uint32_t value;
    float scalar;
    uint32_t offsets[2];
} ProbeRecord;
DISC_ASSERT_SIZE(ProbeRecord, 20);

typedef struct DISC_STRUCT {
    unsigned high : 3;
    unsigned low : 5;
    unsigned rest : 24;
} ProbeBits;
DISC_ASSERT_SIZE(ProbeBits, 4);

uint32_t melee_uwp_endian_probe(void) {
    const volatile unsigned char input[20] = {
        0x12,0x34, 0,2, 0x89,0xab,0xcd,0xef, 0x3f,0xc0,0,0,
        0,0,0,0x80, 0,0,1,0
    };
    ProbeRecord record;
    for(size_t i=0;i<sizeof record;++i)((unsigned char*)&record)[i]=input[i];
    uint32_t failures=0;
    if(record.marker!=0x1234 || record.count!=2) failures|=1;
    if(record.value!=0x89abcdefu || record.scalar!=1.5f) failures|=2;
    if(record.offsets[0]!=0x80 || record.offsets[1]!=0x100) failures|=4;
    record.marker=0x5678;record.scalar=2.5f;record.offsets[1]=0x10203040;
    unsigned char output[20];memcpy(output,&record,sizeof record);
    if(output[0]!=0x56 || output[1]!=0x78 || output[8]!=0x40 || output[9]!=0x20) failures|=8;
    if(output[16]!=0x10 || output[17]!=0x20 || output[18]!=0x30 || output[19]!=0x40) failures|=16;
    const volatile unsigned char bits[4]={0xb3,0x12,0x34,0x56};ProbeBits fields;
    for(size_t i=0;i<sizeof fields;++i)((unsigned char*)&fields)[i]=bits[i];
    if(fields.high!=5 || fields.low!=19 || fields.rest!=0x123456) failures|=32;
    return failures;
}

uint64_t melee_uwp_call_probe(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f) {
    return a+3*b+5*c+7*d+11*e+13*f;
}

uint32_t melee_uwp_stack_probe(void) {
    volatile unsigned char bytes[32768];uint32_t sum=0;
    for(unsigned i=0;i<sizeof bytes;++i)bytes[i]=(unsigned char)i;
    for(unsigned i=0;i<sizeof bytes;++i)sum+=bytes[i];
    return sum;
}
uint32_t melee_uwp_stdio_probe(void) {
    char output[64];unsigned value=0;float scalar=0;
    if(snprintf(output,sizeof output,"%u %.2f %llu",12345u,1.25,4294967296ull)!=21)return 1;
    if(strcmp(output,"12345 1.25 4294967296")!=0)return 2;
    if(sscanf("12345 1.25","%u %f",&value,&scalar)!=2||value!=12345u||scalar!=1.25f)return 3;
    if(snprintf(output,5,"%u",123456u)!=6||strcmp(output,"1234")!=0)return 4;
    return 0;
}
