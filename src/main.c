#include "stm32f0xx.h"
#include <math.h>
#include <stdint.h>
#define SAMPLES 30
uint16_t array[SAMPLES];

void setup_tim7(int fre)
{
    //produce the sinusoid
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    for(int x=0; x < SAMPLES; x += 1)
        array[x] = 2048 + 1952 * sin(2 * M_PI * x / SAMPLES);

    //ENABLE TIMER 7
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //enable the DAC with the Timer 7 TRGO
    //enable timer 7 with the TIMx_Cr2
    RCC->APB1ENR |= (RCC_APB1ENR_TIM7EN); //the place with allll the timers uwu
    TIM7->CR2 &= ~0b1110000;
    TIM7->CR2 |=  0b0100000;


    //trigger a dma request 1khz on TIM2_CH2
    //x = 1/(z*SAMP)
    TIM7->DIER |= 1<<8;
    TIM7->PSC = 0;
    int x = 48000000 / (fre * SAMPLES);
    TIM7->ARR = x-1;
    //https://numero.wiki/654657
    //48m/(psc+1)/arr+1 = fre*samples;


    DAC->CR |= 0x1000;

    //Make sure you enable the timer.
    TIM7->CR1 |= 0b1;

    //Make sure you configure the TRGO event to happen on update.
    //ONLY USE CH4 // look at Table 31, on page 203 of the Family Reference Manual
    TIM7->DIER |= 0x100;//set the UDE bit of the DIER register of the timer to trigger a DMA transfer when the timer update event occurs.

    //WHY ARE WE DOING THIS !!!
 //   TIM7->DIER |= 0b1;


    //ENABLE DMA
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //enable DMA transfer on DAC ch1, set TIM7 as trigger by writing 010 as TSEL1 and TSEL2
        DAC->CR |= DAC_CR_TSEL1_1 | DAC_CR_TEN2 | DAC_CR_EN2 | DAC_CR_TSEL2_1;
        DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    //enable RCC DMA clock
    RCC->AHBENR &= ~RCC_AHBENR_DMA1EN;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;

    //memory source is the array DataArr
    DMA1_Channel4->CMAR = (uint32_t)array; /* (3) */ //found in table 32

    //peripheral destination of data transfer is DAC DHR12R1
    DMA1_Channel4->CPAR = (uint32_t) (&(DAC->DHR12R1));

    //count of data elements to transfer is the number of elements in the array ArrSize
    DMA1_Channel4->CNDTR = SAMPLES;

    //transfer direction is mem -> per
    DMA1_Channel4->CCR &= ~DMA_CCR_DIR;
    DMA1_Channel4->CCR |= DMA_CCR_DIR;

    //set memory sizes
    DMA1_Channel4->CCR &= ~DMA_CCR_MSIZE_1 ;
    DMA1_Channel4->CCR &= ~DMA_CCR_PSIZE_1;
    DMA1_Channel4->CCR |= DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0;

    //mem address should be incremented after each transfer
    DMA1_Channel4->CCR &= ~DMA_CCR_MINC;
    DMA1_Channel4->CCR |= DMA_CCR_MINC;

    //Disable PINC
    DMA1_Channel4->CCR &= ~DMA_CCR_PINC;

    //array will be transferred repeatedly, so channel should be set for circular ops
    DMA1_Channel4->CCR &= ~DMA_CCR_CIRC;
    DMA1_Channel4->CCR |= DMA_CCR_CIRC;


    //enable DMA channel uwu
    DMA1_Channel4->CCR &= ~DMA_CCR_EN;
    DMA1_Channel4->CCR |= DMA_CCR_EN;


    //SETUP DAC
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //enable RCC clock ASAP
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;

    //set TSEL1 field so that DAC is triggered by the timer I selected
    DAC->CR |= 0b010000;

    //enable trigger for ch1
    DAC->CR |=DAC_CR_TEN1;

    //enable ch1
    DAC->CR |=DAC_CR_EN1;

    //SET DMAEN1 since we triggering DMA with DAC
    //uwu that's what we did above >w<

    //Enable PA4 for output
    GPIOA->MODER &= ~0x0300;
    GPIOA->MODER |=  0x0100;
    return;
}



int main(void)
{
    // Uncomment any one of the following calls ...
    //setup_tim7(1000);
    setup_tim7(1234.5);
    //setup_tim7(10000);
    //setup_tim7(100000);

}






























const char font[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, // 32: space
    0x86, // 33: exclamation
    0x22, // 34: double quote
    0x76, // 35: octothorpe
    0x00, // dollar
    0x00, // percent
    0x00, // ampersand
    0x20, // 39: single quote
    0x39, // 40: open paren
    0x0f, // 41: close paren
    0x49, // 42: asterisk
    0x00, // plus
    0x10, // 44: comma
    0x40, // 45: minus
    0x80, // 46: period
    0x00, // slash
    // digits
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67,
    // seven unknown
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    // Uppercase
    0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x6f, 0x76, 0x30, 0x1e, 0x00, 0x38, 0x00,
    0x37, 0x3f, 0x73, 0x7b, 0x31, 0x6d, 0x78, 0x3e, 0x00, 0x00, 0x00, 0x6e, 0x00,
    0x39, // 91: open square bracket
    0x00, // backslash
    0x0f, // 93: close square bracket
    0x00, // circumflex
    0x08, // 95: underscore
    0x20, // 96: backquote
    // Lowercase
    0x5f, 0x7c, 0x58, 0x5e, 0x79, 0x71, 0x6f, 0x74, 0x10, 0x0e, 0x00, 0x30, 0x00,
    0x54, 0x5c, 0x73, 0x7b, 0x50, 0x6d, 0x78, 0x1c, 0x00, 0x00, 0x00, 0x6e, 0x00
};
