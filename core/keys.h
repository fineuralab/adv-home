#pragma once
#include <stdint.h>

// ADV's 7x8 TCA matrix is physically arranged as four keyboard rows.
static inline int home_key_event(uint8_t event) {
    if(!(event&0x80))return 0;
    const int id=(event&0x7f)-1;
    if(id<0||id/10>6||id%10>7)return 0;
    const int row=id%10%4,col=id/10*2+(id%10>=4);
    if((row==0&&col==0)||(row==0&&col==13))return '`';
    if(row==2&&col==13)return '\n';
    if(row==2&&col==11)return ';';
    if(row==3&&col==10)return ',';
    if(row==3&&col==11)return '.';
    if(row==3&&col==12)return '/';
    return '?';
}
