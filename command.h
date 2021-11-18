#ifndef COMMAND_H
#define COMMAND_H

#include <ctype.h>
#include <queue>

#define MAXLINE 81               /* Input buffer size */
#define MAXNAME 31               /* File name size */

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))

/*-------------------- Command Definitions --------------------*/
enum e_com {READ, PC, HELP, QUIT, LEV, LOGICSIM, RFL};        /* command list */
enum e_state {EXEC, CKTLD};         /* Gstate values */
enum e_ntype {GATE, PI, FB, PO};    /* column 1 of circuit format */
enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND};  /* gate types */

struct cmdstruc {
   char name[MAXNAME];        /* command syntax */
   void (*fptr)(char *);             /* function pointer of the commands */
   enum e_state state;        /* execution state sequence */
};

typedef struct n_struc {
   unsigned indx;             /* node index(from 0 to NumOfLine - 1 */
   unsigned num;              /* line number(May be different from indx */
   enum e_gtype type;         /* gate type */
   unsigned fin;              /* number of fanins */
   unsigned fout;             /* number of fanouts */
   struct n_struc **unodes;   /* pointer to array of up nodes */
   struct n_struc **dnodes;   /* pointer to array of down nodes */
   int level;                 /* level of the gate output */
   int value;                 /* value of the gate output */
} NSTRUC;                     

#define NUMFUNCS 7
void cread(char *), pc(char *), help(char *), quit(char *), lev(char *), logicsim(char *), rfl(char *);
void read_cktname(char *), clear(), allocate(), read_inputs(char *, std::vector<int> *, std::queue<int> *), sim(NSTRUC *);

extern struct cmdstruc command[NUMFUNCS];

/*-------------------- Global Variables --------------------*/
extern enum e_state Gstate;     /* global exectution sequence */
extern NSTRUC *Node;                   /* dynamic array of nodes */
extern NSTRUC **Pinput;                /* pointer to array of primary inputs */
extern NSTRUC **Poutput;               /* pointer to array of primary outputs */
extern int Nnodes;                     /* number of nodes */
extern int Ngates;                     /* number of gates */
extern int Npi;                        /* number of primary inputs */
extern int Npo;                        /* number of primary outputs */
extern int Done;                   /* status bit to terminate program */
extern char Cktname[MAXLINE];          /* circuit name */

/*-------------------- Helping Function Definitions --------------------*/


#endif