#include "stm32f0xx.h"
#include "commands.h"
#include "stdio.h"
#include "fifo.h"
#include "tty.h"
#include "lcd.h"
#include "ff.h"
#include "ffconf.h"
#include "diskio.h"
#include "daniel.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "wav2.h"



//Definitions
#define FIFOSIZE 16
#define SAMPLES 8000
#define BIND 69
#define PLAY 70
char serfifo[FIFOSIZE];
int seroffset = 0;
int binds[9];

//wav initializations
sWavHeader header;
FIL f;
BYTE buffer[SAMPLES];
uint16_t buffer16[SAMPLES/2];
UINT hs, br, br2;
uint32_t list;
int channels = 1;
FRESULT fres;
int finish = 0;


//Daniel Initializations
int y = 0;
int cursorY = 0;
TCHAR dirList[40][60]; //array of directories
TCHAR fileList[40][60]; //array of files
int selector = 0;
FATFS FatFs;
FIL fil;
FRESULT fres;
TCHAR str[40];
DIR currDir;

//Lab 10 Fxns
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

//Daniel Fxns
void init_keyPad() { // uses ABCD
    RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC -> MODER &= ~0xffff;
    GPIOC -> MODER |= 0x5500;
    GPIOC -> ODR |= 0x10;
    GPIOC -> PUPDR &= ~0xff;
    GPIOC -> PUPDR |= 0xaa;
}

void printCurrDir() {
    LCD_Clear(BLACK);
    fres = f_getcwd(str, 40);  /* Get current directory path */
    y = 0;
    printString(str, 0, 0);
    //fres = scan_files(str);
    printFileList();
}

void printFileList() { // CHANGE SO SELECTOR DOES NOT GO OUT OF BOUNDS
    y = 40;
    printCursor((selector + 2) * 20);
    for(int i = 0; i < 40; i++) {
        printString(fileList[i],0,0);
    }
}

void emptyFileList() {
    for(int i = 0; i < 40; i++) {
        strcpy(fileList[i], " ");
    }
}

void printCursor(int a) {
    //change color to whatever!!
     LCD_DrawFillRectangle(0, 0 + a, 10, 20 + a, RED);
}

void printString(char * string, int x, int p) {
    //int y = 100;
    //int x = 0;
    int checker = 0;
    for(int i = checker; string[i] != '\0'; i++) {
        LCD_DrawChar((x * 10) + (10 + 10), y, WHITE, BLACK, string[i], (4 * 4), 1);

    if (200 <= (10 * x)) {
        x = (0 - 1);
        y += 20;
    }
        x += 1;
    }
    y += (10 + 10);
}

FRESULT scan_files (char* path)
{//populates array with directory

    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;
    selector = 0;
    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "%s", fno.fname);
                //printString(path,0,100);
                strcpy(fileList[selector],fno.fname);
                selector++;
                path[i] = 0;
            } else {                                       /* It is a file. */
                //printString(fno.fname,0,120);
                strcpy(fileList[selector],fno.fname);
                selector++;
            }
        }
        f_closedir(&dir);
    }
    selector = 0;
    return res;
}

int wav_function(char* filename){
//    char* filename = "SINE8.WAV";
   f_open(&f, filename, FA_READ);

   f_read(&f, &header, sizeof(sWavHeader) ,&hs);

   //list chunk check
   if(header.Subchunk2ID != 0x61746164){ //check to make sure that it is data next
       fres = f_lseek(&f, f_tell(&f) - 8); //go back to subchunk2id
       f_read(&f, &list, sizeof(uint32_t), &hs);//read subchunk2id
       while(list != 0x61746164){//while subchunk id not 'data'
           f_read(&f, &list, 4, &hs); //read subchunk size
           f_lseek(&f, f_tell(&f) + list); //skip to end of data
           f_read(&f, &list, sizeof(uint32_t), &hs); //read new subchunk id
       }
       header.Subchunk2ID = list; //make subchunk 'data'
       f_read(&f, &header.Subchunk2Size, sizeof(uint32_t), &hs); //'data' size
   }

   //check number of channels
   channels = header.NumChannels;

   f_read(&f, &buffer, sizeof(buffer), &br);
   setup();

//           f_close(&f);

   return 0;
}

void setup(){
    //timer 6 enable
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->PSC = 1-1;
    TIM6->ARR = (48000000/(header.SampleRate * channels)) -1;
    //TIM6->DIER |= TIM_DIER_UDE;
    TIM6->CR2 |= 0x20;
    //TIM6->CR1 |= TIM_CR1_ARPE;
    TIM6->CR1 |= TIM_CR1_CEN;

    //setting up the DMA
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    if(header.BitsPerSample == 8){
        DMA1_Channel3-> CMAR = (uint32_t)buffer;
        DMA1_Channel3-> CPAR = (uint32_t)&(DAC->DHR8R1);
        DMA1_Channel3->CCR &= ~DMA_CCR_MSIZE |~DMA_CCR_PSIZE;
        DMA1_Channel3->CNDTR = SAMPLES;
    }
    else{
        DMA1_Channel3-> CMAR = (uint32_t)buffer16;
        DMA1_Channel3-> CPAR = (uint32_t)&(DAC->DHR12L1);
        DMA1_Channel3->CCR |= DMA_CCR_MSIZE_0 |DMA_CCR_PSIZE_0;
        DMA1_Channel3->CNDTR = SAMPLES/2;
    }
    DMA1_Channel3->CCR |= DMA_CCR_DIR;
    DMA1_Channel3->CCR |= DMA_CCR_MINC;
    DMA1_Channel3->CCR |= DMA_CCR_CIRC;
    DMA1_Channel3->CCR |= DMA_CCR_HTIE | DMA_CCR_TCIE;
    DMA1_Channel3->CCR |= DMA_CCR_EN;
    NVIC->ISER[0] = 1<<DMA1_Channel2_3_IRQn;

    //Set up the DAC
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR &= ~DAC_CR_EN1;
    DAC->CR &= ~DAC_CR_TSEL1;
    DAC->CR |= DAC_CR_DMAEN1;
    DAC->CR |= DAC_CR_TEN1;
    DAC->CR |= DAC_CR_EN1;


}
void DMA1_CH2_3_DMA2_CH1_2_IRQHandler(){
    if(DMA1->ISR & DMA_ISR_HTIF3){
        DMA1->IFCR |= DMA_IFCR_CHTIF3;
        if(header.BitsPerSample == 8){
            f_read(&f, &buffer, SAMPLES/2, &br);
            if(br != SAMPLES/2){
                        finish = 1;
                    }
        }
        if(header.BitsPerSample == 16){
            f_read(&f, &buffer16, SAMPLES/2, &br);
            if(br != SAMPLES/2){
                finish = 1;
            }
            for(int i = 0; i<SAMPLES/4; i++){
                //buffer[i] ^= 0x80;
                buffer16[i] += 0x8000;
            }
        }
    }
    if(DMA1->ISR & DMA_ISR_TCIF3){
        DMA1->IFCR |= DMA_IFCR_CTCIF3;
        if(header.BitsPerSample == 8){
            f_read(&f, &buffer[SAMPLES/2], SAMPLES/2, &br2);
            if(br != SAMPLES/2){
                        finish = 1;
                    }
        }
        if(header.BitsPerSample == 16){
            f_read(&f, &buffer16[SAMPLES/4], SAMPLES/2, &br2);
            if(br != SAMPLES/2){
                finish = 1;
            }
            for(int i = SAMPLES/4; i<SAMPLES/2; i++){
                //buffer[i] ^= 0x80;
                buffer16[i] += 0x8000;
            }
        }
    }
    if(finish == 1){
        DMA1_Channel3->CCR &= ~DMA_CCR_EN;
    }
}
//#define ZIROFXNS
#if defined(ZIROFXNS)

//Binding fxns
void enable_ports(void) { //Emir lab6
    //initc
    RCC->AHBENR |= 0x00080000;
    GPIOC->MODER &= ~0xffff;
    GPIOC->MODER |= 0x5500;
    GPIOC->PUPDR &= ~0xff;
    GPIOC->PUPDR |= 0xaa;
    RCC->AHBENR |= 0x00040000;
    GPIOB->MODER &= ~0x3fffff;
    GPIOB->MODER |= 0x155555;
}

uint16_t msg[8] = { 0x0000,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700 };
extern const char font[];

void setup_dma(void) { //Emir lab6
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel2->CCR &= ~DMA_CCR_EN;
    DMA1_Channel2->CPAR = (uint32_t) &(GPIOB->ODR);
    DMA1_Channel2->CMAR = (uint32_t) msg;
    DMA1_Channel2->CNDTR = 8;
    DMA1_Channel2->CCR |= DMA_CCR_DIR;
    DMA1_Channel2->CCR |= DMA_CCR_MINC;
    DMA1_Channel2->CCR |= DMA_CCR_PSIZE_0;
    DMA1_Channel2->CCR &= ~DMA_CCR_PSIZE_1;
    DMA1_Channel2->CCR |= DMA_CCR_MSIZE_0;
    DMA1_Channel2->CCR &= ~DMA_CCR_MSIZE_1;
    DMA1_Channel2->CCR |= DMA_CCR_CIRC;

}

void enable_dma(void) {//Emir lab6
    DMA1_Channel2->CCR |= DMA_CCR_EN;
}

void init_tim2(void) {

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = 4800-1;
    TIM2->ARR  = 10-1;
    TIM2->CR1 |= TIM_CR1_CEN;
    TIM2->DIER |= TIM_DIER_UDE;

}

void append_display(char val) {
    for(int i = 0; i <8; i++ )
    {
           char nchar = msg[i+1];
           nchar &= ~(0xf00);
           msg[i] &= ~(0x0ff);
           msg[i] |=nchar;
           //drawstring(0,0,0xFFFF,0x0000,)
    }
    msg[7] &= ~0xff;
    msg[7] |= val;

}

int main() {
    init_usart5();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    //initialization fxns
    print_pizza();
    enable_sdcard();
    int mode = BIND;

    //Set up UI
    quickLCDinit();
    quickLCDinit();
    UIInit();


    int songprog = 0;
    int songdur = 300;



    fres = f_mount(&FatFs, "", 1);
    fres = f_getcwd(str, 40);  /* Get current directory path */
    //printString(str, 0, 0);
    fres = scan_files(str);

    char ninesounds[10][60] = {"Java", "Python", "C++", "HTML", "SQL","one","wto","three","four","END"};
    //Binds UI
    //This one will use cursor and scrolling
    InitBindUI(fileList);
    bindupdate(0,0,0);

    //Binds UI
    //This one will use cursor and scrolling
    InitBindUI(fileList);
    bindupdate(0,0,0);

    //NP UI
   //This one will not use cursor or scrolling
   //InitNPUI(ninesounds);
   //NPUIupdate(songdur, songprog,ninesounds);


    command_shell();

    //Comprehend button presses
    //if # pressed
    if (mode == BIND)
    {
        mode = PLAY;

    }
    else
    {
        mode = BIND;
    }


}

#endif

#define Daniel
#if defined(Daniel)

int main() {


    //init_usart5();
    //init_keyPad();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    //initialization fxns
    //print_pizza();
    quickLCDinit();
    quickLCDinit();
    LCD_Clear(BLACK);
    //printString("hahahahaha gay gay gay", 0, 0);

    //enable_sdcard();
    FATFS *fs = &FatFs;
    fres = f_mount(fs, "", 1);

    if (fres != FR_OK) {
        printString("SD Card did not mount", 0, 20);
    }
    else {
        printString("SD Card Mounted", 0, 40);
    }
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA -> MODER |= GPIO_MODER_MODER4;

    wav_function("NGGYU2.wav");
    while(1);
//    f_close(&f);

    fres = f_getcwd(str, 40);  /* Get current directory path */
    printString(str, 0, 0);
    fres = scan_files(str);
    printFileList();
    int oldvalue = 0;
    for(;;) { // this is bad, should implement external interrupt
        if(((GPIOC->IDR & 0x2) != 0)) { // move down dir (keypad C)
            if(oldvalue == 0) {
                selector++;
                printCurrDir();
                oldvalue = 1;
            }
        }
        else if(((GPIOC->IDR & 0x8) != 0)) { // move up dir (keypad A)
            if(oldvalue == 0) {
                selector--;
                printCurrDir();
                oldvalue = 1;
            }
        }
        // IN THIS ELSE IF RIGHT BELOW THIS IS WHERE TO IMPLEMENT MUSIC PLAYING...
        // AN IF STATEMENT NEEDS TO CHECK IF FILELIST[SELECTOR] IS A FILE OR FOLDER
        // IF ITS A FILE PLAY IT, IF ITS A FOLDER, MOVE INTO IT WHICH IT ALREADY DOES
        else if(((GPIOC->IDR & 0x4) != 0) && selector >= 0) { // enter dir (keypad B)
            if(oldvalue == 0) {
                y = 0;
                LCD_Clear(BLACK);
                fres = f_chdir(fileList[selector]);
                fres = f_getcwd(str, 40);  /* Get current directory path */
                y = 0;
                printString(str, 0, 0);
                emptyFileList();
                fres = scan_files(str);
                printCurrDir();
                oldvalue = 1;
            }
        }
        else if(((GPIOC->IDR & 0x1) != 0)) { // prev dir (keypad D)
            if(oldvalue == 0) {
                y = 0;
                LCD_Clear(BLACK);
                fres = f_chdir("..");
                fres = f_getcwd(str, 40);  /* Get current directory path */
                y = 0;
                printString(str, 0, 0);
                emptyFileList();
                fres = scan_files(str);
                printCurrDir();
                oldvalue = 1;
            }
        }
        else {
            oldvalue = 0;
        }

    }


}

#endif
