#include "print.h"
int main(void){
    while(1){
        put_str("I am kernel!");
        put_char('\n');
        put_int(0x66666);
        put_char('\n');
    }
    return 0;
}