/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include "commands.h"

//===========================================================================
// 2.1 Initialize the USART
//===========================================================================
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

//===========================================================================
// Main and supporting functions
//===========================================================================
//#define STEP21
        #if defined(STEP21)
        int main(void)//Emir Lab10
        {
            init_usart5();
            for(;;) {
                while (!(USART5->ISR & USART_ISR_RXNE)) { }
                char c = USART5->RDR;
                while(!(USART5->ISR & USART_ISR_TXE)) { }
                USART5->TDR = c;
            }
        }
        #endif

//#define STEP22
#if defined(STEP22)
#include "stdio.h"
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
     while (!(USART5->ISR & USART_ISR_RXNE)) { }
     char c = USART5->RDR;
     if (c=='\r'){
         c= '\n';
     }
     __io_putchar(c);
     return c;
}

int main() //Emir Lab10
{
    init_usart5();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    printf("Enter your name: ");
    char name[80];
    fgets(name, 80, stdin);
    printf("Your name is %s", name);
    printf("Type any characters.\n");
    for(;;) {
        char c = getchar();
        putchar(c);
    }
}
#endif

//#define STEP23
#if defined(STEP23)
#include "stdio.h"
#include "fifo.h"
#include "tty.h"
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
//     while (!(USART5->ISR & USART_ISR_RXNE)) { }
//     char c = USART5->RDR;
//     if (c=='\r'){
//         c= '\n';
//     }
//     __io_putchar(c);
    int c = line_buffer_getchar();
    return c;
}

int main() //Emir Lab10
    {
    init_usart5();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    printf("Enter your name: ");
    char name[80];
    fgets(name, 80, stdin);
    printf("Your name is %s", name);
    printf("Type any characters.\n");
    for(;;) {
        char c = getchar();
        putchar(c);
    }
}
#endif

//#define STEP24

#include "stdio.h"
#include "fifo.h"
#include "tty.h"
#define FIFOSIZE 16
char serfifo[FIFOSIZE];
int seroffset = 0;

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
//     while (!(USART5->ISR & USART_ISR_RXNE)) { }
//     char c = USART5->RDR;
//     if (c=='\r'){
//         c= '\n';
//     }
//     __io_putchar(c);
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
        DMA2_Channel2->CMAR = &serfifo;
        DMA2_Channel2->CPAR = &USART5->RDR;
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
#if defined(STEP24)
int main() //Emir lab10
{
    init_usart5();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    printf("Enter your name: ");
    char name[80];
    fgets(name, 80, stdin);
    printf("Your name is %s", name);
    printf("Type any characters.\n");
    for(;;) {
        char c = getchar();
        putchar(c);
    }
}
#endif

#define STEP5
#if defined(STEP5)
#include "stdio.h"
#include "fifo.h"
#include "tty.h"


//SPI fxns
void init_spi1_slow() //ZP SPI SD card reader
{
    // Set the baud rate divisor to the maximum value to make the SPI baud rate as low as possible.
        //accomplished in the same line as setting to master mode.
    //Set it to Master Mode.
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR | SPI_CR1_BR_1 | SPI_CR1_BR_2;
    //Set the word size to 8-bit.
    SPI1->CR2 &=  ~SPI_CR2_DS_3 | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS_0;
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

    //set pin as output mode
    GPIOB->MODER &= ~GPIO_MODER_MODER2;
    GPIOB->MODER |= GPIO_MODER_MODER2_0;

    //set to output push/pull
    GPIOB->OTYPER &=~0b1000;

    //set output pull up pull down
    GPIOB->PUPDR &=~0b11000000;
    GPIOB->PUPDR |= 0b10000000;

    return;
}

void disable_sdcard() //ZP SPI SD card reader
{   //This function should set PB2 high to disable the SD card.

    //set pin as output mode
    GPIOB->MODER &=~0b11000000;
    GPIOB->MODER |= 0b01000000;

    //set to output push/pull
    GPIOB->OTYPER &=~0b1000;

    //set output to pull up
    GPIOB->PUPDR &=~0b11000000;
    GPIOB->PUPDR |= 0b01000000;

    return;
}

void init_sdcard_io() //ZP SPI SD card reader
{
    init_spi1_slow();
    //Configure PB2 as an output.
    GPIOB->MODER &=~0b11000000;
    GPIOB->MODER |= 0b01000000;

    disable_sdcard();

    return;
}

void  sdcard_io_high_speed() //ZP SPI SD card reader
{
    /*Disable the SPI1 channel.
     * The correct disable procedure is (except when receive only mode is used):
        1. Wait until FTLVL[1:0] = 00 (no more data to transmit).
        2. Wait until BSY=0 (the last data frame is processed).
        3. Disable the SPI (SPE=0).
        4. Read data until FRLVL[1:0] = 00 (read all the received data)
     */
    //but I'm lazy uwu
    SPI1->CR1 &= ~0b1000000;

    //Set the SPI1 Baud Rate register so that the clock rate is 12 MHz.
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_BR | SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE; //set BR to 001, which makes it 4
    SPI1->CR2 = SPI_CR2_SSOE | SPI_CR2_NSSP | SPI_CR2_DS_2 | SPI_CR2_DS_1 | SPI_CR2_DS; //8 bit word size cause I'm edgy
    SPI1->CR1 |= SPI_CR1_SPE;

    //Re-enable the SPI1 channel.
    SPI1->CR1 |= 0b1000000;
    return;
}

void init_lcd_spi()
{
    //Configure PB8, PB11, and PB14 as GPIO outputs.
    GPIOB->MODER &=~ 0b1100001100001100000000000000; //=0x4104000
    GPIOB->MODER |=  0b0100000100000100000000000000;
    init_spi1_slow();
    sdcard_io_high_speed();
    return;
}

int main() {
    init_usart5();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);
    command_shell();
}

#endif
