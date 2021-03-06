#ifndef __COMMANDS_H__
#define __COMMANDS_H__

struct commands_t {
    const char *cmd;
    void      (*fn)(int argc, char *argv[]);
};

//built in
void command_shell(void);

//mine
void print_pizza(void);
void printcommand(char * arr);
void drawstring(int xcord, int ycord, int FGcol, int BGcol, char * stringtoprint);
void quickLCDinit(void);
void UIInit(void);
void writecommand(char * inputarr);
void InitBindUI(char ninesounds[10][60]) ;
void drawfolder(char * foldername);
void NPUIupdate(int songdur,int songprog);

#endif /* __COMMANDS_H_ */
