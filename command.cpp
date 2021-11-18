/*=======================================================================
  A simple parser for "self" format

  The circuit format (called "self" format) is based on outputs of
  a ISCAS 85 format translator written by Dr. Sandeep Gupta.
  The format uses only integers to represent circuit information.
  The format is as follows:

1        2        3        4           5           6 ...
------   -------  -------  ---------   --------    --------
0 GATE   outline  0 IPT    #_of_fout   #_of_fin    inlines
                  1 BRCH
                  2 XOR(currently not implemented)
                  3 OR
                  4 NOR
                  5 NOT
                  6 NAND
                  7 AND

1 PI     outline  0        #_of_fout   0

2 FB     outline  1 BRCH   inline

3 PO     outline  2 - 7    0           #_of_fin    inlines




                                    Author: Chihang Chen
                                    Date: 9/16/94

=======================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <queue>
#include <vector>
#include <set>
#include "command.h"


struct cmdstruc command[NUMFUNCS] = {
   {"READ", cread, EXEC},
   {"PC", pc, CKTLD},
   {"HELP", help, EXEC},
   {"QUIT", quit, EXEC},
   {"LEV", lev, CKTLD},
   {"LOGICSIM", logicsim, CKTLD},
   {"RFL", rfl, CKTLD},
};

enum e_state Gstate = EXEC;

NSTRUC *Node;                   /* dynamic array of nodes */
NSTRUC **Pinput;                /* pointer to array of primary inputs */
NSTRUC **Poutput;               /* pointer to array of primary outputs */
int Nnodes;                     /* number of nodes */
int Ngates;                     /* number of gates */
int Npi;                        /* number of primary inputs */
int Npo;                        /* number of primary outputs */
int Done = 0;                   /* status bit to terminate program */
char Cktname[MAXLINE];          /* circuit name */


//int cread(){}；
//void pc(char *){};
// void help(char *){};
// void quit(char *){};
// void lev(char *){};
// void logicsim(char *){};

/*-----------------------------------------------------------------------
input: circuit description file name
output: nothing
called by: main
description:
  This routine reads in the circuit description file and set up all the
  required data structure. It first checks if the file exists, then it
  sets up a mapping table, determines the number of nodes, PI's and PO's,
  allocates dynamic data arrays, and fills in the structural information
  of the circuit. In the ISCAS circuit description format, only upstream
  nodes are specified. Downstream nodes are implied. However, to facilitate
  forward implication, they are also built up in the data structure.
  To have the maximal flexibility, three passes through the circuit file
  are required: the first pass to determine the size of the mapping table
  , the second to fill in the mapping table, and the third to actually
  set up the circuit information. These procedures may be simplified in
  the future.
-----------------------------------------------------------------------*/
void cread(char *cp) {
   char buf[MAXLINE];
   /*------------------
    * tp: type, nd: id 
    * tbl: lookup table, key: id, value: line number 
   ------------------*/ 
   int ntbl, *tbl, i, j, k, nd, tp, fo, fi, ni = 0, no = 0;
   FILE *fd;
   NSTRUC *np;

   sscanf(cp, "%s", buf);
   if((fd = fopen(buf,"r")) == NULL) {
      printf("File %s does not exist!\n", buf);
      return;
   }

   /* read circuit name */
   read_cktname(buf);

   if(Gstate >= CKTLD) clear();
   Nnodes = Ngates = Npi = Npo = ntbl = 0;
   while(fgets(buf, MAXLINE, fd) != NULL) {
      if(sscanf(buf,"%d %d", &tp, &nd) == 2) {
         if(ntbl < nd) ntbl = nd;
         Nnodes ++;
         if(tp == PI) Npi++;
         else if(tp == PO) Npo++;
      }
   }
   tbl = (int *) malloc(++ntbl * sizeof(int));

   fseek(fd, 0L, 0);
   i = 0;
   while(fgets(buf, MAXLINE, fd) != NULL) {
      if(sscanf(buf,"%d %d", &tp, &nd) == 2) tbl[nd] = i++;
   }
   allocate();

   fseek(fd, 0L, 0);
   while(fscanf(fd, "%d %d", &tp, &nd) != EOF) {
      np = &Node[tbl[nd]];
      np->num = nd;
      if(tp == PI) Pinput[ni++] = np;
      else if(tp == PO) Poutput[no++] = np;
      switch(tp) {
         case PI:
         case PO:
         case GATE:
            fscanf(fd, "%d %d %d", &np->type, &np->fout, &np->fin);
            break;
         
         case FB:
            np->fout = np->fin = 1;
            fscanf(fd, "%d", &np->type);
            break;

         default:
            printf("Unknown node type!\n");
            exit(-1);
         }
      np->unodes = (NSTRUC **) malloc(np->fin * sizeof(NSTRUC *));
      np->dnodes = (NSTRUC **) malloc(np->fout * sizeof(NSTRUC *));
      for(i = 0; i < np->fin; i++) {
         fscanf(fd, "%d", &nd);
         np->unodes[i] = &Node[tbl[nd]];
         }
      for(i = 0; i < np->fout; np->dnodes[i++] = NULL);
      }
   for(i = 0; i < Nnodes; i++) {
      for(j = 0; j < Node[i].fin; j++) {
         np = Node[i].unodes[j];
         k = 0;
         while(np->dnodes[k] != NULL) k++;
         np->dnodes[k] = &Node[i];
         if (np->fout <= k) np->fout = k + 1;
         }
      }
   fclose(fd);
   Gstate = CKTLD;
   printf("==> OK\n");
}

/*-----------------------------------------------------------------------
input: nothing (accept any input as cp without using it)
output: nothing
called by: main
description:
  The routine prints out the circuit description from previous READ command.
-----------------------------------------------------------------------*/
void pc(char *cp) {
   int i, j;
   NSTRUC *np;
   char *gname(int);
   
   printf(" Node   Type \tIn     \t\t\tOut    \n");
   printf("------ ------\t-------\t\t\t-------\n");
   for(i = 0; i<Nnodes; i++) {
      np = &Node[i];
      printf("\t\t\t\t\t");
      for(j = 0; j<np->fout; j++) printf("%d ",np->dnodes[j]->num);
      printf("\r%5d  %s\t", np->num, gname(np->type));
      for(j = 0; j<np->fin; j++) printf("%d ",np->unodes[j]->num);
      printf("\n");
   }
   printf("Primary inputs:  ");
   for(i = 0; i<Npi; i++) printf("%d ",Pinput[i]->num);
   printf("\n");
   printf("Primary outputs: ");
   for(i = 0; i<Npo; i++) printf("%d ",Poutput[i]->num);
   printf("\n\n");
   printf("Number of nodes = %d\n", Nnodes);
   printf("Number of primary inputs = %d\n", Npi);
   printf("Number of primary outputs = %d\n", Npo);
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
  The routine prints ot help inormation for each command.
-----------------------------------------------------------------------*/
void help(char *cp) {
   printf("READ filename - ");
   printf("read in circuit file and creat all data structures\n");
   printf("PC - ");
   printf("print circuit information\n");
   printf("LEV - ");
   printf("levelize the circuit\n");
   printf("LOGICSIM infilename outfilename - ");
   printf("read in the input file and write down simulation results of outputs\n");
   printf("HELP - ");
   printf("print this help information\n");
   printf("QUIT - ");
   printf("stop and exit\n");
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
  Set Done to 1 which will terminates the program.
-----------------------------------------------------------------------*/
void quit(char *cp) {
   Done = 1;
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
  The routine levelizes the circuit.
  If the command is used as "lev", the default output file will be
  [circuit name].txt.
  If the command is used as "lev [filename]", the output will be
  written into [filename].
-----------------------------------------------------------------------*/
void lev(char *cp) {
   char buf[MAXLINE];
   int i = 0, j = 0;
   std::queue<NSTRUC*> qnodes;
   // Qnode *front, *rear;
   NSTRUC *np;
   int maxUpLevel;
   char txtname[MAXLINE];
   FILE *fp = NULL;

   // if the command is given as "lev",
   // parse the default file name
   strcpy(txtname, Cktname);
   strcat(txtname, ".txt");
   fp = fopen(txtname, "w");

   // if the command is given as "lev [output file name]",
   // the output file name will be loaded with read_cktname()
   if (sscanf(cp, "%s", buf) == 1) read_cktname(buf);
   
   Ngates = 0;
   printf("%s\n", Cktname);
   printf("#PI: %d\n", Npi);
   printf("#PO: %d\n", Npo);
   printf("#Nodes: %d\n", Nnodes);
   
   for (i = 0; i < Nnodes; i++)
      if (Node[i].type >= XOR && Node[i].type <= AND) Ngates++;
   printf("#Gates: %d\n", Ngates);
   
   fprintf(fp, "%s\n", Cktname);
   fprintf(fp, "#PI: %d\n", Npi);
   fprintf(fp, "#PO: %d\n", Npo);
   fprintf(fp, "#Nodes: %d\n", Nnodes);
   fprintf(fp, "#Gates: %d\n", Ngates);
   
   // front = (Qnode *) malloc(sizeof(Qnode));
   // front->next = NULL;
   // rear = front;
   for (i = 0; i < Nnodes; i++) Node[i].level = -1;
   for (i = 0; i < Npi; i++) {
      Pinput[i]->level = 0;
      for (j = 0; j < Pinput[i]->fout; j++) {
         // rear = pushQueue(rear, Pinput[i]->dnodes[j]);
         qnodes.push(Pinput[i]->dnodes[j]);
         //printf(fp, "push node %d\n", Pinput[i]->dnodes[j]->num);
      }
   }
   while(!qnodes.empty()) {
      // np = popQueue(front, &rear);
      np = qnodes.front();
      qnodes.pop();
      //printf(fp, "pop node %d\n", np->num);
      maxUpLevel = 0;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->level == -1) { maxUpLevel = -1; break; }
         if (np->unodes[i]->level > maxUpLevel) maxUpLevel = np->unodes[i]->level;
      }
      if (maxUpLevel == -1) continue;
      np->level = maxUpLevel + 1;
      for (i = 0; i < np->fout; i++) { 
         if (np->dnodes[i]->level > 0) continue;
         // rear = pushQueue(rear, np->dnodes[i]); 
         qnodes.push(np->dnodes[i]);
         //printf(fp, "push node %d\n", np->dnodes[i]->num); 
      }
   }
   for (i = 0; i < Nnodes; i++) {
      printf("%d %d\n", Node[i].num, Node[i].level);
      fprintf(fp, "%d %d\n", Node[i].num, Node[i].level);
   }
   fclose(fp);
}

/*-----------------------------------------------------------------------
input: input file name, output file name
output: nothing
called by: main 
description:
  Simulate with given inputs.
  The command should be used as:
  "logicsim [input file name] [output file name]"
  It will read the values of inputs from [input file name] and 
  write the values of outputs into [output file name].
-----------------------------------------------------------------------*/
void logicsim(char *cp) {
   char infile[MAXLINE], outfile[MAXLINE];
   std::queue<NSTRUC*> qnodes;
   std::vector<int> pi_idx;
   std::queue<int> in_value;
   // Qnode *front, *rear;
   NSTRUC *np;
   int level_idx;
   FILE *fp = NULL;
   level_idx = 1;

   // initialize the node queue
   // front = (Qnode *) malloc(sizeof(Qnode));
   // front->next = NULL;
   // rear = front;

   // the levelization information is required for simulation
   lev((char *)(""));

   // parse the input and output filenames
   if (sscanf(cp, "%s %s", &infile, &outfile) != 2) {printf("Incorrect input\n"); return;}
   printf("%s\n", infile);
   printf("%s\n", outfile);
   fp = fopen(outfile, "w");

   // read inputs
   read_inputs(infile, &pi_idx, &in_value);

   // print output port indices
   for (int i = 0; i < Npo; i++) {
      if (i == 0)
         fprintf(fp, "%d", Poutput[i]->num);
      else 
         fprintf(fp, ",%d", Poutput[i]->num);
   }
   fprintf(fp, "\n");


   int loop = in_value.size() / pi_idx.size(); // loop is the number of input patterns

   for (int l = 0; l < loop; l++) { // simulate for each input
      // assign primary input values
      for (int i = 0; i < pi_idx.size(); i++) {
         for (int j = 0; j < Npi; j++) {
            if (Pinput[j]->num == pi_idx[i]) {
               Pinput[j]->value = in_value.front();
               printf("input %d has value %d\n", Pinput[j]->num, Pinput[j]->value);
               in_value.pop();
               break;
            }
         }
      }

      // simulate values of each wires level by level according to their levelization
      // first push all down nodes of primary inputs which are at level 1
      // since the values of inputs are already decided, their values can be simulated
      for (int i = 0; i < Npi; i++) 
         for (int j = 0; j < Pinput[i]->fout; j++) 
            if (Pinput[i]->dnodes[j]->level == 1)
               // rear = pushQueue(rear, Pinput[i]->dnodes[j]);
               qnodes.push(Pinput[i]->dnodes[j]);

      while(!qnodes.empty()) {
         // np = popQueue(front, &rear);
         np = qnodes.front();
         qnodes.pop();
         sim(np);
         // for each down node, if its level is exactly 1 larger than the current node,
         // it will be pushed into the queue. Otherwise, it will be pushed into the queue
         // later, since not all of its inputs' values are decided yet.
         for (int i = 0; i < np->fout; i++) {
            if (np->dnodes[i]->level == (np->level + 1))
               // rear = pushQueue(rear, np->dnodes[i]);
               qnodes.push(np->dnodes[i]);
         }
      }
      for (int i = 0; i < Npo; i++) {
         if (i == 0)
            fprintf(fp, "%d", Poutput[i]->value);
         else
            fprintf(fp, ",%d", Poutput[i]->value);
      }
      if (l != loop - 1)
         fprintf(fp, "\n");
   }

   // write into [output file name]
   fclose(fp);
}

// /*-----------------------------------------------------------------------
// input: input file name, output file name
// output: nothing
// called by: main 
// description:
//   Simulate with given inputs.
//   The command should be used as:
//   "logicsim [input file name] [output file name]"
//   It will read the values of inputs from [input file name] and 
//   write the values of outputs into [output file name].
// -----------------------------------------------------------------------*/
// void logicsim(char *cp) {
//    int i, j;
//    char infile[MAXLINE], outfile[MAXLINE];
//    std::queue<NSTRUC*> qnodes;
//    // Qnode *front, *rear;
//    NSTRUC *np;
//    int level_idx;
//    FILE *fp = NULL;
//    level_idx = 1;

//    // initialize the node queue
//    // front = (Qnode *) malloc(sizeof(Qnode));
//    // front->next = NULL;
//    // rear = front;

//    // the levelization information is required for simulation
//    lev((char *)(""));

//    // parse the input and output filenames
//    if (sscanf(cp, "%s %s", &infile, &outfile) != 2) {printf("Incorrect input\n"); return;}
//    printf("%s\n", infile);
//    printf("%s\n", outfile);
//    fp = fopen(outfile, "w");

//    // read inputs
//    read_inputs(infile);

//    // simulate values of each wires level by level according to their levelization
//    // first push all down nodes of primary inputs which are at level 1
//    // since the values of inputs are already decided, their values can be simulated
//    for (i = 0; i < Npi; i++) 
//       for (j = 0; j < Pinput[i]->fout; j++) 
//          if (Pinput[i]->dnodes[j]->level == 1)
//             // rear = pushQueue(rear, Pinput[i]->dnodes[j]);
//             qnodes.push(Pinput[i]->dnodes[j]);

//    while(!qnodes.empty()) {
//       // np = popQueue(front, &rear);
//       np = qnodes.front();
//       qnodes.pop();
//       sim(np);
//       // for each down node, if its level is exactly 1 larger than the current node,
//       // it will be pushed into the queue. Otherwise, it will be pushed into the queue
//       // later, since not all of its inputs' values are decided yet.
//       for (i = 0; i < np->fout; i++) {
//          if (np->dnodes[i]->level == (np->level + 1))
//             // rear = pushQueue(rear, np->dnodes[i]);
//             qnodes.push(np->dnodes[i]);
//       }
//    }
//    // write into [output file name]
//    for (i = 0; i < Npo; i++) {
//       fprintf(fp, "%d,%d\n", Poutput[i]->num, Poutput[i]->value);
//    }
//    fclose(fp);
// }

/*-----------------------------------------------------------------------
input: output file name
output: nothing
called by: main 
description:
  generates a reduced list of faults required for the ATPG of the circuit.
  In this function we only consider the check point theorem as a method 
  to reduce the number of faults.
-----------------------------------------------------------------------*/
void rfl(char *cp) {
   std::set<NSTRUC*> pi_branch;
   std::set<NSTRUC*>::iterator it;
   FILE *fp = NULL;
   char outfile[MAXLINE];


   for (int i = 0; i < Npi; i++) {
      pi_branch.insert(Pinput[i]);
   }
   for (int i = 0; i < Nnodes; i++) {
      if (Node[i].fout > 1) {
         for (int j = 0; j < Node[i].fout; j++) {
            pi_branch.insert(Node[i].dnodes[j]);
         }
      }
   }

   // parse the output filename
   if (sscanf(cp, "%s", &outfile) != 1) {printf("Incorrect input\n"); return;}
   printf("%s\n", outfile);
   fp = fopen(outfile, "w");

   for (it = pi_branch.begin(); it != pi_branch.end(); ++it) {
      fprintf(fp, "%d@0\n", (*it)->num);
      fprintf(fp, "%d@1\n", (*it)->num);
   }
   fclose(fp);
}

// /*======================================================================*/

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: cread
description:
  This routine clears the memory space occupied by the previous circuit
  before reading in new one. It frees up the dynamic arrays Node.unodes,
  Node.dnodes, Node.flist, Node, Pinput, Poutput, and Tap.
-----------------------------------------------------------------------*/
void clear() {
   int i;

   for(i = 0; i<Nnodes; i++) {
      free(Node[i].unodes);
      free(Node[i].dnodes);
   }
   free(Node);
   free(Pinput);
   free(Poutput);
   Gstate = EXEC;
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: cread
description:
  This routine allocatess the memory space required by the circuit
  description data structure. It allocates the dynamic arrays Node,
  Node.flist, Node, Pinput, Poutput, and Tap. It also set the default
  tap selection and the fanin and fanout to 0.
-----------------------------------------------------------------------*/
void allocate() {
   int i;

   Node = (NSTRUC *) malloc(Nnodes * sizeof(NSTRUC));
   Pinput = (NSTRUC **) malloc(Npi * sizeof(NSTRUC *));
   Poutput = (NSTRUC **) malloc(Npo * sizeof(NSTRUC *));
   for(i = 0; i<Nnodes; i++) {
      Node[i].indx = i;
      Node[i].fin = Node[i].fout = 0;
   }
}

/*-----------------------------------------------------------------------
input: gate type
output: string of the gate type
called by: pc
description:
  The routine receive an integer gate type and return the gate type in
  character string.
-----------------------------------------------------------------------*/
char *gname(int tp) {
   switch(tp) {
      case 0: return((char*)("PI"));
      case 1: return((char*)("BRANCH"));
      case 2: return((char*)("XOR"));
      case 3: return((char*)("OR"));
      case 4: return((char*)("NOR"));
      case 5: return((char*)("NOT"));
      case 6: return((char*)("NAND"));
      case 7: return((char*)("AND"));
   }
}

/*-----------------------------------------------------------------------
input: circuit filepath
output: nothing
called by: cread, lev
description:
  The routine receives the file path and read the circuit name.
-----------------------------------------------------------------------*/
void read_cktname(char *buf) {
   char *sp;      /* start point of circuit name */
   if ((sp = strrchr(buf, '/')) != NULL) {
      strncpy(Cktname, sp + 1, strlen(sp) - 5);
      Cktname[strlen(sp) - 4] = '\0';
   }
   else {
      strncpy(Cktname, buf, strlen(buf) - 4);
      Cktname[strlen(buf) - 4] = '\0';
   }
}

/*-----------------------------------------------------------------------
input: input file name
output: nothing
called by: logicsim 
description:
  Read input values.
-----------------------------------------------------------------------*/
void read_inputs(char *cp, std::vector<int> * pi_idx, std::queue<int> * in_value) {
   FILE *fd;
   int idx, value;
   char temp;
   int i;
   printf("input file name: %s\n", cp);
   if ((fd = fopen(cp, "r")) == NULL) {
      printf("File %s does not exist!\n", cp);
      return;
   }
   while (fscanf(fd, "%d%c", &idx, &temp) != EOF) {
      pi_idx->push_back(idx);
      if (temp == '\n' || temp == '\r')
         break;
   }
   while (fscanf(fd, "%d%c", &value, &temp) != EOF) {
      in_value->push(value);
   }
   fclose(fd);
}

// /*-----------------------------------------------------------------------
// input: input file name
// output: nothing
// called by: logicsim 
// description:
//   Read input values.
// -----------------------------------------------------------------------*/
// void read_inputs(char *cp) {
//    FILE *fd;
//    int idx, value;
//    int i;
//    printf("input file name: %s\n", cp);
//    if ((fd = fopen(cp, "r")) == NULL) {
//       printf("File %s does not exist!\n", cp);
//       return;
//    }
//    while (fscanf(fd, "%d,%d", &idx, &value) != EOF) {
//       for (i = 0; i < Npi; i++) {
//          if (Pinput[i]->num == idx) {
//             Pinput[i]->value = value;
//             printf("input %d has value %d\n", idx, value);
//             break;
//          }
//       }
//    }
//    fclose(fd);
// }

/*-----------------------------------------------------------------------
input: node
output: nothing
called by: main 
description:
  Simulate the value of the given node.
-----------------------------------------------------------------------*/
void sim(NSTRUC *np) {
   int i;
   NSTRUC *input;
   
   if (np->type == NOT) {
      np->value = 0;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 0) {
            np->value = 1;
            break;
         }
      }
   }
   else if (np->type == BRCH) {
      np->value = 0;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 1) {
            np->value = 1;
            break;
         }
      }
   }
   else if (np->type == XOR) {
      np->value = 0;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 1) {
            np->value = (np->value == 0) ? 1:0;
         }
      }
   }
   else if (np->type == OR) {
      np->value = 0;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 1) {
            np->value = 1;
            break;
         }
      }
   }
   else if (np->type == NOR) {
      np->value = 1;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 1) {
            np->value = 0;
            break;
         }
      }
   }
   else if (np->type == NAND) {
      np->value = 0;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 0) {
            np->value = 1;
            break;
         }
      }
   }
   else if (np->type == AND) {
      np->value = 1;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 0) {
            np->value = 0;
            break;
         }
      }
   }
   //printf("node %d has type: %d value: %d level: %d\n", np->num, np->type, np->value, np->level);
}

/*========================= End of program ============================*/

