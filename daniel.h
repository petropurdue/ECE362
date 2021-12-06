#ifndef __DANIEL_H__
#define __DANIEL_H__

//mine
void init_keyPad();
void printCurrDir();
void printFileList();
void emptyFileList();
void printCursor(int a);
//void printString(char * string, int x, int p);
FRESULT scan_files (char* path);

#endif 
