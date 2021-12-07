#include "stm32f0xx.h"
#include "commands.h"
#include "stdio.h"
#include "fifo.h"
#include "tty.h"
#include "lcd.h"
//#include "daniel.h"
#include <stddef.h>
#include <string.h>

#define FIFOSIZE 16
char serfifo[FIFOSIZE];
int seroffset = 0;

void init_usart5()//Emir Lab10
{
    //GPIOC
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER &= ~0x3000000;// clear pc12
    GPIOC->MODER |= 0x2000000; //alt funciton for PC12
    GPIOC->AFR[1] &= ~0xf0000; // clear AFR[2]
    GPIOC->AFR[1] |= 0x20000; //set AFR[2] for PC12 for AF2

    //GPIOD
    RCC->AHBENR |= RCC_AHBENR_GPIODEN; //enable GPIOD
    GPIOD -> MODER &= ~0x30; //clear the moder for PD2
    GPIOD -> MODER |= 0x20; //set PD2 to alt function
    GPIOD->AFR[0] &= ~0xf00;//clear alt function for pd2
    GPIOD-> AFR[0] |= 0x200;//set alt function 2 for pd2

    //USART
    RCC -> APB1ENR |= RCC_APB1ENR_USART5EN;
    USART5 -> CR1 &= ~USART_CR1_UE; //turn off the USART
    USART5 -> CR1 &= ~USART_CR1_M; //clear the M0 bit
    USART5 -> CR1 &= ~0x10000000; //clear the M1 bit
    USART5 -> CR2 &= ~(USART_CR2_STOP_0|USART_CR2_STOP_1); //One stop bit
    USART5 -> CR1 &= ~USART_CR1_PCE; //no parity
    USART5 -> CR1 &= ~USART_CR1_OVER8; //oversampling 16x
    USART5->BRR |= 0x1A1;//baud rate to 115.kBaud
    USART5->CR1 |= USART_CR1_RE;
    USART5->CR1 |= USART_CR1_TE;
    USART5 -> CR1 |= USART_CR1_UE;
    while(!(USART5->ISR & USART_ISR_TEACK)&&(USART5->ISR &USART_ISR_REACK));


}

char interrupt_getchar()//Emir Lab10
{
    while(!(fifo_newline(&input_fifo))){
        asm volatile ("wfi");
    }
    char ch = fifo_remove(&input_fifo);
    return ch;
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

int __io_getchar(void) //Emir Lab10
{
    int c =  interrupt_getchar();
    return c;
}

void enable_tty_interrupt() //Emir Lab10
{

        USART5-> CR1 |= USART_CR1_RXNEIE;
        USART5->CR3 |= USART_CR3_DMAR;
        NVIC->ISER[0] |= 1<<USART3_8_IRQn;

        RCC->AHBENR |= RCC_AHBENR_DMA2EN;
        DMA2->RMPCR |= DMA_RMPCR2_CH2_USART5_RX;
        DMA2_Channel2->CCR &= ~DMA_CCR_EN;
        DMA2_Channel2->CMAR = (uint32_t) &(serfifo);
        DMA2_Channel2->CPAR = (uint32_t) &(USART5->RDR);
        DMA2_Channel2->CNDTR = FIFOSIZE;
        DMA2_Channel2->CCR |= DMA_CCR_MINC;
        DMA2_Channel2->CCR |= DMA_CCR_CIRC;
        DMA2_Channel2->CCR |= DMA_CCR_PL;
        DMA2_Channel2->CCR |= DMA_CCR_EN;
}

void USART3_4_5_6_7_8_IRQHandler(void) //Emir Lab10
{
            while(DMA2_Channel2->CNDTR != sizeof serfifo - seroffset) {
                if (!fifo_full(&input_fifo))
                    insert_echo_char(serfifo[seroffset]);
                seroffset = (seroffset + 1) % sizeof serfifo;
            }
        }

int main() {
    init_usart5();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    //initialization fxns
    print_pizza();

    //Set up UI
    quickLCDinit();
    quickLCDinit();
    UIInit();

    int songprog = 0;
    int songdur = 300;

    char ninesounds[10][60] = {"Java", "Python", "C++", "HTML", "SQL","one","wto","three","four","END"};
    //InitBindUI(ninesounds);
    InitNPUI(ninesounds);
    NPUIupdate(songdur, songprog,ninesounds);

    //printcommand("drawfillrect 0 0 200 200 f81f");

    //any additional comands can be manually inputted here
    command_shell();

}

//SPI fxns
void init_spi1_slow() //ZP SPI SD card reader
{

    //disable the SPi before editing it
    //SPI1->CR1 &= ~SPI_CR1_SPE;


    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    //GPIOB->MODER &= ~(GPIO_MODER_MODER3 | GPIO_MODER_MODER4 | GPIO_MODER_MODER5);
    GPIOB->MODER |=  GPIO_MODER_MODER3_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1;

    GPIOB->AFR[0] &=~ 0xFFF000;


    // Set the baud rate divisor to the maximum value to make the SPI baud rate as low as possible.
        //accomplished in the same line as setting to master mode.
    //Set it to Master Mode.
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR | SPI_CR1_BR_1 | SPI_CR1_BR_2;
    //Set the word size to 8-bit.
    SPI1->CR2 |=  SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0;
    //Configure "Software Slave Management" and "Internal Slave Select".
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI ; //!!! what the heck is this
    //Set the "FIFO reception threshold" bit in CR2 so that the SPI channel immediately releases a received 8-bit value.
    SPI1->CR2 |= SPI_CR2_FRXTH;
    //Enable the SPI channel.
    SPI1->CR1 |= SPI_CR1_SPE;

    return;
}

void enable_sdcard() //ZP SPI SD card reader
{   //This function should set PB2 low to enable the SD card.
    GPIOB->BSRR |= GPIO_BSRR_BR_2;
}

void disable_sdcard() //ZP SPI SD card reader
{   //This function should set PB2 high to disable the SD card.
    GPIOB->BSRR |= GPIO_BSRR_BS_2;
}

void init_sdcard_io() //ZP SPI SD card reader
{
    init_spi1_slow();
    GPIOB->MODER |= GPIO_MODER_MODER2_0;
    disable_sdcard();
}

void  sdcard_io_high_speed() //ZP SPI SD card reader
{
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 &= ~SPI_CR1_BR;
    SPI1->CR1 |= SPI_CR1_BR_0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void init_lcd_spi() //ZP SPI LCD screen
{
    //Configure PB8, PB11, and PB14 as GPIO outputs.
    GPIOB->MODER |= GPIO_MODER_MODER8_0 | GPIO_MODER_MODER14_0 | GPIO_MODER_MODER11_0;
    //Call init_spi1_slow() to configure SPI1.
    init_spi1_slow();
    //Call sdcard_io_high_speed() to make SPI1 fast.
    sdcard_io_high_speed();
}

