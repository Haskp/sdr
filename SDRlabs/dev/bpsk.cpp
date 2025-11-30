#include <stdio.h>
int bpskARR[] = {0,1,1,0,1,1,0,0};
int bpskMod[]={};
int upsampleARR[] = (int)realloc(bpskMod,80*sizeof(int));


void upsampling (){

    for (int i=0; i<*bpskARR; i++){
        for(int j=0; j < upsampleARR;j+10){
            if (j%10 ==0){
            upsampleARR[j] == bpskMod[i];
            }
            }
        }
    }