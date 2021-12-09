#include "stm32f0xx.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "ff.h"
#include "ffconf.h"
#include "diskio.h"
#include "commands.h"
#include "wav2.h"

#define SAMPLES 8000

void setup(sWavHeader *header, BYTE buffer){

    //timer 6 enable
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->PSC = (24)-1;
    TIM6->ARR = (2000000/(header->SampleRate*header->BitsPerSample)) -1;
    TIM6->DIER |= TIM_DIER_UDE;
    TIM6->CR2 |= 0x20;
    TIM6->CR1 |= TIM_CR1_CEN;

    //setting up the DMA
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    DMA1_Channel3-> CMAR = (uint32_t)buffer;
    if(header->BitsPerSample == 8){
    DMA1_Channel3-> CPAR = (uint32_t)&(DAC->DHR8R1);
    DMA1_Channel3->CCR &= ~DMA_CCR_MSIZE |~DMA_CCR_PSIZE;
    }
    else{
        DMA1_Channel3-> CPAR = (uint32_t)&(DAC->DHR12L1);
        DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0 |DMA_CCR_PSIZE_0;
    }
    DMA1_Channel3->CNDTR = SAMPLES;
    DMA1_Channel3->CCR |= DMA_CCR_DIR;
    DMA1_Channel3->CCR |= DMA_CCR_MINC;
    DMA1_Channel3->CCR |= DMA_CCR_CIRC;
    DMA1_Channel3->CCR |= DMA_CCR_HTIE | DMA_CCR_TCIE;
    DMA1_Channel3->CCR |= DMA_CCR_EN;
    NVIC->ISER[0] = 1<<DMA1_Channel2_3_IRQn;

    //Set up the DAC
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR |= DAC_CR_TEN1;
    DAC->CR |= DAC_CR_EN1;
}

int __io_putchar(int c) //Emir Lab10
{
    while(!(USART5->ISR & USART_ISR_TXE)) { }
    ///
    if (c == '\n'){
    USART5->TDR = '\r';
    while(!(USART5->ISR & USART_ISR_TXE)){}
    }
    ///
    USART5->TDR = c;
    return c;
}
//char * filename
int main()
{
    sWavHeader header;
    //head header;
    FIL f;
    BYTE buffer[SAMPLES];
    UINT hs, br, br2;
    uint32_t fileLength;
    FATFS FatFs;
    FRESULT fres;

    fres = f_mount(&FatFs, "", 1);
    char* filename = "sine8.wav";
    f_open(&f, filename, FA_READ);

    f_read(&f, &header, sizeof(sWavHeader) ,&hs);
    //f_read(&f, &header, sizeof(head) ,&hs);
    //fileLength = header.Subchunk2Size;
    f_read(&f, &buffer, sizeof(buffer), &br);
    setup(&header, &buffer);
    while(!f_eof(f)){
        if(DMA1->ISR & DMA_ISR_HTIF3){
            DMA1->IFCR = DMA_IFCR_CHTIF3;
            f_read(&f, &buffer, SAMPLES/2, &br);
        }
        if(DMA1->ISR & DMA_ISR_TCIF3){
            DMA1->IFCR = DMA_IFCR_CTCIF3;
            f_read(&f, &buffer[SAMPLES/2], SAMPLES/2, &br2);
        }
    }

    f_close(&f);

    return 0;
}

