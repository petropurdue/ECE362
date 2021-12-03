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
void drawstring(int xcord, int ycord, int FGcol, int BGcol, char * stringtoprint,int fontsize, int mode);
void quickLCDinit(void);

#endif /* __COMMANDS_H_ */
