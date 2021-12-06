#include "stm32f0xx.h"
#include "commands.h"
#include "stdio.h"
#include "string.h"
#include "fifo.h"
#include "tty.h"
#include "lcd.h"
#include "ff.h"
#define FIFOSIZE 16
char serfifo[FIFOSIZE];
int seroffset = 0;

void disable_sdcard();
void enable_sdcard();
void printString(char * string, int x, int p);
void printCursor(int a);
void printFileList();
FRESULT scan_files (char* path);

int y = 0;
int cursorY = 0;
TCHAR dirList[40][40];
TCHAR fileList[40][40];
int selector = 0;
FATFS FatFs;
FIL fil;
FRESULT fres;
TCHAR str[40];
DIR currDir;
static FILINFO fno;

void init_keyPad() { // uses 456B *HELPED*
    RCC -> AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC -> MODER &= ~0x0000ffff;
    GPIOC -> MODER |= 0x00000055;
    GPIOC -> ODR |= 0x0000000c;
    GPIOC -> PUPDR &= ~0x0000ff00;
    GPIOC -> PUPDR |= 0x0000aa00;
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
	 LCD_DrawFillRectangle(0, 0 + a, 10, 20 + a, BLUE);
}

void printString(char * string, int x, int p) { // *HELPED*
    //int y = 100;
    //int x = 0;
    for(int i = 0; string[i] != '\0'; i++) {
        LCD_DrawChar((10 + 10) + (x * 10), y, WHITE, BLACK, string[i], (4 * 4), 1);
        //nano_wait(50000000);
        if ((x * 10) >= 200) {
            y += 20;
            x = (0 - 1);
        }
        x++;
    }
    y += (10 + 10);
}

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

FRESULT scan_files (char* path) {
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
                //res = scan_files(path);                    /* Enter the directory */
                //if (res != FR_OK) break;
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

int main() {
    //init_usart5();
	init_keyPad();
    enable_tty_interrupt();
    setbuf(stdin,0);
    setbuf(stdout,0);
    setbuf(stderr,0);

    //initialization fxns
    //print_pizza();
    quickLCDinit();
    quickLCDinit();
    //printcommand("my custom function is working");
    //drawstring(0, 0, 0xFFFF, 0x0000, "All I can think of is fuck off but I don't think that's appropriate :(", 16, 0);
    //drawstring(0, 0, 0xFFFF, 0x0000, "wwwwwwwwwwwwwwwwwwwwwwwwwwwwaw", 16, 0);
    LCD_Clear(BLACK);
    //printString("hahahahaha gay gay gay", 0, 0);

    enable_sdcard();
    fres = f_mount(&FatFs, "", 1);
    if (fres != FR_OK) {
    	printString("SD Card did not mount", 0, 20);
    }
    else {
    	printString("SD Card Mounted", 0, 40);
    }
    fres = f_getcwd(str, 40);  /* Get current directory path */
    printString(str, 0, 0);
    fres = scan_files(str);
    //f_mount(NULL, "", 0); // unmount sdcard
    //disable_sdcard();
    printFileList();
    //any additional comands can be manually inputted here
    //command_shell();
    int oldvalue = 0;
    for(;;) { // this is bad, should implement external interrupt
    	if(((GPIOC->IDR & 0x00000020) != 0)) { // move down dir
    		if(oldvalue == 0) {
				selector++;
				printCurrDir();
				oldvalue = 1;
    		}
    	}
    	else if(((GPIOC->IDR & 0x00000080) != 0)) { // move up dir
    		if(oldvalue == 0) {
				selector--;
				printCurrDir();
				oldvalue = 1;
    		}
    	}
    	// IN THIS ELSE IF RIGHT BELOW THIS IS WHERE TO IMPLEMENT MUSIC PLAYING...
    	// AN IF STATEMENT NEEDS TO CHECK IF FILELIST[SELECTOR] IS A FILE OR FOLDER
    	// IF ITS A FILE PLAY IT, IF ITS A FOLDER, MOVE INTO IT WHICH IT ALREADY DOES
    	else if(((GPIOC->IDR & 0x00000040) != 0) && selector >= 0) { // enter dir
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
    	else if(((GPIOC->IDR & 0x00000010) != 0)) { // prev dir
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

