#ifndef ATPG_H
#define ATPG_H

#define ZERO (unsigned int)0
#define ONE (unsigned int)15
#define D (unsigned int)12
#define D_BAR (unsigned int)3
#define X (unsigned int)9

int dalg(char *cp);
int real_d_alg(int fault_node, std::vector<int> imply_stack_node, std::vector<int> imply_stack_dir, std::vector<unsigned int> imply_stack_val);
int imply_and_check(int fault_node, std::vector<int> imply_stack_node, std::vector<int> imply_stack_dir, std::vector<unsigned int> imply_stack_val);

int podem(char *cp);
int real_podem(int fault_node, int fault_type);
int obj(int fault_node, int fault_type);
int backtrace(void);
int imply(int fault_node,int fault_type, std::vector<int> imply_stack_dir,std::vector<unsigned int> imply_stack_val);
int atpg(char *cp);


#endif