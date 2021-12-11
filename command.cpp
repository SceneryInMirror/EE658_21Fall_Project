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
#include <utility>
#include "command.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <math.h>
#include <time.h>

struct cmdstruc command[NUMFUNCS] = {
   {"READ", cread, EXEC},
   {"PC", pc, CKTLD},
   {"HELP", help, EXEC},
   {"QUIT", quit, EXEC},
   {"LEV", lev, CKTLD},
   {"LOGICSIM", logicsim, CKTLD},
   {"RFL", rfl, CKTLD},
   {"DFS", dfs, CKTLD},
   {"PFS", pfs, CKTLD},
   {"RTG", rtg, CKTLD},
   {"DALG", dalg, CKTLD}, 
   {"PODEM", podem, CKTLD}, 
   {"ATPG_DET", atpg_det, EXEC},
   {"ATPG", atpg, EXEC},
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
NSTRUC **map_num_node;
struct n_struc *starting_node;
int *map_indx_num;
char netlist_file_name[81];
int atpg_flag = 0;  //flag from atpg function calls   Zeming

std::vector<int> input_test_vector_node;      //Zeming
std::vector<int> input_test_vector_value;     //Zeming
std::vector<int> fault_dic;                   //2*n fault, 0 = invalid, 1 = valid  Zeming
std::vector<int> fault_dic_node;              //Zeming for dfs store the node number

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
int cread(char *cp) {
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
      return 0;
   }

   /* read circuit name */
   read_cktname(buf);

   if(Gstate >= CKTLD) clear_mem();
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
   return 0;
}

/*-----------------------------------------------------------------------
input: nothing (accept any input as cp without using it)
output: nothing
called by: main
description:
  The routine prints out the circuit description from previous READ command.
-----------------------------------------------------------------------*/
int pc(char *cp) {
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
   return 0;
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
  The routine prints ot help inormation for each command.
-----------------------------------------------------------------------*/
int help(char *cp) {
   printf("READ filename - ");
   printf("read in circuit file and creat all data structures\n");
   printf("PC - ");
   printf("print circuit information\n");
   printf("LEV - ");
   printf("levelize the circuit\n");
   printf("LOGICSIM inFile outFile - ");
   printf("read in the input file and write down simulation results of outputs\n");
   printf("RFL outFile - ");
   printf("write down the reduced fault list\n");
   printf("DFS inFile outFile - ");
   printf("read in the input file and write down deductive fault list\n");
   printf("PFS testPatternFile faultListFile outFile - ");
   printf("read in the test patterns and fault list, and report detectable faults\n");
   printf("RTG ntot ntfcr testPatternFile fcFile - ");
   printf("generate random test patterns and calculate the FC\n");
   printf("HELP - ");
   printf("print this help information\n");
   printf("QUIT - ");
   printf("stop and exit\n");
   return 0;
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: main 
description:
  Set Done to 1 which will terminates the program.
-----------------------------------------------------------------------*/
int quit(char *cp) {
   Done = 1;
   return 0;
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
int lev(char *cp)
{
    int ind;
    FILE* lev_report;
    // char *report_name = new char;
    char report_name[20];
    for (ind = 0; ind < 20; ind++){
      report_name[ind] = netlist_file_name[ind];
    }
    // report_name = netlist_file_name;
    strcat(report_name, "_level.txt");
    if (!atpg_flag)
    {
      lev_report = fopen(report_name, "w");
    }
    
    NSTRUC *np, *ipt_np, *opt_np;
    int fault_ind;
    for (ind = 0; ind < Nnodes; ind++)  // Initialization
    {
        np = &Node[ind];
        np->level = -1;
        np->next_node = NULL;
        np->fin_level = np->fin;  // This parameter is only useful to levelization
    }

    int head_count = 0, tail_count = 0, ind_dnode, ind_unode, max_level;
    NSTRUC ** unleveled;
    unleveled = (NSTRUC **) malloc(Nnodes * sizeof(NSTRUC *));
    int max_num = 0;
    for (ind = 0; ind < Npi; ind++)  // Include all PI nodes to the unleveled array.
    {
        if (ind == 0){
            starting_node = Pinput[ind];
        }
        np = Pinput[ind];
        unleveled[tail_count++] = np;
    }
    while (head_count < tail_count)
    {
        np = unleveled[head_count++];
        if (np->num > max_num){
          max_num = np->num;
        }
        if (np->level == -1)
        {
            for (ind_dnode = 0; ind_dnode < np->fout; ind_dnode++)  // Include unleveled nodes from their fan-out nodes to the array.
            {
                opt_np = np->dnodes[ind_dnode];
                opt_np->fin_level -= 1;
                if (opt_np->fin_level == 0)
                {
                    unleveled[tail_count++] = opt_np;
                }
            }
            if (np->type == IPT) {np->level = 0;}  // PI node has level 0.
            else if (np->type == BRCH)  // Branch
            {
                ipt_np = np->unodes[0];
                np->level = ipt_np->level + 1;
            }
            else
            {
                max_level = 0;
                for (ind_unode = 0; ind_unode < np->fin; ind_unode++)
                {
                    ipt_np = np->unodes[ind_unode];
                    if (max_level < ipt_np->level) {max_level = ipt_np->level;}
                }
                np->level = max_level + 1;
            }
        }
        // printf("%d,%d\n", np->num, np->level);
        if (head_count < tail_count){
            np->next_node = unleveled[head_count];
            // printf("%d %d \n", np->num, np->next_node->num);
        }
        if (!atpg_flag)
        {
          fprintf(lev_report, "%d,%d\n", np->num, np->level);
        }
        
    }
    if (!atpg_flag)
    {
      fclose(lev_report);
    }
    

    // Create a map from num to node
    map_num_node = new NSTRUC* [max_num + 1];
    map_indx_num = new int[Nnodes];
    struct n_struc *current_node;
    current_node = starting_node;
    while(1){
      map_num_node[current_node->num] = current_node;
      map_indx_num[current_node->indx] = current_node->num;
      if (current_node->next_node == NULL){
        break;
      }
      current_node = current_node->next_node;
    }
    if (!atpg_flag){
      printf("Levelization=> OK\n");
    }
    return 0;
}
// int lev(char *cp) {
//    char buf[MAXLINE];
//    int i = 0, j = 0;
//    std::queue<NSTRUC*> qnodes;
//    NSTRUC *np;
//    int maxUpLevel, maxNum = 0;
//    char txtname[MAXLINE];
//    FILE *fp = NULL;

//    // if the command is given as "lev",
//    // parse the default file name
//    strcpy(txtname, Cktname);
//    strcat(txtname, ".txt");
//    fp = fopen(txtname, "w");

//    // if the command is given as "lev [output file name]",
//    // the output file name will be loaded with read_cktname()
//    if (sscanf(cp, "%s", buf) == 1) read_cktname(buf);
   
//    Ngates = 0;
//    printf("%s\n", Cktname);
//    printf("#PI: %d\n", Npi);
//    printf("#PO: %d\n", Npo);
//    printf("#Nodes: %d\n", Nnodes);
   
//    for (i = 0; i < Nnodes; i++)
//       if (Node[i].type >= XOR && Node[i].type <= AND) Ngates++;
//    printf("#Gates: %d\n", Ngates);
   
//    fprintf(fp, "%s\n", Cktname);
//    fprintf(fp, "#PI: %d\n", Npi);
//    fprintf(fp, "#PO: %d\n", Npo);
//    fprintf(fp, "#Nodes: %d\n", Nnodes);
//    fprintf(fp, "#Gates: %d\n", Ngates);
   
//    // front = (Qnode *) malloc(sizeof(Qnode));
//    // front->next = NULL;
//    // rear = front;
//    for (i = 0; i < Nnodes; i++) Node[i].level = -1;
//    for (i = 0; i < Npi; i++) {
//       Pinput[i]->level = 0;
//       if (i > 0) Pinput[i - 1]->next_node = Pinput[i];
//       for (j = 0; j < Pinput[i]->fout; j++) {
//          qnodes.push(Pinput[i]->dnodes[j]);
//       }
//    }
//    Pinput[Npi - 1]->next_node = qnodes.front();
//    while(!qnodes.empty()) {
//       np = qnodes.front();
//       qnodes.pop();
//       if (!qnodes.empty())
//          np->next_node = qnodes.front();
//       else 
//          np->next_node = NULL;
//       maxUpLevel = 0;
//       for (i = 0; i < np->fin; i++) {
//          if (np->unodes[i]->level == -1) { maxUpLevel = -1; break; }
//          if (np->unodes[i]->level > maxUpLevel) maxUpLevel = np->unodes[i]->level;
//       }
//       if (maxUpLevel == -1) continue;
//       np->level = maxUpLevel + 1;
//       for (i = 0; i < np->fout; i++) { 
//          if (np->dnodes[i]->level > 0) continue;
//          qnodes.push(np->dnodes[i]);
//       }
//    }
//    for (i = 0; i < Nnodes; i++) {
//       printf("%d %d\n", Node[i].num, Node[i].level);
//       fprintf(fp, "%d %d\n", Node[i].num, Node[i].level);
//    }

//    for (int i = 0; i < Nnodes; i++)
//       if (Node[i].num > maxNum) maxNum = Node[i].num;
//    starting_node = Pinput[0];
//    map_indx_num = new int[Nnodes];
//    map_num_node = new NSTRUC* [maxNum + 1];
//    struct n_struc *current_node;
//    current_node = starting_node;
//    for (int i = 0; i < Nnodes; i++) {
//       map_indx_num[current_node->indx] = current_node->num;
//       map_num_node[Node[i].num] = &Node[i];
//    }

//    fclose(fp);
//    return 0;
// }

/*-----------------------------------------------------------------------
input: input file name, output file name
output: nothing
called by: main, dfs 
description:
  Simulate with given inputs.
  The command should be used as:
  "logicsim [input file name] [output file name]"
  It will read the values of inputs from [input file name] and 
  write the values of outputs into [output file name].
-----------------------------------------------------------------------*/
int logicsim(char *cp) {
   char infile[MAXLINE], outfile[MAXLINE];
   std::queue<NSTRUC*> qnodes;
   std::vector<int> pi_idx;
   std::queue<int> in_value;
   NSTRUC *np;
   int level_idx;
   FILE *fp = NULL;
   level_idx = 1;

   // the levelization information is required for simulation
   lev((char *)(""));

   // parse the input and output filenames
   if (sscanf(cp, "%s %s", &infile, &outfile) != 2) {printf("Incorrect input\n"); return 0;}
   printf("input file name: %s\n", infile);
   printf("output file name: %s\n", outfile);
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
               //printf("input %d has value %d\n", Pinput[j]->num, Pinput[j]->value);
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
               qnodes.push(Pinput[i]->dnodes[j]);

      while(!qnodes.empty()) {
         np = qnodes.front();
         qnodes.pop();
         sim(np);
         // for each down node, if its level is exactly 1 larger than the current node,
         // it will be pushed into the queue. Otherwise, it will be pushed into the queue
         // later, since not all of its inputs' values are decided yet.
         for (int i = 0; i < np->fout; i++) {
            if (np->dnodes[i]->level == (np->level + 1))
               qnodes.push(np->dnodes[i]);
         }
      }
      for (int i = 0; i < Npo; i++) {
         if (i == 0) {
            if (Poutput[i]->value == -1) {
               fprintf(fp, "%c", 'X');
            }
            else {
               fprintf(fp, "%d", Poutput[i]->value);
            }
         }
         else {
            if (Poutput[i]->value == -1) {
               fprintf(fp, ",%c", 'X');
            }
            else{
               fprintf(fp, ",%d", Poutput[i]->value);
            }
         }
      }
      if (l != loop - 1)
         fprintf(fp, "\n");
   }

   // write into [output file name]
   fclose(fp);
   return 0;
}

/*-----------------------------------------------------------------------
input: output file name
output: nothing
called by: main 
description:
  generates a reduced list of faults required for the ATPG of the circuit.
  In this function we only consider the check point theorem as a method 
  to reduce the number of faults.
-----------------------------------------------------------------------*/
int rfl(char *cp) {
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
   if (sscanf(cp, "%s", &outfile) != 1) {printf("Incorrect input\n"); return 0;}
   printf("%s\n", outfile);
   fp = fopen(outfile, "w");

   for (it = pi_branch.begin(); it != pi_branch.end(); ++it) {
      fprintf(fp, "%d@0\n", (*it)->num);
      fprintf(fp, "%d@1\n", (*it)->num);
   }
   fclose(fp);
   return 0;
}

/*-----------------------------------------------------------------------
input: input file name, output file name
output: nothing
called by: main 
description:
  The DFS simulator simply has test patterns (one or many) as input 
  and reports all the detectable (so not just the RFL) faults using 
  these test patterns.
-----------------------------------------------------------------------*/

// step 1: logic simulation, values of all nodes
// step 2: fault list for each node, starting from the PIs
// step 3: fault propagation
//    case 1: all inputs have non-controlling values, the union of the fault lists of all inputs will be propagated
//    case 2: some inputs have controlling values, the intersection of the fault lists of controlling inputs and the complement lists of non-controlling inputs will be propagated
// step 4: print all detectable faults at the outputs    

int dfs(char *cp)
{
  int i, j;
  char sim_input_name[20];
  char fault_output_name[20];
  FILE *sim_input;
  int input[MAXLINE];
  int value[MAXLINE];
  sscanf(cp, "%s %s", sim_input_name, fault_output_name);
  sim_input = fopen(sim_input_name,"r");
  // Get the input values from file
  if (atpg_flag == 1)   //if from ATPG, Zeming
  {
    for (int i = 0; i < Npi; i++)
    {
      input[i] = input_test_vector_node[i];
      value[i] = input_test_vector_value[i];
      input_test_vector_value.pop_back();     //pop out the vector for next vector initiate
    }
    //printf("atpg_flag = 1");
  }
  else  //not from ATPG     Zeming
  {
    for (i = 0; i < Npi; i ++){
      if(fscanf(sim_input,"%d,%d", &input[i], &value[i]) == 2){
        printf("%d, %d\n",input[i],value[i]);
      }
    }
    //printf("atpg_flag = 0");
  }
  // Create mapping from indx to num
  // int* map_indx_num = new int[Nnodes]; // REMINDER to delete
  // First step: simulate the whole logic
  int temp_value;
  struct n_struc *current_node;
  current_node = starting_node;
  while (1){
    // map_indx_num[current_node->indx] = current_node->num; // REMINDER to delete
    switch (current_node->type){ // enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND}; 
      case IPT:{  // Get input value from txt file
        for (j = 0; j < Npi; j++){
          if (input[j] == current_node->num){
            break;
          }
        }
        current_node->value = value[j];
        break;
      }
      case BRCH:{
        current_node->value = current_node->unodes[0]->value;
        break;
      }
      case XOR:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value;
          }
          else{
            temp_value = temp_value ^ (current_node->unodes[i]->value);
          }
        }
        current_node->value = temp_value;
        break;
      }
      case OR:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value;
          }
          else{
            temp_value = temp_value || (current_node->unodes[i]->value);
          }
        }
        current_node->value = temp_value;
        break;
      }
      case NOR:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value;
          }
          else{
            temp_value = temp_value || (current_node->unodes[i]->value);
          }
        }
        current_node->value = !temp_value;
        break;
      }
      case NOT:{
        current_node ->value = !(current_node->unodes[0]->value);
        break;
      }
      case NAND:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value;
          }
          else{
            temp_value = temp_value && (current_node->unodes[i]->value);
          }
        }
        current_node->value = !temp_value;
        break;
      }
      case AND:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value;
          }
          else{
            temp_value = temp_value && (current_node->unodes[i]->value);
          }
        }
        current_node->value = temp_value;
        break;
      }
      default:;
    }
    if (current_node->next_node == NULL){
      break;
    }
    current_node = current_node->next_node;
  }
  // Second step: Create the empty list for each node. 
  int fault_ind, fault_ind2;
  int** fault_list_valid = new int*[Nnodes];
  for (fault_ind = 0; fault_ind < Nnodes; fault_ind++){
    fault_list_valid[fault_ind] = new int[2 * Nnodes]; // NOTE: Even index is sa1, while odd index is sa0
  }
  for (fault_ind = 0; fault_ind < Nnodes; fault_ind++){
    for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
      fault_list_valid[fault_ind][fault_ind2] = 0;
    }
  }
  // Thrid: Start the deductive fault analysis. 
  current_node = starting_node;
  while (1){
    // printf("Idx: %d Num: %d\n", current_node->indx, current_node->num);
    switch (current_node->type){ // enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND}; 
      case IPT:{
        fault_list_valid[current_node->indx][2 * current_node->indx + current_node->value] = 1;
        break;
      }
      case BRCH:{
        for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
          fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
        }
        fault_list_valid[current_node->indx][2 * current_node->indx + current_node->value] = 1;
        break;
      }
      case XOR:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
              // if (current_node->unodes[i]->value){
              fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
              // }
            }
          }
          else{
            for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
              // if (current_node->unodes[i]->value){
              fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] ^ fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
              // }
            }
          }
        }
        // Add the fault of the wire itself
        fault_list_valid[current_node->indx][2 * current_node->indx + current_node->value] = 1;
        break;
      }
      case OR:{
        if (current_node->value == 0){ // Add the union of the previous faults
          for (i = 0; i < current_node->fin; i++){
            if (i == 0){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                  if (current_node->unodes[i]->value==0){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
                }
            }
              else{ 
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                  if (current_node->unodes[i]->value==0){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] || fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
                  }
                }
              }
          }
      }
        else{
          for (i = 0; i < current_node->fin; i++){
            if (i == 0){
              if (current_node->unodes[i]->value == 1){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
              }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = 1 - fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
              }
            }
            else{
              if (current_node->unodes[i]->value == 1){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] && fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
                  }
              }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] && (1 - fault_list_valid[current_node->unodes[i]->indx][fault_ind2]);
                  }
              }
            }
          }
        }
        // Add the fault of the wire itself
        fault_list_valid[current_node->indx][2 * current_node->indx + current_node->value] = 1;
        break;
      }
      case NOR:{
        if (current_node->value == 1){ // Add the union of the previous faults
          for (i = 0; i < current_node->fin; i++){
            if (i == 0){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                  if (current_node->unodes[i]->value==0){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
                }
            }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                  if (current_node->unodes[i]->value==0){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] || fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
                  }
                }
              }
          }
      }
        else{
          for (i = 0; i < current_node->fin; i++){
            if (i == 0){
              if (current_node->unodes[i]->value == 1){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
              }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = 1 - fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
              }
            }
            else{
              if (current_node->unodes[i]->value == 1){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] && fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
                  }
              }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] && (1 - fault_list_valid[current_node->unodes[i]->indx][fault_ind2]);
                  }
              }
            }
          }
        }
        // Add the fault of the wire itself
        fault_list_valid[current_node->indx][2 * current_node->indx + current_node->value] = 1;
        break;
      }
      case NOT:{
        for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
          fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
        }
        fault_list_valid[current_node->indx][2 * current_node->indx + current_node->value] = 1;
        break;
      }
      case NAND:{
        if (current_node->value == 0){ // Add the union of the previous faults
          for (i = 0; i < current_node->fin; i++){
            if (i == 0){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                  if (current_node->unodes[i]->value){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
                }
            }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                  if (current_node->unodes[i]->value){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] || fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
                  }
                }
              }
          }
      }
        else{
          for (i = 0; i < current_node->fin; i++){
            if (i == 0){
              if (current_node->unodes[i]->value == 0){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
              }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = 1 - fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
              }
            }
            else{
              if (current_node->unodes[i]->value == 0){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] && fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
                  }
              }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] && (1 - fault_list_valid[current_node->unodes[i]->indx][fault_ind2]);
                  }
              }
            }
          }
        }
        // Add the fault of the wire itself
        fault_list_valid[current_node->indx][2 * current_node->indx + current_node->value] = 1;
        break;
      }
      case AND:{
        if (current_node->value == 1){ // Add the union of the previous faults
          for (i = 0; i < current_node->fin; i++){
            if (i == 0){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                  if (current_node->unodes[i]->value){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
                }
            }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                  if (current_node->unodes[i]->value){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] || fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
                  }
                }
              }
          }
      }
        else{
          for (i = 0; i < current_node->fin; i++){
            if (i == 0){
              if (current_node->unodes[i]->value == 0){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
              }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = 1 - fault_list_valid[current_node->unodes[0]->indx][fault_ind2];
                  }
              }
            }
            else{
              if (current_node->unodes[i]->value == 0){
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] && fault_list_valid[current_node->unodes[i]->indx][fault_ind2];
                  }
              }
              else{
                for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
                    fault_list_valid[current_node->indx][fault_ind2] = fault_list_valid[current_node->indx][fault_ind2] && (1 - fault_list_valid[current_node->unodes[i]->indx][fault_ind2]);
                  }
              }
            }
          }
        }
        // Add the fault of the wire itself
        fault_list_valid[current_node->indx][2 * current_node->indx + current_node->value] = 1;
        break;
      }
      default:;
    }
 //    for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
  //    if (fault_list_valid[current_node->indx][fault_ind2]){
  //      printf("Node_index: %d, Fault_index: %d s@%d\n", current_node->indx, (int)(fault_ind2 /2), 1 - (fault_ind2 % 2));
  //    }
  // }
    if (current_node->next_node == NULL){
      break;
    }
    current_node = current_node->next_node;
  }
  // Last step: print the output to file
  FILE* dfs_output;
  if (!atpg_flag)
  {
    dfs_output = fopen(fault_output_name,"w");
  }
  
  // Initialize the final fault list. 
  int* final_list = new int[2 * Nnodes];
  for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
    final_list[fault_ind2] = 0;
  }
  // Get the faults detected by each output port. 
  for(i = 0; i < Npo; i++){
    for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++){
      if (fault_list_valid[Poutput[i]->indx][fault_ind2]){
        final_list[fault_ind2] = 1;
      }
    }
  }
  if (!atpg_flag)
  {
    for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++)
    {
      if (final_list[fault_ind2]){
        printf("Total - Fault_index: %d s@%d\n", map_indx_num[(int)(fault_ind2 /2)], 1 - (fault_ind2 % 2)); //sa1, sa0
        fprintf(dfs_output, "%d@%d\n", map_indx_num[(int)(fault_ind2 /2)], 1 - (fault_ind2 % 2));
        //printf("can dectect- Fault_index: %d s@%d\n", map_indx_num[(int)(fault_ind2 /2)], 1 - (fault_ind2 % 2)); //sa1, sa0
      }
      else    //Zeming
      {
        ;
        //printf("cannot dectect- Fault_index: %d s@%d\n", map_indx_num[(int)(fault_ind2 /2)], 1 - (fault_ind2 % 2)); //sa1, sa0
      }
    }
    fclose(dfs_output);
  }
  else    //atpg flag == 1
  {
    for (fault_ind2 = 0; fault_ind2 < 2 * Nnodes; fault_ind2++)
    {
      if (final_list[fault_ind2]){
        //printf("Total - Fault_index: %d s@%d\n", map_indx_num[(int)(fault_ind2 /2)], 1 - (fault_ind2 % 2)); //sa1, sa0
        fault_dic.push_back(1);     //push_back << first in to left most   //Zeming
        fault_dic_node.push_back(map_indx_num[(int)(fault_ind2 /2)]);
        //printf("can dectect- Fault_index: %d s@%d\n", map_indx_num[(int)(fault_ind2 /2)], 1 - (fault_ind2 % 2)); //sa1, sa0
      }
      else    //Zeming
      {
        fault_dic.push_back(0); 
        fault_dic_node.push_back(map_indx_num[(int)(fault_ind2 /2)]);
        //printf("cannot dectect- Fault_index: %d s@%d\n", map_indx_num[(int)(fault_ind2 /2)], 1 - (fault_ind2 % 2)); //sa1, sa0
      }
    }
    
  }
  if (!atpg_flag)
  {
    printf("DFS=> OK\n");
  }
  
  return 0;
}

// int dfs(char *cp) {
//    char infile[MAXLINE], outfile[MAXLINE];
//    FILE *fp = NULL;
//    std::queue<NSTRUC*> qnodes;
//    std::vector<int> pi_idx;
//    std::queue<int> in_value;
//    std::set<std::pair<int, int> > detectable_faults;
//    NSTRUC *np;
//    int level_idx = 1;

//    // the levelization information is required for simulation
//    lev((char *)(""));

//    // parse the input and output filenames
//    if (sscanf(cp, "%s %s", &infile, &outfile) != 2) {printf("Incorrect input\n"); return 0;}
//    printf("%s\n", infile);
//    printf("%s\n", outfile);
//    fp = fopen(outfile, "w");

//    // read inputs
//    read_inputs(infile, &pi_idx, &in_value);

//    int loop = in_value.size() / pi_idx.size(); // loop is the number of input patterns

//    for (int l = 0; l < loop; l++) { // simulate for each input

//       reset_fault_list();

//       //********** step 1: simulate values of all nodes **********//
//       //********** step 2: create fault list for each node, starting from the PIs **********//
//       // assign primary input values
//       for (int i = 0; i < pi_idx.size(); i++) {
//          for (int j = 0; j < Npi; j++) {
//             if (Pinput[j]->num == pi_idx[i]) {
//                Pinput[j]->value = in_value.front();
//                Pinput[j]->fault_list->insert(std::make_pair(Pinput[j]->num, (Pinput[j]->value == 1 ? 0 : 1)));
//                printf("input %d has value %d\n", Pinput[j]->num, Pinput[j]->value);
//                in_value.pop();
//                break;
//             }
//          }
//       }

//       // simulate values of each wires level by level according to their levelization
//       // first push all down nodes of primary inputs which are at level 1
//       // since the values of inputs are already decided, their values can be simulated
//       for (int i = 0; i < Npi; i++) 
//          for (int j = 0; j < Pinput[i]->fout; j++) 
//             if (Pinput[i]->dnodes[j]->level == 1)
//                qnodes.push(Pinput[i]->dnodes[j]);

//       while(!qnodes.empty()) {
//          np = qnodes.front();
//          qnodes.pop();
//          sim(np);
//          //********** step 3: propagate fault lists **********//
//          // case 1: all inputs have non-controlling values, the union of the fault lists of all inputs will be propagated
//          // case 2: some inputs have controlling values, the intersection of the fault lists of controlling inputs and the complement lists of non-controlling inputs will be propagated
//          propagate_fault(np);
//          // for each down node, if its level is exactly 1 larger than the current node,
//          // it will be pushed into the queue. Otherwise, it will be pushed into the queue
//          // later, since not all of its inputs' values are decided yet.
//          for (int i = 0; i < np->fout; i++) {
//             if (np->dnodes[i]->level == (np->level + 1))
//                qnodes.push(np->dnodes[i]);
//          }
//       }
//       for (int i = 0; i < Npo; i++) {
//          for (std::set< std::pair<int, int> >::iterator it = Poutput[i]->fault_list->begin(); it != Poutput[i]->fault_list->end(); ++it) {
//             detectable_faults.insert(*it);
//          }   
//       }
//    }
//    //********** step 4: print all detectable faults at the outputs *//   
//    for (std::set< std::pair<int, int> >::iterator it = detectable_faults.begin(); it != detectable_faults.end(); ++it) {
//       fprintf(fp, "%d@%d\n", (*it).first, (*it).second);   
//    }
   
//    // release all fault lists
//    for (int i = 0; i < Nnodes; i++)
//       delete(Node[i].fault_list);

//    fclose(fp);
//    return 0;
// }


/*-----------------------------------------------------------------------
input: input test pattern file name, input fault list file name, output file name
output: nothing
called by: main
description:
  The PFS simulator gets a list of faults and a list of test patterns (one
  or many) as inputs and reports which one of the faults can be detected
  with these test patterns using the PFS (single pattern, parallel faults
  simulation) method.
-----------------------------------------------------------------------*/

// step 1: Determine bit-width of the processor
// step 2: fault list for each node, starting from the PIs
// step 3: fault propagation
//    case 1: all inputs have non-controlling values, the union of the fault lists of all inputs will be propagated
//    case 2: some inputs have controlling values, the intersection of the fault lists of controlling inputs and the complement lists of non-controlling inputs will be propagated
// step 4: print all detectable faults at the outputs

int pfs(char *cp) {
   long l;
   int bw, value, idx, fv;
   char temp;
   NSTRUC *np;
   std::vector<int> pi_idx;
   std::queue<int> in_value;
   std::queue<NSTRUC*> qnodes;
   FILE * fi, *ff, *fo;
   char infile1[MAXLINE], infile2[MAXLINE], outfile[MAXLINE];
   std::vector<std::pair<int, int> > loaded_faults;
   std::set<std::pair<int, int> > detected_faults;

   // the levelization information is required for simulation
   lev((char *)(""));

   bw = (8 * sizeof(l));
   printf("Word size of this machine is %hi bits\n", bw);

   // parse the input and output filenames
   if (sscanf(cp, "%s %s %s", &infile1, &infile2, &outfile) != 3) {
      printf("Incorrect input\n");
      return 0;
   }
   printf("input file name: %s\n", infile1);
   printf("fault file name: %s\n", infile2);
   printf("output file name: %s\n", outfile);

   if ((fi = fopen(infile1, "r")) == NULL) {
      printf("File %s does not exist!\n", infile1);
      return 0;
   }
   fo = fopen(outfile, "w");

   // read the input indices
   while (fscanf(fi, "%d%c", &idx, &temp) != EOF) {
      pi_idx.push_back(idx);
      if (temp == '\n')
         break;
      else if (temp == '\r') {
         fscanf(fi, "%c", &temp);
         break;
      }
   }

   // for each input pattern
   while (!feof(fi)) {
      // load one input pattern
      while (fscanf(fi, "%d%c", &value, &temp) != EOF) {
         in_value.push(value);
         if (temp == '\n')
            break;
         else if (temp == '\r') {
            fscanf(fi, "%c", &temp);
            break;
         }
      }
      if (feof(fi))
         break;
      // open the fault list file
      if ((ff = fopen(infile2, "r")) == NULL) {
         printf("File %s does not exist!\n", infile2);
         return 0;
      }

      // for every bw - 1 faults:
      while (!feof(ff)) {
         // load faults
         for (int i = 0; i < bw - 1; i++) {
            fscanf(ff, "%d@%d", &idx, &fv);
            if (feof(ff))
               break;
            printf("loaded fault #%d: %d@%d\n", i, idx, fv);
            loaded_faults.push_back(std::make_pair(idx, fv));
         }

         // initialize the inputs
         for (int i = 0; i < pi_idx.size(); i++) {
            for (int j = 0; j < Npi; j++) {
               if (Pinput[j]->num == pi_idx[i]) {
                  initialize_pi(Pinput[j], in_value.front());
                  check_faults(Pinput[j], loaded_faults);
                  in_value.pop();
                  for (int k = 0; k < Pinput[j]->fout; k++) {
                     if (Pinput[j]->dnodes[k]->level == 1)
                        qnodes.push(Pinput[j]->dnodes[k]);
                  }
               }
            }  
         }
         // simulate and propagate
         while (!qnodes.empty()) {
            np = qnodes.front();
            qnodes.pop();
            pfs_sim(np);
            check_faults(np, loaded_faults);
            // push all children nodes with level + 1 to the qnodes
            for (int i = 0; i < np->fout; i++) {
               if (np->dnodes[i]->level == (np->level + 1))
                  qnodes.push(np->dnodes[i]);
            }
         }
         
         // report detected faults
         report_faults(&detected_faults, &loaded_faults);
      }

      // close the fault list file
      fclose(ff);
   }

   // close the input file
   fclose(fi);

   // write output file and close it
   for (std::set<std::pair<int, int> >::iterator it = detected_faults.begin(); it != detected_faults.end(); ++it) {
      fprintf(fo, "%d@%d\n", (*it).first, (*it).second);
   }
   fclose(fo);
   return 0;
};

void report_faults(std::set<std::pair<int, int> > * detected_faults, std::vector<std::pair<int, int> > * loaded_faults) {
   int bw = loaded_faults->size();
   for (int i = 0; i < Npo; i++) {
      int MSB = (Poutput[i]->parallel_vector >> (bw-1));
      for (int j = 0; j < bw; j++) {
         int bj = (Poutput[i]->parallel_vector & (1 << j)) >> j;
         if (MSB != bj) {
            detected_faults->insert((*loaded_faults)[j]);
            // detected_faults->insert(std::make_pair((*loaded_faults)[j]->first, (*loaded_faults)[j]->second));
         }
      }
   }
}

void pfs_sim(NSTRUC * np) {
   int i;
   
   if (np->type == NOT) {
      np->parallel_vector = ~np->unodes[0]->parallel_vector;
   }
   else if (np->type == BRCH) {
      np->parallel_vector = np->unodes[0]->parallel_vector;
   }
   else if (np->type == XOR) {
      np->parallel_vector = 0;
      for (i = 0; i < np->fin; i++) {
         np->parallel_vector ^= np->unodes[i]->parallel_vector;
      }
   }
   else if (np->type == OR) {
      np->parallel_vector = 0;
      for (i = 0; i < np->fin; i++) {
         np->parallel_vector |= np->unodes[i]->parallel_vector;
      }
   }
   else if (np->type == NOR) {
      np->parallel_vector = 0;
      for (i = 0; i < np->fin; i++) {
         np->parallel_vector |= np->unodes[i]->parallel_vector;
      }
      np->parallel_vector = ~(np->parallel_vector);
   }
   else if (np->type == NAND) {
      np->parallel_vector = 1;
      for (i = 0; i < np->fin; i++) {
         np->parallel_vector &= np->unodes[i]->parallel_vector;
      }
      np->parallel_vector = ~(np->parallel_vector);
   }
   else if (np->type == AND) {
      np->parallel_vector = 1;
      for (i = 0; i < np->fin; i++) {
         np->parallel_vector &= np->unodes[i]->parallel_vector;
      }
   }
}

void check_faults(NSTRUC * np, std::vector<std::pair<int, int> > loaded_faults) {
   for (int i = 0; i < loaded_faults.size(); i++) {
      if (np->num == loaded_faults[i].first) {
         np->parallel_vector &= ~(1 << i);
         np->parallel_vector |= (loaded_faults[i].second << i);
         printf("input %d's vector has been modified: %lu\n", np->num, np->parallel_vector);
      }
   }
}

void initialize_pi(NSTRUC * np, int v) {
   switch(v) {
      case 1: np->parallel_vector = ~0; break;
      case 0: np->parallel_vector = 0; break;
   }
   printf("input %d's vector is: %lu\n", np->num, np->parallel_vector);
}


/*-----------------------------------------------------------------------
input: input test pattern file name, input fault list file name, output file name
output: nothing
called by: main
description:
  The PFS simulator gets a list of faults and a list of test patterns (one
  or many) as inputs and reports which one of the faults can be detected
  with these test patterns using the PFS (single pattern, parallel faults
  simulation) method.
-----------------------------------------------------------------------*/
int rtg(char *cp) {
   int ntot, ntfcr;
   char tp_report[MAXLINE], fc_report[MAXLINE];
   FILE *ftp = NULL, *ffc = NULL;   
   std::set<std::pair<int, int> > detected_faults;
   std::queue<int> test_pattern;
   float fc = 0.0;
   clock_t start;
   srand(time(NULL));
   int r = rand() % 1000;

   // the levelization information is required for simulation
   lev((char *)(""));

   // parse the input and output filenames
   if (sscanf(cp, "%d %d %s %s", &ntot, &ntfcr, &tp_report, &fc_report) != 4) {printf("Incorrect input\n"); return 0;}
   printf("test pattern report file: %s\n", tp_report);
   printf("fc report file: %s\n", fc_report);
   ftp = fopen(tp_report, "w");
   ffc = fopen(fc_report, "w");
   
   // write input indices into test pattern report file
   for (int i = 0; i < Npi; i++) {
      if (i == 0)
         fprintf(ftp, "%d", Pinput[i]->num);
      else
         fprintf(ftp, ",%d", Pinput[i]->num);
   }
   fprintf(ftp, "\n");

   start = clock();
   for (int i = 0; i < ntot; i++) {
      r = rand() % 1000;
      rtg_single(ftp, &test_pattern, start, r);
      dfs_single(&test_pattern, &detected_faults);
      //fc = calculate_fc(fc, &detected_faults);
      fc = 1.0 * detected_faults.size() / (2 * Nnodes);
      if ((i + 1) % ntfcr == 0)
         // report_fc(ffc, fc);
         fprintf(ffc, "%.2f\n", fc * 100);
   }
   return 0;
}

void dfs_single(std::queue<int> * test_pattern, std::set<std::pair<int, int> > * detected_faults) {
   std::queue<NSTRUC*> qnodes;  
   NSTRUC* np; 

   reset_fault_list();

   //********** step 1: simulate values of all nodes **********//
   //********** step 2: create fault list for each node, starting from the PIs **********//
   // assign values for primary inputs
   for (int i = 0; i < Npi; i++) {
      Pinput[i]->value = test_pattern->front();
      Pinput[i]->fault_list->insert(std::make_pair(Pinput[i]->num, (Pinput[i]->value == 1 ? 0 : 1)));
      test_pattern->pop();
   }

      // simulate values of each wires level by level according to their levelization
      // first push all down nodes of primary inputs which are at level 1
      // since the values of inputs are already decided, their values can be simulated
   for (int i = 0; i < Npi; i++) 
      for (int j = 0; j < Pinput[i]->fout; j++) 
         if (Pinput[i]->dnodes[j]->level == 1)
            qnodes.push(Pinput[i]->dnodes[j]);

   while(!qnodes.empty()) {
      np = qnodes.front();
      qnodes.pop();
      sim(np);
      //********** step 3: propagate fault lists **********//
      // case 1: all inputs have non-controlling values, the union of the fault lists of all inputs will be propagated
      // case 2: some inputs have controlling values, the intersection of the fault lists of controlling inputs and the complement lists of non-controlling inputs will be propagated
      propagate_fault(np);
      // for each down node, if its level is exactly 1 larger than the current node,
      // it will be pushed into the queue. Otherwise, it will be pushed into the queue
      // later, since not all of its inputs' values are decided yet.
      for (int i = 0; i < np->fout; i++) {
         if (np->dnodes[i]->level == (np->level + 1))
            qnodes.push(np->dnodes[i]);
      }
   }
   for (int i = 0; i < Npo; i++) {
      for (std::set< std::pair<int, int> >::iterator it = Poutput[i]->fault_list->begin(); it != Poutput[i]->fault_list->end(); ++it) {
         detected_faults->insert(*it);
      }   
   }
}

void rtg_single(FILE * ftp, std::queue<int> * test_pattern, clock_t start, int r) {
   int v;
   clock_t end;
   end = clock();
   srand((unsigned int)((end - start) / 1000) + r);
   printf("%d\n", int((end - start)/1000));
   for (int i = 0; i < Npi; i++) {
      v = rand() % 2;
      test_pattern->push(v);
      if (i == 0) 
         fprintf(ftp, "%d", v);
      else
         fprintf(ftp, ",%d", v);
   }
   fprintf(ftp, "\n");
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
   int idx; 
   char value;
   char temp;
   int i;
   printf("input file name: %s\n", cp);
   if ((fd = fopen(cp, "r")) == NULL) {
      printf("File %s does not exist!\n", cp);
      return;
   }
   while (fscanf(fd, "%d%c", &idx, &temp) != EOF) {
      pi_idx->push_back(idx);
      if (temp == '\n')
         break;
      else if (temp == '\r') {
         fscanf(fd, "%c", &temp);
         break;
      }
   }
   while (fscanf(fd, "%c%c", &value, &temp) != EOF) {
      switch (value) {
         case '1': in_value->push(1); break;
         case '0': in_value->push(0); break;
         case 'x':
         case 'X': in_value->push(-1); break;
      }
      if (temp == '\n')
         continue;
      else if (temp == '\r') {
         fscanf(fd, "%c", &temp);
         continue;
      }      
   }
   fclose(fd);
}

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
         if (np->unodes[i]->value == -1) {
            np->value = -1;
            break;
         }
         else if (np->unodes[i]->value == 0) {
            np->value = 1;
            break;
         }
      }
   }
   else if (np->type == BRCH) {
      np->value = 0;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == -1) {
            np->value = -1;
            break;
         }
         else if (np->unodes[i]->value == 1) {
            np->value = 1;
            break;
         }
      }
   }
   else if (np->type == XOR) {
      np->value = 0;
      for (i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == -1) {
            np->value = -1;
            break;
         }
         else if (np->unodes[i]->value == 1) {
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
         else if (np->unodes[i]->value == -1) {
            np->value = -1;
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
         else if (np->unodes[i]->value == -1) {
            np->value = -1;
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
         else if (np->unodes[i]->value == -1) {
            np->value = -1;
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
         else if (np->unodes[i]->value == -1) {
            np->value = -1;
         }
      }
   }
   //printf("node %d has type: %d value: %d level: %d\n", np->num, np->type, np->value, np->level);
}

/*-----------------------------------------------------------------------
input: node
output: nothing
called by: dfs 
description:
  Propagate fault list.
-----------------------------------------------------------------------*/

// step 3: fault propagation
//    case 1: all inputs have non-controlling values, the union of the fault lists of all inputs will be propagated
//    case 2: some inputs have controlling values, the intersection of the fault lists of controlling inputs and the complement lists of non-controlling inputs will be propagated


void propagate_fault(NSTRUC *np) {
   std::vector<NSTRUC *> cinputs;    // inputs with controlling values
   std::set<std::pair<int, int> > cinput_faults;    // faults shared by all fault lists of controlling inputs
   std::set<std::pair<int, int> > ncinput_faults;    // all faults in the fault lists of non-controlling inputs
   bool cinput_exist = false;

   if (np->type == OR or np->type == NOR) {
      for (int i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 1) {
            cinput_exist = true;
            cinputs.push_back(np->unodes[i]);
         }
         else {
            for (std::set< std::pair<int, int> >::iterator it = np->unodes[i]->fault_list->begin(); it != np->unodes[i]->fault_list->end(); ++it) {
               ncinput_faults.insert(*it);
            }
         }
      }
   }
   else if (np->type == NAND or np->type == AND) {
      for (int i = 0; i < np->fin; i++) {
         if (np->unodes[i]->value == 0) {
            cinput_exist = true;
            cinputs.push_back(np->unodes[i]);
         }
         else {
            for (std::set< std::pair<int, int> >::iterator it = np->unodes[i]->fault_list->begin(); it != np->unodes[i]->fault_list->end(); ++it) {
               ncinput_faults.insert(*it);
            }
         }
      }
   }
   else if (np->type == XOR) {
      std::set<std::pair<int, int> > odd_faults;
      std::set<std::pair<int, int> > even_faults;
      for (int i = 0; i < np->fin; i++) {
         for (std::set< std::pair<int, int> >::iterator it = np->unodes[i]->fault_list->begin(); it != np->unodes[i]->fault_list->end(); ++it) {
            if (even_faults.find(*it) != even_faults.end()) {
               odd_faults.insert(*it);
               even_faults.erase(*it);
            }
            else if (odd_faults.find(*it) != odd_faults.end()){
               even_faults.insert(*it);
               odd_faults.erase(*it);
            }
            else {
               odd_faults.insert(*it);
            }
         }
      }
      np->fault_list->insert(std::make_pair(np->num, (np->value == 1 ? 0 : 1)));
      for (std::set< std::pair<int, int> >::iterator it = odd_faults.begin(); it != odd_faults.end(); ++it) {
            np->fault_list->insert(*it);         
      }
      return;
   }

   np->fault_list->insert(std::make_pair(np->num, (np->value == 1 ? 0 : 1)));
   if (cinput_exist) {
      int n_cinputs = cinputs.size();
      for (std::set< std::pair<int, int> >::iterator it = cinputs[0]->fault_list->begin(); it != cinputs[0]->fault_list->end(); ++it)
         cinput_faults.insert(*it);
      for (int i = 1; i < n_cinputs; i++) {
         for (std::set< std::pair<int, int> >::iterator it = cinput_faults.begin(); it != cinput_faults.end(); ++it) {
            if (cinputs[i]->fault_list->find(*it) == cinputs[i]->fault_list->end())
               cinput_faults.erase(it);   
         }
      }
      for (std::set< std::pair<int, int> >::iterator it = cinput_faults.begin(); it != cinput_faults.end(); ++it) {
         if (ncinput_faults.find(*it) == ncinput_faults.end())
            np->fault_list->insert(*it);   
      }
   }
   else {
      for (int i = 0; i < np->fin; i++) {
         for (std::set< std::pair<int, int> >::iterator it = np->unodes[i]->fault_list->begin(); it != np->unodes[i]->fault_list->end(); ++it) {
            np->fault_list->insert(*it);
         }
      }
   }
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: dfs 
description:
  Reset all fault lists.
-----------------------------------------------------------------------*/
void reset_fault_list() {
   for (int i = 0; i < Nnodes; i++) {
      //delete(Node[i].fault_list);
      Node[i].fault_list = new(std::set<std::pair<int, int> >);
   }
}

/*-----------------------------------------------------------------------
input: nothing
output: nothing
called by: pfs
description:
  Compare number of faults with processor bit_width
-----------------------------------------------------------------------*/

int fault_bit_comparison(int fault_n, int bit_width){
    int number_words;
    if (fault_n > bit_width - 1) {
        number_words = ceil((1.0 * fault_n) / (bit_width - 1));
        //int remaining = fault_n - bit_width;
    }
    else {
        number_words = 1;
    }
    return number_words ;
}


void clear_mem()
{
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
/*========================= End of program ============================*/
