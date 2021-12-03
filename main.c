#include "stm32f0xx.h"


//===========================================================================
// 2.1 Initialize USART
//===========================================================================
void init_usart5(void){
    //Enable the RCC clocks to GPIOC and GPIOD.
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN|RCC_AHBENR_GPIODEN;

    //Do all the steps necessary to configure pin PC12 to be routed to USART5_TX.
    GPIOC->MODER &= ~GPIO_MODER_MODER12;
    GPIOC->MODER |= GPIO_MODER_MODER12_1;
    GPIOC->AFR[1] &= ~0xf<<(4*4);
    GPIOC->AFR[1] |= 0x2<<(4*4);

    //Do all the steps necessary to configure pin PD2 to be routed to USART5_RX.
    GPIOD->MODER &= ~GPIO_MODER_MODER2;
    GPIOD->MODER |= GPIO_MODER_MODER2_1;
    GPIOD->AFR[0] &= ~0xf<<(4*2);
    GPIOD->AFR[0] = 0x2<<(4*2);

    //Enable the RCC clock to the USART5 peripheral.
    RCC->APB1ENR |= RCC_APB1ENR_USART5EN;

    //Configure USART5 as follows:
    //    (First, disable it by turning off its UE bit.)
    USART5->CR1 &= ~USART_CR1_UE;

    //    Set a word size of 8 bits.
    USART5->CR1 &= ~(USART_CR1_M|0x10000000);

    //    Set it for one stop bit.
    USART5->CR2 &= ~USART_CR2_STOP;

    //    Set it for no parity.
    USART5->CR1 &= ~USART_CR1_PCE;

    //    Use 16x oversampling.
    USART5->CR1 &= ~USART_CR1_OVER8;

    //    Use a baud rate of 115200 (115.2 kbaud). Refer to table 96 of the Family Reference Manual.
    USART5->BRR = 0x1A1;

    //    Enable the transmitter and the receiver by setting the TE and RE bits.
    USART5->CR1 |= USART_CR1_TE|USART_CR1_RE;

    //    Enable the USART.
    USART5->CR1 |= USART_CR1_UE;

    //    Finally, you should wait for the TE and RE bits to be acknowledged by checking that TEACK and REACK bits are both set in the ISR. This indicates that the USART is ready to transmit and receive.
    while((USART5->ISR & USART_ISR_TEACK)==0 || (USART5->ISR & USART_ISR_REACK)==0);
}

//#define STEP21
#if defined(STEP21)
int main(void)
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
#include <stdio.h>

int __io_putchar(int c) {
    if(c == '\n'){
        __io_putchar('\r');
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = c;
        return c;
    }
    while(!(USART5->ISR & USART_ISR_TXE)) { }
    USART5->TDR = c;
    return c;
}

int __io_getchar(void) {
     while (!(USART5->ISR & USART_ISR_RXNE)) { }
     char c = USART5->RDR;
     if(c == '\r'){
         __io_putchar(c);
         return('\n');
     }
     __io_putchar(c);
     return c;
}

int main() {
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
#include "fifo.h"
#include "tty.h"
#include <stdio.h>

int __io_putchar(int c) {
    if(c == '\n'){
        __io_putchar('\r');
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = c;
        return c;
    }
    while(!(USART5->ISR & USART_ISR_TXE)) { }
    USART5->TDR = c;
    return c;
}

int __io_getchar(void) {
    return(line_buffer_getchar());
}

int main() {
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

#define STEP24
#if defined(STEP24)
#include "fifo.h"
#include "tty.h"
#include <stdio.h>
#define FIFOSIZE 16
//The first will be the circular buffer that DMA deposits characters into.
//The second will keep track of the offset in the buffer of the next character read from it.
char serfifo[FIFOSIZE];
int seroffset = 0;

void enable_tty_interrupt(){
    //Raise an interrupt every time the receive data register becomes not empty.
    USART5->CR1 |= USART_CR1_RXNEIE;
    //Remember to set the proper bit in the NVIC ISER as well.
    //Note that the name of the bit to set is difficult to determine. It is USART3_8_IRQn.
    NVIC->ISER[0] = 1<<USART3_8_IRQn;

    //Trigger a DMA operation every time the receive data register becomes not empty.
    USART5->CR3 |= USART_CR3_DMAR;

    //enable the RCC clock for DMA Controller 2
    //Table 33 shows that Channel 2 of Controller 2 can be used with USART5_RX.
    //To do so, however, the "remap" register must be set to allow it.
    RCC->AHBENR |= RCC_AHBENR_DMA2EN;
    DMA2->RMPCR |= DMA_RMPCR2_CH2_USART5_RX;
    DMA2_Channel2->CCR &= ~DMA_CCR_EN;  // First make sure DMA is turned off

    //CMAR should be set to the address of serfifo.
    DMA2_Channel2->CMAR = serfifo;

    //CPAR should be set to the address of the USART5 RDR.
    DMA2_Channel2->CPAR = (uint32_t) &(USART5->RDR);

    //CNDTR should be set to FIFOSIZE.
    DMA2_Channel2->CNDTR = FIFOSIZE;

    //The DIRection of copying should be from peripheral to memory.
    DMA2_Channel2->CCR &= ~DMA_CCR_DIR;

    //Neither the total-completion nor the half-transfer interrupt should be enabled.
    DMA2_Channel2->CCR &= ~DMA_CCR_HTIE;
    DMA2_Channel2->CCR &= ~DMA_CCR_TCIE;

    //Both the MSIZE and the PSIZE should be set for 8 bits.
    DMA2_Channel2->CCR &= ~DMA_CCR_MSIZE;
    DMA2_Channel2->CCR &= ~DMA_CCR_PSIZE;

    //MINC should be set to increment the CMAR.
    DMA2_Channel2->CCR |= DMA_CCR_MINC;

    //PINC should not be set so that CPAR always points at the USART5 RDR.
    DMA2_Channel2->CCR &= ~DMA_CCR_PINC;

    //Enable CIRCular transfers.
    DMA2_Channel2->CCR |= DMA_CCR_CIRC;

    //Do not enable MEM2MEM transfers.
    DMA2_Channel2->CCR &= ~DMA_CCR_MEM2MEM;

    //Set the Priority Level to highest.
    DMA2_Channel2->CCR |= DMA_CCR_PL;

    //Finally, make sure that the channel is enabled for operation.
    DMA2_Channel2->CCR |= DMA_CCR_EN;
}

int interrupt_getchar(){
    while(fifo_newline(&input_fifo) == 0){
        asm volatile ("wfi"); // wait for an interrupt
    }

    //remove the first character from the fifo and return it
    char ch = fifo_remove(&input_fifo);
    return ch;
}

int __io_putchar(int c) {
    if(c == '\n'){
        __io_putchar('\r');
        while(!(USART5->ISR & USART_ISR_TXE));
        USART5->TDR = c;
        return c;
    }
    while(!(USART5->ISR & USART_ISR_TXE)) { }
    USART5->TDR = c;
    return c;
}

int __io_getchar(void) {
    return(interrupt_getchar());
}

void USART3_4_5_6_7_8_IRQHandler(void) {
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
