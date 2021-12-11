#ifndef COMMAND_H
#define COMMAND_H

#include <stdio.h>
#include <ctype.h>
#include <queue>
#include <vector>
#include <set>
#include <utility>
#include <time.h>
#include "atpg.h"

#define MAXLINE 81               /* Input buffer size */
#define MAXNAME 31               /* File name size */

#define ZERO (unsigned int)0
#define ONE (unsigned int)15
#define D (unsigned int)12
#define D_BAR (unsigned int)3
#define X (unsigned int)9

#define Upcase(x) ((isalpha(x) && islower(x))? toupper(x) : (x))
#define Lowcase(x) ((isalpha(x) && isupper(x))? tolower(x) : (x))

/*-------------------- Command Definitions --------------------*/
enum e_com {READ, PC, HELP, QUIT, LEV, LOGICSIM, RFL, DFS, PFS, RTG, DALG, PODEM, ATPG_DET, ATPG};        /* command list */
enum e_state {EXEC, CKTLD};         /* Gstate values */
enum e_ntype {GATE, PI, FB, PO};    /* column 1 of circuit format */
enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND};  /* gate types */

struct cmdstruc {
   char name[MAXNAME];        /* command syntax */
   int (*fptr)(char *);             /* function pointer of the commands */
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
   struct n_struc *next_node;                  /* Address of the next node */
   std::set<std::pair<int, int> > *fault_list;    /* fault list */
   unsigned long parallel_vector;      /* used for pfs to store the values for each fault circuit, the MSB is for the good circuit */
   unsigned int fault_value;                   /* For ATPG */
   std::vector<unsigned int> value_stack;      /* For ATPG */
   std::vector<int> previous_imply_dir;    /* For ATPG */
   unsigned fin_level;                         /* used by lev() */  
} NSTRUC;                     

#define NUMFUNCS 14
int cread(char *), pc(char *), help(char *), quit(char *), lev(char *), logicsim(char *), rfl(char *), dfs(char *), pfs(char *), rtg(char *);
void read_cktname(char *), clear(), allocate(), read_inputs(char *, std::vector<int> *, std::queue<int> *), sim(NSTRUC *), propagate_fault(NSTRUC *), reset_fault_list();
int fault_bit_comparison(int, int);
void dfs_single(std::queue<int> *, std::set<std::pair<int, int> > *);
void rtg_single(FILE *, std::queue<int> *, clock_t, int);
void initialize_pi(NSTRUC*, int);
void check_faults(NSTRUC *, std::vector<std::pair<int, int> >);
void pfs_sim(NSTRUC *);
void report_faults(std::set<std::pair<int, int> > *, std::vector<std::pair<int, int> > *);
int dalg(char *cp), podem(char *cp), atpg_det(char *cp), atpg(char *cp);
void clear_mem();

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
extern NSTRUC **map_num_node;
extern struct n_struc *starting_node;
extern int *map_indx_num;
extern char netlist_file_name[81];
extern int atpg_flag;  //flag from atpg function calls   Zeming
extern std::vector<int> input_test_vector_node;      //Zeming
extern std::vector<int> input_test_vector_value;     //Zeming
extern std::vector<int> fault_dic;                   //2*n fault, 0 = invalid, 1 = valid  Zeming
extern std::vector<int> fault_dic_node;              //Zeming for dfs store the node number


/*-------------------- Helping Function Definitions --------------------*/


#endif