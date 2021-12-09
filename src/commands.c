
#include "stm32f0xx.h"
#include "ff.h"
#include "lcd.h"
#include "tty.h"
#include "commands.h"
#include <string.h>
#include <stdio.h>
#include <stddef.h>


// Data structure for the mounted file system.
FATFS fs_storage;

typedef union {
    struct {
        unsigned int bisecond:5; // seconds divided by 2
        unsigned int minute:6;
        unsigned int hour:5;
        unsigned int day:5;
        unsigned int month:4;
        unsigned int year:7;
    };
} fattime_t;

// Current time in the FAT file system format.
static fattime_t fattime;

void set_fattime(int year, int month, int day, int hour, int minute, int second)
{
    fattime_t newtime;
    newtime.year = year - 1980;
    newtime.month = month;
    newtime.day = day;
    newtime.hour = hour;
    newtime.minute = minute;
    newtime.bisecond = second/2;
    int len = sizeof newtime;
    memcpy(&fattime, &newtime, len);
}

void advance_fattime(void)
{
    fattime_t newtime = fattime;
    newtime.bisecond += 1;
    if (newtime.bisecond == 30) {
        newtime.bisecond = 0;
        newtime.minute += 1;
    }
    if (newtime.minute == 60) {
        newtime.minute = 0;
        newtime.hour += 1;
    }
    if (newtime.hour == 24) {
        newtime.hour = 0;
        newtime.day += 1;
    }
    if (newtime.month == 2) {
        if (newtime.day >= 29) {
            int year = newtime.year + 1980;
            if ((year % 1000) == 0) { // we have a leap day in 2000
                if (newtime.day > 29) {
                    newtime.day -= 28;
                    newtime.month = 3;
                }
            } else if ((year % 100) == 0) { // no leap day in 2100
                if (newtime.day > 28)
                newtime.day -= 27;
                newtime.month = 3;
            } else if ((year % 4) == 0) { // leap day for other mod 4 years
                if (newtime.day > 29) {
                    newtime.day -= 28;
                    newtime.month = 3;
                }
            }
        }
    } else if (newtime.month == 9 || newtime.month == 4 || newtime.month == 6 || newtime.month == 10) {
        if (newtime.day == 31) {
            newtime.day -= 30;
            newtime.month += 1;
        }
    } else {
        if (newtime.day == 0) { // cannot advance to 32
            newtime.day = 1;
            newtime.month += 1;
        }
    }
    if (newtime.month == 13) {
        newtime.month = 1;
        newtime.year += 1;
    }

    fattime = newtime;
}

uint32_t get_fattime(void)
{
    return *(uint32_t *)&fattime;
}

void print_error(FRESULT fr, const char *msg)
{
    const char *errs[] = {
            [FR_OK] = "Success",
            [FR_DISK_ERR] = "Hard error in low-level disk I/O layer",
            [FR_INT_ERR] = "Assertion failed",
            [FR_NOT_READY] = "Physical drive cannot work",
            [FR_NO_FILE] = "File not found",
            [FR_NO_PATH] = "Path not found",
            [FR_INVALID_NAME] = "Path name format invalid",
            [FR_DENIED] = "Permision denied",
            [FR_EXIST] = "Prohibited access",
            [FR_INVALID_OBJECT] = "File or directory object invalid",
            [FR_WRITE_PROTECTED] = "Physical drive is write-protected",
            [FR_INVALID_DRIVE] = "Logical drive number is invalid",
            [FR_NOT_ENABLED] = "Volume has no work area",
            [FR_NO_FILESYSTEM] = "Not a valid FAT volume",
            [FR_MKFS_ABORTED] = "f_mkfs aborted",
            [FR_TIMEOUT] = "Unable to obtain grant for object",
            [FR_LOCKED] = "File locked",
            [FR_NOT_ENOUGH_CORE] = "File name is too large",
            [FR_TOO_MANY_OPEN_FILES] = "Too many open files",
            [FR_INVALID_PARAMETER] = "Invalid parameter",
    };
    if (fr < 0 || fr >= sizeof errs / sizeof errs[0])
        printf("%s: Invalid error\n", msg);
    else
        printf("%s: %s\n", msg, errs[fr]);
}

void append(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Specify only one file name to append to.");
        return;
    }
    FIL fil;        /* File object */
    char line[100]; /* Line buffer */
    FRESULT fr;     /* FatFs return code */
    fr = f_open(&fil, argv[1], FA_WRITE|FA_OPEN_EXISTING|FA_OPEN_APPEND);
    if (fr) {
        print_error(fr, argv[1]);
        return;
    }
    printf("To end append, enter a line with a single '.'\n");
    for(;;) {
        fgets(line, sizeof(line)-1, stdin);
        if (line[0] == '.' && line[1] == '\n')
            break;
        int len = strlen(line);
        if (line[len-1] == '\004')
            len -= 1;
        UINT wlen;
        fr = f_write(&fil, (BYTE*)line, len, &wlen);
        if (fr)
            print_error(fr, argv[1]);
    }
    f_close(&fil);
}

void cat(int argc, char *argv[])
{
    for(int i=1; i<argc; i++) {
        FIL fil;        /* File object */
        char line[100]; /* Line buffer */
        FRESULT fr;     /* FatFs return code */

        /* Open a text file */
        fr = f_open(&fil, argv[i], FA_READ);
        if (fr) {
            print_error(fr, argv[i]);
            return;
        }

        /* Read every line and display it */
        while(f_gets(line, sizeof line, &fil))
            printf(line);
        /* Close the file */
        f_close(&fil);
    }
}

void cd(int argc, char *argv[])
{
    if (argc > 2) {
        printf("Too many arguments.");
        return;
    }
    FRESULT res;
    if (argc == 1) {
        res = f_chdir("/");
        if (res)
            print_error(res, "(default path)");
        return;
    }
    res = f_chdir(argv[1]);
    if (res)
        print_error(res, argv[1]);
}

int to_int(char *start, char *end, int base)
{
    int n = 0;
    for( ; start != end; start++)
        n = n * base + (*start - '0');
    return n;
}

static const char *month_name[] = {
        [1] = "Jan",
        [2] = "Feb",
        [3] = "Mar",
        [4] = "Apr",
        [5] = "May",
        [6] = "Jun",
        [7] = "Jul",
        [8] = "Aug",
        [9] = "Sep",
        [10] = "Oct",
        [11] = "Nov",
        [12] = "Dec",
};

void set_fattime(int,int,int,int,int,int);
void date(int argc, char *argv[])
{
    if (argc == 2) {
        char *d = argv[1];
        if (strlen(d) != 14) {
            printf("Date format: YYYYMMDDHHMMSS\n");
            return;
        }
        for(int i=0; i<14; i++)
            if (d[i] < '0' || d[i] > '9') {
                printf("Date format: YYYMMDDHHMMSS\n");
                return;
            }
        int year = to_int(d, &d[4], 10);
        int month = to_int(&d[4], &d[6], 10);
        int day   = to_int(&d[6], &d[8], 10);
        int hour  = to_int(&d[8], &d[10], 10);
        int minute = to_int(&d[10], &d[12], 10);
        int second = to_int(&d[12], &d[14], 10);
        set_fattime(year, month, day, hour, minute, second);
        return;
    }
    int integer = get_fattime();
    fattime_t ft = *(fattime_t *)&integer;
    int year = ft.year + 1980;
    int month = ft.month;
    printf("%d-%s-%02d %02d:%02d:%02d\n",
            year, month_name[month], ft.day, ft.hour, ft.minute, ft.bisecond*2);
}

void dino(int argc, char *argv[])
{
    const char str[] =
    "   .-~~^-.\n"
    " .'  O    \\\n"
    "(_____,    \\\n"
    " `----.     \\\n"
    "       \\     \\\n"
    "        \\     \\\n"
    "         \\     `.             _ _\n"
    "          \\       ~- _ _ - ~       ~ - .\n"
    "           \\                              ~-.\n"
    "            \\                                `.\n"
    "             \\    /               /       \\    \\\n"
    "              `. |         }     |         }    \\\n"
    "                `|        /      |        /       \\\n"
    "                 |       /       |       /          \\\n"
    "                 |      /`- _ _ _|      /.- ~ ^-.     \\\n"
    "                 |     /         |     /          `.    \\\n"
    "                 |     |         |     |             -.   ` . _ _ _ _ _ _\n"
    "                 |_____|         |_____|                ~ . _ _ _ _ _ _ _ >\n";
    puts(str);
}

void input(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Specify only one file name to create.");
        return;
    }
    FIL fil;        /* File object */
    char line[100]; /* Line buffer */
    FRESULT fr;     /* FatFs return code */
    fr = f_open(&fil, argv[1], FA_WRITE|FA_CREATE_NEW);
    if (fr) {
        print_error(fr, argv[1]);
        return;
    }
    printf("To end input, enter a line with a single '.'\n");
    for(;;) {
        fgets(line, sizeof(line)-1, stdin);
        if (line[0] == '.' && line[1] == '\n')
            break;
        int len = strlen(line);
        if (line[len-1] == '\004')
            len -= 1;
        UINT wlen;
        fr = f_write(&fil, (BYTE*)line, len, &wlen);
        if (fr)
            print_error(fr, argv[1]);
    }
    f_close(&fil);
}

void lcd_init(int argc, char *argv[])
{
    LCD_Setup();
}

void ls(int argc, char *argv[])
{
    FRESULT res;
    DIR dir;
    static FILINFO fno;
    const char *path = "";
    int info = 0;
    int i=1;
    do {
        if (argv[i][0] == '-') {
            for(char *c=&argv[i][1]; *c; c++)
                if (*c == 'l')
                    info=1;
            if (i+1 < argc) {
                i += 1;
                continue;
            }
        } else {
            path = argv[i];
        }

        res = f_opendir(&dir, path);                       /* Open the directory */
        if (res != FR_OK) {
            print_error(res, argv[1]);
            return;
        }
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (info) {
                printf("%04d-%s-%02d %02d:%02d:%02d %6d %c%c%c%c%c ",
                        (fno.fdate >> 9) + 1980,
                        month_name[fno.fdate >> 5 & 15],
                        fno.fdate & 31,
                        fno.ftime >> 11,
                        fno.ftime >> 5 & 63,
                        (fno.ftime & 31) * 2,
                        fno.fsize,
                        (fno.fattrib & AM_DIR) ? 'D' : '-',
                        (fno.fattrib & AM_RDO) ? 'R' : '-',
                        (fno.fattrib & AM_HID) ? 'H' : '-',
                        (fno.fattrib & AM_SYS) ? 'S' : '-',
                        (fno.fattrib & AM_ARC) ? 'A' : '-');
            }
            if (path[0] != '\0')
                printf("%s/%s\n", path, fno.fname);
            else
                printf("%s\n", fno.fname);
        }
        f_closedir(&dir);
        i += 1;
    } while(i<argc);
}

void mkdir(int argc, char *argv[])
{
    for(int i=1; i<argc; i++) {
        FRESULT res = f_mkdir(argv[i]);
        if (res != FR_OK) {
            print_error(res, argv[i]);
            return;
        }
    }
}

void mount(int argc, char *argv[])
{
    FATFS *fs = &fs_storage;
    if (fs->id != 0) {
        print_error(FR_DISK_ERR, "Already mounted.");
        return;
    }
    f_mount(fs, "", 1);
}

void pwd(int argc, char *argv[])
{
    char line[100];
    FRESULT res = f_getcwd(line, sizeof line);
    if (res != FR_OK)
        print_error(res, "pwd");
    else
        printf("%s\n", line);
}

void rm(int argc, char *argv[])
{
    FRESULT res;
    for(int i=1; i<argc; i++) {
        res = f_unlink(argv[i]);
        if (res != FR_OK)
            print_error(res, argv[i]);
    }
}

void shout(int argc, char *argv[])
{
    char arr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz0123456789\n";
    for(int i; i<1000; i++)
        for(int c=0; c<sizeof arr; c++)
            putchar(arr[c]);
}

void clear(int argc, char *argv[])
{
    int value = 0;
    if (argc == 2)
        value = strtoul(argv[1], 0, 16);
    LCD_Clear(value);
}

void drawline(int argc, char *argv[])
{
    if (argc != 6) {
        printf("Wrong number of arguments: line x1 y1 x2 y2 color");
        return;
    }
    int x1 = strtoul(argv[1], 0, 10);
    int y1 = strtoul(argv[2], 0, 10);
    int x2 = strtoul(argv[3], 0, 10);
    int y2 = strtoul(argv[4], 0, 10);
    int c = strtoul(argv[5], 0, 16);
    LCD_DrawLine(x1,y1,x2,y2,c);
}

void UIInit()
{
    //SetBG
        int x1 = 0;
        int y1 = 0;
        int x2 = 240;
        int y2 = 320;
        int c = 0;
        LCD_DrawFillRectangle(x1,y1,x2,y2,c);
}

void InitBindUI(char ninesounds[10][60]) //zp UI initialization
{
    LCD_DrawString(0, 0, 0xF800, 0000, "Epic ECE369 .wav Player" , 16, 0);
    LCD_DrawString(0, 16, 0xF800, 0000, "Set Binds Mode" , 16, 0);
    drawstring(0, 2, 0xFFFF, 0000, "------------------------------");
    drawstring(0, 3, 0xFFFF, 0000, "           SD VIEW:");



    drawstring(0,14,0xF800,0000,"------------------------------");
    drawstring(0,15,0X07FF,0000,"Controls:");
    drawstring(0,16,0X07FF,0000,"A: Cursor Up   B: Cursor Down");
    drawstring(0,17,0X07FF,0000,"C: Enter Dir.  D: Exit Dir.");
    drawstring(0,18,0x07FF,0000,"Press 1-9 to bind to the");
    drawstring(0,19,0x07FF,0000,"selected sound");
}

void InitNPUI(char boundsounds[10][60]) //zp UI initialization
{
    LCD_DrawString(0, 0, 0xF800, 0000, "Epic ECE362 .wav Player" , 16, 0);
    LCD_DrawString(0, 16, 0xF800, 0000, "Now Playing" , 16, 0);
    drawstring(0, 2, 0xFFFF, 0000, "------------------------------");
    drawstring(0, 3, 0xFFFF, 0000, "%songname%");
    drawstring(0, 4, 0xFFFF, 0000, "Song Progress:      /");

    //Song binds UI:
    drawstring(0, 6, 0xFFFF, 0000, "0:");
    drawstring(0, 7, 0xFFFF, 0000, "1:");
    drawstring(0, 8, 0xFFFF, 0000, "2:");
    drawstring(0, 9, 0xFFFF, 0000, "3:");
    drawstring(0, 10, 0xFFFF, 0000, "4:");
    drawstring(0, 11, 0xFFFF, 0000, "5:");
    drawstring(0, 12, 0xFFFF, 0000, "6:");
    drawstring(0, 13, 0xFFFF, 0000, "7:");
    drawstring(0, 14, 0xFFFF, 0000, "8:");
    drawstring(0, 15, 0xFFFF, 0000, "9:");

    //Bound songs list
    for (int i = 6; i < 6+10; i++)
    {
        drawstring(2,i,0x001F,0000,boundsounds[i-6]);
    }
    //Bottom UI
    drawstring(0,16,0xF800,0000,"------------------------------");
    drawstring(0,17,0x07FF,0000,"Press 1-9 to play the bound");
    drawstring(0,18,0x07FF,0000,"sounds");
}

void NPUIupdate(int songdur,int songprog,char boundsounds[10][60])
{
    //Song slider calculations
    double percent = (double)songprog / (double)songdur;
    percent *= 10;
    char timtot[6];
    sprintf(timtot,"%02d:%02d",songdur/60,songdur- (songdur/60)*60);
    char timeelapsed[6];
    sprintf(timeelapsed,"%02d:%02d",songprog/60,songprog- (songprog/60)*60);

    //Print necessities
    drawstring(15, 4, 0xFFFF, 0000, timeelapsed);
    drawstring(21, 4, 0xFFFF, 0000, timtot);
    drawstring(8,5,0x07FF, 0000,"[-----------]");
    drawstring(9 + percent,5,0x07FF, 0000,"|");


}



/*
void writecommand(char * inputarr) //zp command integreation
{
    int stringlength = strlen(inputarr);
    //take an input string and append \0 to i, then run parse_command on it
    char line [100];
    line[9] = '\0';
    parse_command(line);
}
*/


void drawfolder(char * foldername)
{
    char str[9];
    strncpy(str,foldername,9);
    LCD_DrawString(19*8, 3*16, 0xF81F, 0, str, 16, 0);
}
void drawstring(int xcord, int ycord, int FGcol, int BGcol, char * stringtoprint)
{
    //max string length is 30 characters. It does not matter if it is a long character (ex lll vs www).
    //maximum number of lines is 19

    //If string length is longer than 30, make it end in ".."
    xcord = xcord * 8;
    ycord = ycord * 16;
    int stringlength = strlen(stringtoprint);
    //printf("\nString length is %d\n",stringlength);
    //printf("pizza");
    char  shortstring[30];
    strncpy(shortstring,stringtoprint,29);
    LCD_DrawString(xcord, ycord, FGcol, BGcol, shortstring, 16, 0);
    if (stringlength > 30)
    {
        //printf("\nlong string detected!\n");
        LCD_DrawString(224, ycord, FGcol, BGcol, "..", 16, 0);
    }
}

void drawrect(int argc, char *argv[])
{
    if (argc != 6) {
        printf("Wrong number of arguments: drawrect x1 y1 x2 y2 color");
        return;
    }
    int x1 = strtoul(argv[1], 0, 10);
    int y1 = strtoul(argv[2], 0, 10);
    int x2 = strtoul(argv[3], 0, 10);
    int y2 = strtoul(argv[4], 0, 10);
    int c = strtoul(argv[5], 0, 16);
    LCD_DrawRectangle(x1,y1,x2,y2,c);
}

void drawfillrect(int argc, char *argv[])
{
    if (argc != 6) {
        printf("Wrong number of arguments: drawfillrect x1 y1 x2 y2 color");
        return;
    }
    int x1 = strtoul(argv[1], 0, 10);
    int y1 = strtoul(argv[2], 0, 10);
    int x2 = strtoul(argv[3], 0, 10);
    int y2 = strtoul(argv[4], 0, 10);
    int c = strtoul(argv[5], 0, 16);
    LCD_DrawFillRectangle(x1,y1,x2,y2,c);
}

struct commands_t cmds[] = {
        { "append", append },
        { "cat", cat },
        { "cd", cd },
        { "date", date },
        { "dino", dino },
        { "input", input },
        { "lcd_init", lcd_init },
        { "ls", ls },
        { "mkdir", mkdir },
        { "mount", mount },
        { "pwd", pwd },
        { "rm", rm },
        { "shout", shout },
        { "clear",    clear },
        { "drawline", drawline },
        { "drawrect", drawrect },
        { "drawfillrect", drawfillrect },
};

// A weak definition that can be overridden by a better one.
__attribute((weak)) struct commands_t usercmds[] = {
        { 0, 0 }
};

void exec(int argc, char *argv[])
{
    //for(int i=0; i<argc; i++)
    //    printf("%d: %s\n", i, argv[i]);
    for(int i=0; usercmds[i].cmd != 0; i++)
        if (strcmp(usercmds[i].cmd, argv[0]) == 0) {
            usercmds[i].fn(argc, argv);
            return;
        }
    for(int i=0; i<sizeof cmds/sizeof cmds[0]; i++)
        if (strcmp(cmds[i].cmd, argv[0]) == 0) {
            cmds[i].fn(argc, argv);
            return;
        }
    printf("%s: No such command.\n", argv[0]);
}

void parse_command(char *c)
{
    char *argv[20];
    int argc=0;
    int skipspace=1;
    for(; *c; c++) {
        if (skipspace) {
            if (*c != ' ' && *c != '\t') {
                argv[argc++] = c;
                skipspace = 0;
            }
        } else {
            if (*c == ' ' || *c == '\t') {
                *c = '\0';
                skipspace=1;
            }
        }
    }
    if (argc > 0) {
        argv[argc] = "";
        exec(argc, argv);
    }
}

static void insert_echo_string(const char *s)
{
    // This puts a string into the *input* stream, so it can be edited.
    while(*s)
        insert_echo_char(*s++);
}

void print_pizza(void)
{
    printf("\n========================\n");
    printf("new execution!\n");
    printf("========================\n");
}

void quickLCDinit(void) //ZP lcd stuff
{
    //This function will initialize the LCD screen. Make sure it is called twice.
    char line[100];
    line[0] = 'l';
    line[1] = 'c';
    line[2] = 'd';
    line[3] = '_';
    line[4] = 'i';
    line[5] = 'n';
    line[6] = 'i';
    line[7] = 't';
    line[8] = '\0';
    line[9] = '\0';
    parse_command(line);
}

void printcommand(char * arr) //zp terminal execution
{
    //this will print any command you send over to the terminal. Call with format >printcommand("command I want to be printed");
    //This function is ONLY useful for testing; it will not be integrated into the final product.
    int i = 0;
    while(arr[i])
    {
        printf("%c",arr[i]);
        i++;
    }
}

void command_shell(void)
{
    printf("\nEnter current "); fflush(stdout);
    insert_echo_string("date 20211108103000");
    char line[100];

    fgets(line, 99, stdin);
    line[99] = '\0';
    int len = strlen(line);
    if (line[len-1] == '\n')
        line[len-1] = '\0';
    parse_command(line);

    puts("This is the STM32 command shell.");
    puts("Type 'mount' before trying any file system commands.");
    puts("Type 'lcd_init' before trying any draw commands.");
    for(;;) {
        printf("> ");
        fgets(line, 99, stdin);
        line[99] = '\0';
        len = strlen(line);
        if (line[len-1] == '\n')
            line[len-1] = '\0';
        parse_command(line);
    }
}
