#include "command.h"
#include "atpg.h"
#include <string.h>
#include <cstdlib>

int* d_frontier;
int* d_frontier_array; // for podem 
int d_frontier_flag;
std::vector<int*> d_frontier_stack;
int* j_frontier;  
std::vector<int*> j_frontier_stack;
int podem_start_iter;
std::vector<int> obj_output_node;
std::vector<unsigned int> obj_output_val;
int backtrace_output_node;
unsigned int backtrace_output_val;
int podem_fail_count;

//----------------zemingdone-------------------//
std::vector<int> atpg_to_alg_fault_node;      //atpg provide fault node to alg node#5
std::vector<int> atpg_to_alg_fault_type;      //atpg provide fault type to alg s@0/1
std::vector<int> alg_return_test_vector;      //alg return PI value to atpg
std::vector<int> alg_return_PI_node;  
int dalg_to_atpg_success = 0;                     //dlag success flag for atpg
int podem_to_atpg_success = 0;

int dalg(char *cp)  // This is the preparation function for D-alg
{ 
  pc(Cktname);
  lev((char *)(""));
  starting_node = Pinput[0];
  // Get the faults from user input
  int fault_node, fault_type;
  int seed_dalg, seed_dalg_temp;
  // sscanf(cp, "%d %d", &fault_node, &fault_type);
  if (!atpg_flag)
  {
    sscanf(cp, "%d %d", &fault_node, &fault_type);
  }
  else
  {
    fault_node = atpg_to_alg_fault_node.back();
    fault_type = atpg_to_alg_fault_type.back();
    atpg_to_alg_fault_node.pop_back();
    atpg_to_alg_fault_type.pop_back();
    /* debug
    printf("fault_node %d\n", fault_node);
    printf("fault_type %d\n", fault_type);
    */
  }
  if (!atpg_flag){  // If called by terminal directly
    printf("D-alg begins! Faulty node: %d s@%d\n", fault_node, fault_type);
  }
  // Create stacks for D, J, and imply&check
  d_frontier = new int[Nnodes];
  j_frontier = new int[Nnodes];
  std::vector<int> imply_stack_node;
  std::vector<int> imply_stack_dir; // 1 means forward and 0 means backward
  std::vector<unsigned int> imply_stack_val;  // 0 - zero; 7 - one; 6 - D; 1 - D_bar; 3 = X
  // Initialize the imply stack by adding the forward and backward implication of the faulty node
  imply_stack_node.push_back(fault_node);
  if (fault_type){
    imply_stack_val.push_back(ZERO);
  }
  else{
    imply_stack_val.push_back(ONE);
  }
  imply_stack_dir.push_back(0); // Backward

  imply_stack_node.push_back(fault_node);
  if (fault_type){
    imply_stack_val.push_back(D_BAR);
  }
  else{
    imply_stack_val.push_back(D);
  }
  imply_stack_dir.push_back(1); // Forward
  
  // Initialize the circuit by setting all node values to be X and empty the D and J-frontiers
  struct n_struc *current_node;
  current_node = starting_node;
  while(1){
    current_node->fault_value = X;  // TODO: Maybe this will not be used later

    while(!current_node->value_stack.empty()){
      current_node->value_stack.pop_back();
    }
    current_node->value_stack.push_back(X);
    current_node->previous_imply_dir.push_back(0);

    d_frontier[current_node->indx] = 0;
    j_frontier[current_node->indx] = 0;

    if (current_node->next_node == NULL){
      break;
    }
    current_node = current_node->next_node;
  }
  d_frontier_stack.push_back(d_frontier);
  j_frontier_stack.push_back(j_frontier);
  // Call the real d-algorithm
  int d_alg_flag;
  d_alg_flag = real_d_alg(fault_node, imply_stack_node, imply_stack_dir, imply_stack_val);
  // If success, print the input vector to file or give it to ATPG
  if (d_alg_flag){
    if (!atpg_flag){  // If called by terminal directly
      printf("D-alg success!\n");
      // Print the result to file
      char output_file_name[81];
      char fault_node_str[20], fault_type_str[20];
      for (int ind = 0; ind < 81; ind++){
        output_file_name[ind] = Cktname[ind];
      }
      strcat(output_file_name, "_DALG_");
      sprintf(fault_node_str, "%d", fault_node);
      strcat(output_file_name, fault_node_str);
      strcat(output_file_name, "@");
      sprintf(fault_type_str, "%d", fault_type);
      strcat(output_file_name, fault_type_str);
      strcat(output_file_name, ".txt");
      FILE * d_alg_report;
      d_alg_report = fopen(output_file_name, "w");
      for (int i = 0; i < Npi; i++){
        if (Pinput[i]->value_stack.back() == ONE){
          fprintf(d_alg_report, "%d,1\n", Pinput[i]->num);
        }
        else if (Pinput[i]->value_stack.back() == ZERO){
          fprintf(d_alg_report, "%d,0\n", Pinput[i]->num);
        }
        else if (Pinput[i]->value_stack.back() == D){
          fprintf(d_alg_report, "%d,1\n", Pinput[i]->num);
        }
        else if (Pinput[i]->value_stack.back() == D_BAR){
          fprintf(d_alg_report, "%d,0\n", Pinput[i]->num);
        }
        else{
          fprintf(d_alg_report, "%d,X\n", Pinput[i]->num);
        }
      }
      fclose(d_alg_report);
    }
    else{  // TODO_ZM: Add the atpg required information here. 
      //printf("atpg call D-alg success!\n");
      dalg_to_atpg_success = 1;       //flag to atapg is set
      for (int i = 0; i < Npi; i++)
      {
        if (Pinput[i]->value_stack.back() == ONE){
          //printf("%d,1\n", Pinput[i]->num);
          alg_return_test_vector.push_back(1);
        }
        else if (Pinput[i]->value_stack.back() == ZERO){
          //printf("%d,0\n", Pinput[i]->num);
          alg_return_test_vector.push_back(0);
        }
        else if (Pinput[i]->value_stack.back() == D){
          //printf("%d,1\n", Pinput[i]->num);
          alg_return_test_vector.push_back(1);
        }
        else if (Pinput[i]->value_stack.back() == D_BAR){
          //printf("%d,0\n", Pinput[i]->num);
          alg_return_test_vector.push_back(0);
        }
        else{
          //printf("%d,X\n", Pinput[i]->num);
          // seed_dalg = rand() % 6335;
          // srand(seed_dalg);
          seed_dalg_temp = rand() % 2;
          alg_return_test_vector.push_back(seed_dalg_temp);  //if x shows, provide default 0
        }
      }
    }
    return 1;
  }
  else{
    if (!atpg_flag){  // If called by terminal directly
      printf("D-alg failed!\n");
    }
    else{  // TODO_ZM: Add the atpg required information here. 
      dalg_to_atpg_success = 0;
      printf("atpg called, then D-alg failed!\n");
    }
    return 0;
  }
}

int real_d_alg(int fault_node, std::vector<int> imply_stack_node, std::vector<int> imply_stack_dir, std::vector<unsigned int> imply_stack_val){
  // recursive_depth_limit = recursive_depth_limit + 1;
  // if(recursive_depth_limit > 100000){
  //  printf("Give up due to large search space tried... \n");
  //  return 0; 
  // }
  int stack_ind;
  int imply_success;
  int d_success;
  struct n_struc *current_node;
  // Call imply and check
  imply_success = imply_and_check(fault_node, imply_stack_node, imply_stack_dir, imply_stack_val);
  if (!imply_success){  // If imply and check failure, then return failure (0)
    return 0;
  }
  // ++++++++++++++++++++++++++++This is for debug purpose, will be commented out later++++++++++++++++++++++++++++
  if (!atpg_flag){  // If called by terminal directly
    printf("---------------------------------\n");
    if (imply_success){ // Consistency
      current_node = starting_node;
      while(1){
        printf("%d, %u\n", current_node->num, current_node->value_stack.back());
        if (current_node->next_node == NULL){
          break;
        }
        current_node = current_node->next_node;
      }
    }
  }
  // ----------------------------Above is for debug purpose, will be commented out later----------------------------
  // Check if error is at PO
  int i, j, ipt_has_X;
  int error_at_PO;
  error_at_PO = 0;
  for (i = 0; i < Npo; i++){
    if ((Poutput[i]->value_stack.back() == D) || (Poutput[i]->value_stack.back() == D_BAR)){
      error_at_PO = 1;
      break;
    }
  }
  std::vector<int> imply_stack_node_inside;
  std::vector<int> imply_stack_dir_inside; // 1 means forward and 0 means backward
  std::vector<unsigned int> imply_stack_val_inside;  // 0 - zero; 7 - one; 6 - D; 1 - D_bar; 3 = X
  int d_empty, j_empty;
  if (!error_at_PO){  // If error not at PO
    // Check if D-frontier is empty
    d_empty = 1;
    for (i = 0; i < Nnodes; i++){
      if (d_frontier_stack.back()[i] == 1){
        d_empty = 0;
        break;
      }
    }
    if (d_empty){
      return 0;  // if D-frontier is empty, return failure
    }
    for (i = 0; i < Nnodes; i++){
      if (d_frontier_stack.back()[Nnodes - i - 1] == 1){  // If this gate is in D-frontier
        current_node = map_num_node[map_indx_num[Nnodes - i - 1]];
        if ((current_node->type == AND) || (current_node->type == NAND)){  // Controling value is 0
          if (!atpg_flag){  // If called by terminal directly
            printf("Selected D-frontier: %d\n", current_node->num);
          }
          // Assign cbar to every input of G with value x
          ipt_has_X = 0;
          for (j = 0; j < current_node->fin; j++){
            if (current_node->unodes[j]->value_stack.back() == X){
              ipt_has_X = 1;
              while(!imply_stack_node_inside.empty()){
                imply_stack_node_inside.pop_back();
                imply_stack_dir_inside.pop_back();
                imply_stack_val_inside.pop_back();
              }
              imply_stack_node_inside.push_back(current_node->unodes[j]->num);
              imply_stack_dir_inside.push_back(0);  // Backward
              imply_stack_val_inside.push_back(ONE);
              imply_stack_node_inside.push_back(current_node->unodes[j]->num);
              imply_stack_dir_inside.push_back(1);  // Forward
              imply_stack_val_inside.push_back(ONE);
            }
          }
          if (!ipt_has_X){
            return 0;
          }
          // Push the stack
          d_frontier_stack.push_back(d_frontier_stack.back());
          j_frontier_stack.push_back(j_frontier_stack.back());
          for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
            // printf("stack_ind: %d, total: %d, node num: %d \n", stack_ind, Nnodes, map_num_node[map_indx_num[stack_ind]]->value_stack.size());
            map_num_node[map_indx_num[stack_ind]]->value_stack.push_back(map_num_node[map_indx_num[stack_ind]]->value_stack.back());
            map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.push_back(map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.back());
          }
          // If D-alg() = SUCCESS then return SUCCESS
          d_success = real_d_alg(fault_node, imply_stack_node_inside, imply_stack_dir_inside, imply_stack_val_inside);
          if (d_success){
            return 1;
          }
          // Reverse the stack
          d_frontier_stack.pop_back();
          j_frontier_stack.pop_back();
          for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
            // printf("stack_ind: %d, total: %d, node num: %d \n", stack_ind, Nnodes, map_num_node[map_indx_num[stack_ind]]->value_stack.size());
            map_num_node[map_indx_num[stack_ind]]->value_stack.pop_back();
            map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.pop_back();
          }
        }
        else if ((current_node->type == OR) || (current_node->type == NOR)){  // Controling value is 1
          if (!atpg_flag){  // If called by terminal directly
            printf("Selected D-frontier: %d\n", current_node->num);
          }
          // Assign cbar to every input of G with value x
          ipt_has_X = 0;
          for (j = 0; j < current_node->fin; j++){
            if (current_node->unodes[j]->value_stack.back() == X){
              ipt_has_X = 1;
              while(!imply_stack_node_inside.empty()){
                imply_stack_node_inside.pop_back();
                imply_stack_dir_inside.pop_back();
                imply_stack_val_inside.pop_back();
              }
              imply_stack_node_inside.push_back(current_node->unodes[j]->num);
              imply_stack_dir_inside.push_back(0);  // Backward
              imply_stack_val_inside.push_back(ZERO);
              imply_stack_node_inside.push_back(current_node->unodes[j]->num);
              imply_stack_dir_inside.push_back(1);  // Forward
              imply_stack_val_inside.push_back(ZERO);
            }
          }
          if (!ipt_has_X){
            return 0;
          }
          // Push the stack
          d_frontier_stack.push_back(d_frontier_stack.back());
          j_frontier_stack.push_back(j_frontier_stack.back());
          for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
            // printf("stack_ind: %d, total: %d, node num: %d \n", stack_ind, Nnodes, map_num_node[map_indx_num[stack_ind]]->value_stack.size());
            map_num_node[map_indx_num[stack_ind]]->value_stack.push_back(map_num_node[map_indx_num[stack_ind]]->value_stack.back());
            map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.push_back(map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.back());
          }
          // If D-alg() = SUCCESS then return SUCCESS
          d_success = real_d_alg(fault_node, imply_stack_node_inside, imply_stack_dir_inside, imply_stack_val_inside);
          if (d_success){
            return 1;
          }
          // Reverse the stack
          d_frontier_stack.pop_back();
          j_frontier_stack.pop_back();
          for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
            // printf("stack_ind: %d, total: %d, node num: %d \n", stack_ind, Nnodes, map_num_node[map_indx_num[stack_ind]]->value_stack.size());
            map_num_node[map_indx_num[stack_ind]]->value_stack.pop_back();
            map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.pop_back();
          }
        }
        else if (current_node->type == XOR){  // No controling value
          if (!atpg_flag){  // If called by terminal directly
            printf("Selected D-frontier: %d\n", current_node->num);
          }
          // Assign cbar to every input of G with value x
          ipt_has_X = 0;
          for (j = 0; j < current_node->fin; j++){
            if (current_node->unodes[j]->value_stack.back() == X){
              ipt_has_X = 1;
              while(!imply_stack_node_inside.empty()){
                imply_stack_node_inside.pop_back();
                imply_stack_dir_inside.pop_back();
                imply_stack_val_inside.pop_back();
              }
              imply_stack_node_inside.push_back(current_node->unodes[j]->num);
              imply_stack_dir_inside.push_back(0);  // Backward
              imply_stack_val_inside.push_back(ZERO);
              imply_stack_node_inside.push_back(current_node->unodes[j]->num);
              imply_stack_dir_inside.push_back(1);  // Forward
              imply_stack_val_inside.push_back(ZERO);
            }
          }
          if (!ipt_has_X){
            return 0;
          }
          // Push the stack
          d_frontier_stack.push_back(d_frontier_stack.back());
          j_frontier_stack.push_back(j_frontier_stack.back());
          for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
            // printf("stack_ind: %d, total: %d, node num: %d \n", stack_ind, Nnodes, map_num_node[map_indx_num[stack_ind]]->value_stack.size());
            map_num_node[map_indx_num[stack_ind]]->value_stack.push_back(map_num_node[map_indx_num[stack_ind]]->value_stack.back());
            map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.push_back(map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.back());
          }
          // If D-alg() = SUCCESS then return SUCCESS
          d_success = real_d_alg(fault_node, imply_stack_node_inside, imply_stack_dir_inside, imply_stack_val_inside);
          if (d_success){
            return 1;
          }
          // Reverse the stack
          d_frontier_stack.pop_back();
          j_frontier_stack.pop_back();
          for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
            // printf("stack_ind: %d, total: %d, node num: %d \n", stack_ind, Nnodes, map_num_node[map_indx_num[stack_ind]]->value_stack.size());
            map_num_node[map_indx_num[stack_ind]]->value_stack.pop_back();
            map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.pop_back();
          }
          if (!atpg_flag){  // If called by terminal directly
            printf("Selected D-frontier: %d\n", current_node->num);
          }
          // Assign cbar to every input of G with value x
          ipt_has_X = 0;
          for (j = 0; j < current_node->fin; j++){
            if (current_node->unodes[j]->value_stack.back() == X){
              ipt_has_X = 1;
              while(!imply_stack_node_inside.empty()){
                imply_stack_node_inside.pop_back();
                imply_stack_dir_inside.pop_back();
                imply_stack_val_inside.pop_back();
              }
              imply_stack_node_inside.push_back(current_node->unodes[j]->num);
              imply_stack_dir_inside.push_back(0);  // Backward
              imply_stack_val_inside.push_back(ONE);
              imply_stack_node_inside.push_back(current_node->unodes[j]->num);
              imply_stack_dir_inside.push_back(1);  // Forward
              imply_stack_val_inside.push_back(ONE);
            }
          }
          if (!ipt_has_X){
            return 0;
          }
          // Push the stack
          d_frontier_stack.push_back(d_frontier_stack.back());
          j_frontier_stack.push_back(j_frontier_stack.back());
          for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
            // printf("stack_ind: %d, total: %d, node num: %d \n", stack_ind, Nnodes, map_num_node[map_indx_num[stack_ind]]->value_stack.size());
            map_num_node[map_indx_num[stack_ind]]->value_stack.push_back(map_num_node[map_indx_num[stack_ind]]->value_stack.back());
            map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.push_back(map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.back());
          }
          // If D-alg() = SUCCESS then return SUCCESS
          d_success = real_d_alg(fault_node, imply_stack_node_inside, imply_stack_dir_inside, imply_stack_val_inside);
          if (d_success){
            return 1;
          }
          // Reverse the stack
          d_frontier_stack.pop_back();
          j_frontier_stack.pop_back();
          for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
            // printf("stack_ind: %d, total: %d, node num: %d \n", stack_ind, Nnodes, map_num_node[map_indx_num[stack_ind]]->value_stack.size());
            map_num_node[map_indx_num[stack_ind]]->value_stack.pop_back();
            map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.pop_back();
          }
        }
      }
    }
    return 0; // return failure
  }

  /* error propagated to a PO */
  j_empty = 1;
  // Check if J-frontier is empty
  for (i = 0; i < Nnodes; i++){
  if (j_frontier_stack.back()[i] == 1){
    j_empty = 0;
    break;
  }
  }
  if (j_empty){  // If no J-frontier gate, then return success
    return 1;
  }
  // Select a gate from the J-frontier
  for (i = 0; i < Nnodes; i++){
    if (j_frontier_stack.back()[i] == 1){  // Found a gate which belong to the J-frontier
      current_node = map_num_node[map_indx_num[i]];
      if (!atpg_flag){
        printf("Selected J-frontier: %d\n", current_node->num);
      }
      // while(1){  // This loop will be exited when all inputs of current_node are specified
        // ipt_has_X = 0;  // Flag variable indicating whether there is X value in the input of the gate
      if ((current_node->type == AND) || (current_node->type == NAND)){  // Controling value is 0
        for(j = 0; j < current_node->fin; j++){
          if (current_node->unodes[j]->value_stack.back() == X){
            imply_stack_node_inside.push_back(current_node->unodes[j]->num);
            imply_stack_dir_inside.push_back(0);  // Backward
            imply_stack_val_inside.push_back(ZERO);
            ipt_has_X = 1;
            // Push the stack
            d_frontier_stack.push_back(d_frontier_stack.back());
            j_frontier_stack.push_back(j_frontier_stack.back());
            for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
              map_num_node[map_indx_num[stack_ind]]->value_stack.push_back(map_num_node[map_indx_num[stack_ind]]->value_stack.back());
              map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.push_back(map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.back());
            }
            // Remove this gate from J-frontier
            j_frontier_stack.back()[current_node->indx] = 0;
            // If D-alg() = SUCCESS then return SUCCESS
            d_success = real_d_alg(fault_node, imply_stack_node_inside, imply_stack_dir_inside, imply_stack_val_inside);
            if (d_success){
              return 1;
            }
            // printf("Backward imply: %d\n", current_node->unodes[j]->num);
            // Reverse the stack
            d_frontier_stack.pop_back();
            j_frontier_stack.pop_back();
            for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
              map_num_node[map_indx_num[stack_ind]]->value_stack.pop_back();
              map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.pop_back();
            }
            /* Reverse decision */
            imply_stack_node_inside.push_back(current_node->unodes[j]->num);
            imply_stack_dir_inside.push_back(0);  // Backward
            imply_stack_val_inside.push_back(ONE);
          }
        }
      }
      else if ((current_node->type == OR) || (current_node->type == NOR)){  // Controling value is 1
        for(j = 0; j < current_node->fin; j++){
          if (current_node->unodes[j]->value_stack.back() == X){
            imply_stack_node_inside.push_back(current_node->unodes[j]->num);
            imply_stack_dir_inside.push_back(0);  // Backward
            imply_stack_val_inside.push_back(ONE);
            ipt_has_X = 1;
            // Push the stack
            d_frontier_stack.push_back(d_frontier_stack.back());
            j_frontier_stack.push_back(j_frontier_stack.back());
            for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
              map_num_node[map_indx_num[stack_ind]]->value_stack.push_back(map_num_node[map_indx_num[stack_ind]]->value_stack.back());
              map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.push_back(map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.back());
            }
            // Remove this gate from J-frontier
            j_frontier_stack.back()[current_node->indx] = 0;
            // If D-alg() = SUCCESS then return SUCCESS
            d_success = real_d_alg(fault_node, imply_stack_node_inside, imply_stack_dir_inside, imply_stack_val_inside);
            if (d_success){
              return 1;
            }
            // Reverse the stack
            d_frontier_stack.pop_back();
            j_frontier_stack.pop_back();
            for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
              map_num_node[map_indx_num[stack_ind]]->value_stack.pop_back();
              map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.pop_back();
            }
            /* Reverse decision */
            imply_stack_node_inside.push_back(current_node->unodes[j]->num);
            imply_stack_dir_inside.push_back(0);  // Backward
            imply_stack_val_inside.push_back(ZERO);
          }
        }
      }
      else if (current_node->type == XOR){  // No controling value
        ipt_has_X = 0;
        for(j = 0; j < current_node->fin; j++){
          if (current_node->unodes[j]->value_stack.back() == X){
          	if ((current_node->value_stack.back() == ONE) || (current_node->value_stack.back() == D)){
          		imply_stack_node_inside.push_back(current_node->unodes[j]->num);
	            imply_stack_dir_inside.push_back(0);  // Backward
	            imply_stack_val_inside.push_back(ONE);
	            imply_stack_node_inside.push_back(current_node->unodes[1-j]->num);
	            imply_stack_dir_inside.push_back(0);  // Backward
	            imply_stack_val_inside.push_back(ZERO);
	            ipt_has_X = 1;
          	}
          	else{
          		imply_stack_node_inside.push_back(current_node->unodes[j]->num);
	            imply_stack_dir_inside.push_back(0);  // Backward
	            imply_stack_val_inside.push_back(ONE);
	            imply_stack_node_inside.push_back(current_node->unodes[1-j]->num);
	            imply_stack_dir_inside.push_back(0);  // Backward
	            imply_stack_val_inside.push_back(ONE);
	            ipt_has_X = 1;
          	}
            // imply_stack_node_inside.push_back(current_node->unodes[j]->num);
            // imply_stack_dir_inside.push_back(0);  // Backward
            // imply_stack_val_inside.push_back(ONE);
            // ipt_has_X = 1;
            // Push the stack
            d_frontier_stack.push_back(d_frontier_stack.back());
            j_frontier_stack.push_back(j_frontier_stack.back());
            for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
              map_num_node[map_indx_num[stack_ind]]->value_stack.push_back(map_num_node[map_indx_num[stack_ind]]->value_stack.back());
              map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.push_back(map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.back());
            }
            // Remove this gate from J-frontier
            if (current_node->unodes[1 - j]->value_stack.back() == X){
              j_frontier_stack.back()[current_node->indx] = 0;
            }
            // If D-alg() = SUCCESS then return SUCCESS
            d_success = real_d_alg(fault_node, imply_stack_node_inside, imply_stack_dir_inside, imply_stack_val_inside);
            if (d_success){
              return 1;
            }
            // Reverse the stack
            d_frontier_stack.pop_back();
            j_frontier_stack.pop_back();
            for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
              map_num_node[map_indx_num[stack_ind]]->value_stack.pop_back();
              map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.pop_back();
            }
            // XOR the other choice 
            if ((current_node->value_stack.back() == ONE) || (current_node->value_stack.back() == D)){
          		imply_stack_node_inside.push_back(current_node->unodes[j]->num);
	            imply_stack_dir_inside.push_back(0);  // Backward
	            imply_stack_val_inside.push_back(ZERO);
	            imply_stack_node_inside.push_back(current_node->unodes[1-j]->num);
	            imply_stack_dir_inside.push_back(0);  // Backward
	            imply_stack_val_inside.push_back(ONE);
	            ipt_has_X = 1;
          	}
          	else{
          		imply_stack_node_inside.push_back(current_node->unodes[j]->num);
	            imply_stack_dir_inside.push_back(0);  // Backward
	            imply_stack_val_inside.push_back(ZERO);
	            imply_stack_node_inside.push_back(current_node->unodes[1-j]->num);
	            imply_stack_dir_inside.push_back(0);  // Backward
	            imply_stack_val_inside.push_back(ZERO);
	            ipt_has_X = 1;
          	}
            // imply_stack_node_inside.push_back(current_node->unodes[j]->num);
            // imply_stack_dir_inside.push_back(0);  // Backward
            // imply_stack_val_inside.push_back(ZERO);
            // ipt_has_X = 1;
            // Push the stack
            d_frontier_stack.push_back(d_frontier_stack.back());
            j_frontier_stack.push_back(j_frontier_stack.back());
            for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
              map_num_node[map_indx_num[stack_ind]]->value_stack.push_back(map_num_node[map_indx_num[stack_ind]]->value_stack.back());
              map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.push_back(map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.back());
            }
            // Remove this gate from J-frontier
            if (current_node->unodes[1 - j]->value_stack.back() == X){
              j_frontier_stack.back()[current_node->indx] = 0;
            }
            // If D-alg() = SUCCESS then return SUCCESS
            d_success = real_d_alg(fault_node, imply_stack_node_inside, imply_stack_dir_inside, imply_stack_val_inside);
            if (d_success){
              return 1;
            }
            // Reverse the stack
            d_frontier_stack.pop_back();
            j_frontier_stack.pop_back();
            for (stack_ind = 0; stack_ind < Nnodes; stack_ind++){
              map_num_node[map_indx_num[stack_ind]]->value_stack.pop_back();
              map_num_node[map_indx_num[stack_ind]]->previous_imply_dir.pop_back();
            }
          }
        }
      }
      return 0;  // return failure
    }
  }
}


int imply_and_check(int fault_node, std::vector<int> imply_stack_node, std::vector<int> imply_stack_dir, std::vector<unsigned int> imply_stack_val){
  int current;
  unsigned int current_val;
  int current_dir;
  struct n_struc *current_node;
  struct n_struc *opt_node;
  struct n_struc *ipt_node;
  struct n_struc *fault_node_struct;
  int i, j;
  int count;
  int temp_to_be_modified;  // Used in backward prop special case
  unsigned int temp;
  int has_D, has_D_BAR, has_one_or_zero;  // Used in some forward implication cases
  int last_imply_node;
  fault_node_struct = map_num_node[fault_node];
  while (imply_stack_node.size()){  // Check if the assignment stack is empty. If empty, it means imply is finished. 
    // Obtain the current implication from the stack
    current_dir = imply_stack_dir.back();
    current = imply_stack_node.back();
    current_val = imply_stack_val.back();
    imply_stack_dir.pop_back();
    imply_stack_node.pop_back();
    imply_stack_val.pop_back();
    current_node = map_num_node[current];
    if (!atpg_flag){  // If called by terminal directly
      if (current_dir){
        printf("Imply: %d forward %u\n", current, current_val);
      }
      else{
        printf("Imply: %d backward %u\n", current, current_val);
      }
    }
    // Assign value to the node
    // If the node value is already assigned (not X), and the value to be assigned is different from the node value
    // Then inconsistency occurs
    if (current == fault_node){  // If this node is the faulty node
      if (current_node->value_stack.back() != X){
        if ((current_node->value_stack.back() == D) && (current_val == ZERO)){
          // printf("Inconsistency at %d\n", current);
            return 0;  // Return failure
        }
        if ((current_node->value_stack.back() == D_BAR) && (current_val == ONE)){
          // printf("Inconsistency at %d\n", current);
            return 0;  // Return failure
        }
      }
    }
    else{  // If this node is not the faulty node
      if ((current_val != current_node->value_stack.back()) && (current_node->value_stack.back() != X)){
        if (current != fault_node){ // Also, if the node is the faulty node, we accept both D/D_bar and 1/0 values (No consistency)
          // printf("Inconsistency at %d\n", current);
          return 0;  // Return failure
        }
      }
    }
    // If the value to be assigned is the same as the already the same as the node value, then no need to imly again
  //   if ((current_node->value_stack.back() == current_val) && (current != fault_node)){
  //  // However, we accept backward implication repeatedly. TODO: Maybe need to be changed. 
    // if (!current_dir){
    //  goto C;
    // }
  //      continue;
  //      C:;
  //   }
    // Start the node implication now
    current_node->value_stack.back() = current_val;
    // *************** Forward propagation ******************
    if (current_dir){
      // For the faulty node, if its input indicate a 1/0 value, we still make it D/D_bar
      if (current == fault_node){
        if (current_node->value_stack.back() == ONE){
          current_node->value_stack.back() = D;
        }
        else if (current_node->value_stack.back() == ZERO){
          current_node->value_stack.back() = D_BAR;
        }
      }
      // Let's now look at what this node can implicate forward. 
      for (i = 0; i < current_node->fout; i++){
        // Obtain the fan-out node of this node
        opt_node = current_node->dnodes[i];
        switch (opt_node->type){ // enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND};             
          case BRCH:{
            // if (opt_node->num != last_imply_node){
            if (opt_node == fault_node_struct){
              if ((current_node->value_stack.back() == ONE) && (opt_node->value_stack.back() == D)){
                ;
              }
              else if ((current_node->value_stack.back() == ZERO) && (opt_node->value_stack.back() == D_BAR)){
                ;
              }
              else{
                imply_stack_node.push_back(opt_node->num);
                imply_stack_dir.push_back(1);  // Forward
                imply_stack_val.push_back(current_node->value_stack.back());
              }
            }
            else{
              if (opt_node->value_stack.back() != current_node->value_stack.back()){
                imply_stack_node.push_back(opt_node->num);
                imply_stack_dir.push_back(1);  // Forward
                imply_stack_val.push_back(current_node->value_stack.back());
              }
            }
            break;
          }
          case NOT:{
            imply_stack_node.push_back(opt_node->num);
            imply_stack_dir.push_back(1);  // Forward
            imply_stack_val.push_back((unsigned int)15 - current_node->value_stack.back());
            break;
          }
          case XOR:{
            for (j = 0; j < opt_node->fin; j++){
              // Find the other input of the XOR gate
            if (opt_node->unodes[j]->num != current){
              if (opt_node->unodes[j]->value_stack.back() != X){  // If the other node is not X
                // Simulate the output of the gate
                temp = opt_node->unodes[0]->value_stack.back() ^ opt_node->unodes[1]->value_stack.back();
                // if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                if (opt_node->value_stack.back() != temp){
                  imply_stack_node.push_back(opt_node->num);
                  imply_stack_dir.push_back(1);
                  imply_stack_val.push_back(temp);
                }
                // XOR output is no longer X, so remove from D-frontier
                d_frontier_stack.back()[opt_node->indx] = 0;
                // }
                // else{
                //   if ((current_val == D) || (current_val == D_BAR) && (opt_node->value_stack.back() == X)){
                //     d_frontier_stack.back()[opt_node->indx] = 1;
                //   }
                // }
                // Since both inputs of XOR are specified, remove from J-frontier
                j_frontier_stack.back()[opt_node->indx] = 0;
              }
              else if (opt_node->value_stack.back() != X){  // If the XOR output value is specified but the other input is not. 
                if (opt_node != fault_node_struct){  // If the XOR output is NOT faulty node
                  temp = current_node->value_stack.back() ^ opt_node->value_stack.back();
                }
                else{  // If the XOR output is the faulty node
                  if (opt_node->value_stack.back() == D){
                    temp = current_node->value_stack.back() ^ ONE;
                  }
                  else{
                    temp = current_node->value_stack.back() ^ ZERO;
                  }
                }
                // if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                if (opt_node->unodes[j]->value_stack.back() != temp){  // TODO: Should I propogate temp backward always or not. 
                    imply_stack_node.push_back(opt_node->unodes[j]->num);
                    imply_stack_dir.push_back(0);
                    imply_stack_val.push_back(temp);
                }
                // Definitely not J-frontier any longer
                j_frontier_stack.back()[opt_node->indx] = 0;
                // }
              }
              else{  // If both the other node and the XOR output are X
                if ((current_node->value_stack.back() == D) || (current_node->value_stack.back() == D_BAR)){
                  d_frontier_stack.back()[opt_node->indx] = 1;
                }
                // Definitely not J-frontier
                j_frontier_stack.back()[opt_node->indx] = 0;
              }
            }
            }
            break;
          }
          case AND:{
            if (current_node->value_stack.back() == ZERO){  // If the current node is assigned a controlling value
              if (opt_node->value_stack.back() != current_node->value_stack.back()){
                imply_stack_node.push_back(opt_node->num);
                imply_stack_dir.push_back(1);
                imply_stack_val.push_back(ZERO);
              }
              // Definitely not J-frontier
              j_frontier_stack.back()[opt_node->indx] = 0;
              // Definitely not D-frontier
              d_frontier_stack.back()[opt_node->indx] = 0;
            }
            else if (current_node->value_stack.back() == ONE){  // If the current node is assigned a non-controlling value
              if (opt_node->value_stack.back() == ZERO){  // In this condition, only when one of the input is ZERO or there are D and D_BAR. 
                // Definitely not D-frontier
                d_frontier_stack.back()[opt_node->indx] = 0;
                has_D = 0;////////////
                has_D_BAR = 0;////////////
                has_one_or_zero = 0;////////////
                count = 0;  // Will represent the number of D, D_BAR and non-controlling values in the gate input
                temp_to_be_modified = -1;  // Will mark the index of the gate input which is X
                for (j = 0; j < opt_node->fin; j++){
                  if ((opt_node->unodes[j]->value_stack.back() == ONE) || (opt_node->unodes[j]->value_stack.back() == D) || (opt_node->unodes[j]->value_stack.back() == D_BAR)){
                    count = count + 1;
                    ////////////
                    if (opt_node->unodes[j]->value_stack.back() == D){
                      has_D = 1;
                    }
                    if (opt_node->unodes[j]->value_stack.back() == D_BAR){
                      has_D_BAR = 1;
                    }
                    ////////////
                  }
                  else if (opt_node->unodes[j]->value_stack.back() == X){
                    temp_to_be_modified = j;
                  }
                  else{
                    has_one_or_zero = 1;////////////
                    break;

                  }
                }
                ////////////
                if ((count == opt_node->fin - 1) && (temp_to_be_modified >= 0)){  // If there is only one X in the input and all the other inputs are non controlling
                  if ((has_one_or_zero) || (has_D && has_D_BAR)){
                    ;
                  }
                  else{
                    imply_stack_node.push_back(opt_node->unodes[temp_to_be_modified]->num);
                    imply_stack_dir.push_back(0);
                    imply_stack_val.push_back(ZERO);
                  }
                  j_frontier_stack.back()[opt_node->indx] = 0;
                }
                else{  // If more than one X or no X in the gate input
                  if (temp_to_be_modified < 0){ // If no X in the gate input
                    j_frontier_stack.back()[opt_node->indx] = 0;
                  }
                  else{  //If more than one X in the gate input
                    if (has_one_or_zero){  // If there is a controlling value in the gate input
                      j_frontier_stack.back()[opt_node->indx] = 0;
                    }
                    else if ((has_D && has_D_BAR)){
                      j_frontier_stack.back()[opt_node->indx] = 0;
                    }
                    else{
                       j_frontier_stack.back()[opt_node->indx] = 1;
                    }
                  }
                }
                ////////////
              }
              else if (opt_node->value_stack.back() == X){  // If gate output is not specified
                has_D = 0;////////////
                has_D_BAR = 0;////////////
                has_one_or_zero = 0;////////////
                for (j = 0; j < opt_node->fin; j++){
                    if (j == 0){
                      temp = opt_node->unodes[j]->value_stack.back();
                    }
                    else{
                      temp = temp & opt_node->unodes[j]->value_stack.back();
                    }
                    if (opt_node->unodes[j]->value_stack.back() == D){
                      has_D = 1;
                    }
                    else if (opt_node->unodes[j]->value_stack.back() == D_BAR){
                      has_D_BAR = 1;
                    }
                    else if (opt_node->unodes[j]->value_stack.back() == ZERO){
                      has_one_or_zero = 1;
                    }
                }
                if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                  imply_stack_node.push_back(opt_node->num);
                  imply_stack_dir.push_back(1);
                  imply_stack_val.push_back(temp);
                  j_frontier_stack.back()[opt_node->indx] = 0;
                  d_frontier_stack.back()[opt_node->indx] = 0;
                }
                // If not , temp = X, then no need to imply
                else{
                  j_frontier_stack.back()[opt_node->indx] = 0;
                  if ((has_D) && (has_D_BAR)){  // If both D and D_Bar exist
                    d_frontier_stack.back()[opt_node->indx] = 0;
                  }
                  else{
                    if(has_D || has_D_BAR){  // If only one of the D or D_bar exists
                      d_frontier_stack.back()[opt_node->indx] = 1;
                    }
                    else{  // No fault value in the gate input
                      d_frontier_stack.back()[opt_node->indx] = 0;
                    }
                  }
                }
              }
            }
            else if (current_node->value_stack.back() != X){  // If the current node is assigned a D or D_BAR
              // Run the gate simulation
              for (j = 0; j < opt_node->fin; j++){
                if (j == 0){
                  temp = opt_node->unodes[j]->value_stack.back();
                }
                else{
                  temp = temp & opt_node->unodes[j]->value_stack.back();
                }
              }
              if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                if (opt_node == fault_node_struct){
                  if (((temp == ONE) || (temp == D) ) && (opt_node->value_stack.back() == D)){
                    ;
                  }
                  else if (((temp == ZERO) || (temp == D_BAR) ) && (opt_node->value_stack.back() == D_BAR)){
                    ;
                  }
                  else{
                    imply_stack_node.push_back(opt_node->num);
                    imply_stack_dir.push_back(1);
                    imply_stack_val.push_back(temp);
                  }
                }
                else{
                  if (opt_node->value_stack.back() != temp){
                    imply_stack_node.push_back(opt_node->num);
                    imply_stack_dir.push_back(1);
                    imply_stack_val.push_back(temp);
                  }
                }
                d_frontier_stack.back()[opt_node->indx] = 0;
                j_frontier_stack.back()[opt_node->indx] = 0;
              }
              else{ // If temp = X
                d_frontier_stack.back()[opt_node->indx] = 1;
              }
            }
            break;
          }
          
          case NAND:{
            if (current_node->value_stack.back() == ZERO){  // If the current node is assigned a controlling value
              if (opt_node->value_stack.back() != current_node->value_stack.back()){
                imply_stack_node.push_back(opt_node->num);
                imply_stack_dir.push_back(1);
                imply_stack_val.push_back(ONE);
              }
              // Definitely not J-frontier
              j_frontier_stack.back()[opt_node->indx] = 0;
              // Definitely not D-frontier
              d_frontier_stack.back()[opt_node->indx] = 0;
            }
            else if (current_node->value_stack.back() == ONE){  // If the current node is assigned a non-controlling value
              if (opt_node->value_stack.back() == ONE){  // In this condition, only when one of the input is ZERO or there are D and D_BAR. 
                // Definitely not D-frontier
                d_frontier_stack.back()[opt_node->indx] = 0;
                has_D = 0;////////////
                has_D_BAR = 0;////////////
                has_one_or_zero = 0;////////////
                count = 0;  // Will represent the number of D, D_BAR and non-controlling values in the gate input
                temp_to_be_modified = -1;  // Will mark the index of the gate input which is X
                for (j = 0; j < opt_node->fin; j++){
                  if ((opt_node->unodes[j]->value_stack.back() == ONE) || (opt_node->unodes[j]->value_stack.back() == D) || (opt_node->unodes[j]->value_stack.back() == D_BAR)){
                    count = count + 1;
                    ////////////
                    if (opt_node->unodes[j]->value_stack.back() == D){
                      has_D = 1;
                    }
                    if (opt_node->unodes[j]->value_stack.back() == D_BAR){
                      has_D_BAR = 1;
                    }
                    ////////////
                  }
                  else if (opt_node->unodes[j]->value_stack.back() == X){
                    temp_to_be_modified = j;
                  }
                  else{
                    has_one_or_zero = 1;////////////
                    break;

                  }
                }
                ////////////
                if ((count == opt_node->fin - 1) && (temp_to_be_modified >= 0)){  // If there is only one X in the input and all the other inputs are non controlling
                  if ((has_one_or_zero) || (has_D && has_D_BAR)){
                    ;
                  }
                  else{
                    imply_stack_node.push_back(opt_node->unodes[temp_to_be_modified]->num);
                    imply_stack_dir.push_back(0);
                    imply_stack_val.push_back(ZERO);
                  }
                  j_frontier_stack.back()[opt_node->indx] = 0;
                }
                else{  // If more than one X or no X in the gate input
                  if (temp_to_be_modified < 0){ // If no X in the gate input
                    j_frontier_stack.back()[opt_node->indx] = 0;
                  }
                  else{  //If more than one X in the gate input
                    if (has_one_or_zero){  // If there is a controlling value in the gate input
                      j_frontier_stack.back()[opt_node->indx] = 0;
                    }
                    else if ((has_D && has_D_BAR)){
                      j_frontier_stack.back()[opt_node->indx] = 0;
                    }
                    else{
                       j_frontier_stack.back()[opt_node->indx] = 1;
                    }
                  }
                }
                ////////////
              }
              else if (opt_node->value_stack.back() == X){  // If gate output is not specified
                has_D = 0;////////////
                has_D_BAR = 0;////////////
                has_one_or_zero = 0;////////////
                for (j = 0; j < opt_node->fin; j++){
                    if (j == 0){
                      temp = opt_node->unodes[j]->value_stack.back();
                    }
                    else{
                      temp = temp & opt_node->unodes[j]->value_stack.back();
                    }
                    if (opt_node->unodes[j]->value_stack.back() == D){
                      has_D = 1;
                    }
                    else if (opt_node->unodes[j]->value_stack.back() == D_BAR){
                      has_D_BAR = 1;
                    }
                    else if (opt_node->unodes[j]->value_stack.back() == ZERO){
                      has_one_or_zero = 1;
                    }
                }
                if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                  imply_stack_node.push_back(opt_node->num);
                  imply_stack_dir.push_back(1);
                  imply_stack_val.push_back((unsigned int)15 - temp);
                  j_frontier_stack.back()[opt_node->indx] = 0;
                  d_frontier_stack.back()[opt_node->indx] = 0;
                }
                // If not , temp = X, then no need to imply
                else{
                  j_frontier_stack.back()[opt_node->indx] = 0;
                  if ((has_D) && (has_D_BAR)){  // If both D and D_Bar exist
                    d_frontier_stack.back()[opt_node->indx] = 0;
                  }
                  else{
                    if(has_D || has_D_BAR){  // If only one of the D or D_bar exists
                      d_frontier_stack.back()[opt_node->indx] = 1;
                    }
                    else{  // No fault value in the gate input
                      d_frontier_stack.back()[opt_node->indx] = 0;
                    }
                  }
                }
              }
            }
            else if (current_node->value_stack.back() != X){  // If the current node is assigned a D or D_BAR
              // Run the gate simulation
              for (j = 0; j < opt_node->fin; j++){
                if (j == 0){
                  temp = opt_node->unodes[j]->value_stack.back();
                }
                else{
                  temp = temp & opt_node->unodes[j]->value_stack.back();
                }
              }
              temp = (unsigned int)15 - temp;
              if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                if (opt_node == fault_node_struct){
                  if (((temp == ONE) || (temp == D) ) && (opt_node->value_stack.back() == D)){
                    ;
                  }
                  else if (((temp == ZERO) || (temp == D_BAR) ) && (opt_node->value_stack.back() == D_BAR)){
                    ;
                  }
                  else{
                    imply_stack_node.push_back(opt_node->num);
                    imply_stack_dir.push_back(1);
                    imply_stack_val.push_back(temp);
                  }
                }
                else{
                  if (opt_node->value_stack.back() != temp){
                    imply_stack_node.push_back(opt_node->num);
                    imply_stack_dir.push_back(1);
                    imply_stack_val.push_back(temp);
                  }
                }
                d_frontier_stack.back()[opt_node->indx] = 0;
                j_frontier_stack.back()[opt_node->indx] = 0;
              }
              else{ // If temp = X
                d_frontier_stack.back()[opt_node->indx] = 1;
              }
            }
            break;
          }
          
          case OR:{
            if (current_node->value_stack.back() == ONE){  // If the current node is assigned a controlling value
              if (opt_node->value_stack.back() != current_node->value_stack.back()){
                imply_stack_node.push_back(opt_node->num);
                imply_stack_dir.push_back(1);
                imply_stack_val.push_back(ONE);
              }
              // Definitely not J-frontier
              j_frontier_stack.back()[opt_node->indx] = 0;
              // Definitely not D-frontier
              d_frontier_stack.back()[opt_node->indx] = 0;
            }
            else if (current_node->value_stack.back() == ZERO){  // If the current node is assigned a non-controlling value
              if (opt_node->value_stack.back() == ONE){  // In this condition, only when one of the input is ZERO or there are D and D_BAR. 
                // Definitely not D-frontier
                d_frontier_stack.back()[opt_node->indx] = 0;
                has_D = 0;////////////
                has_D_BAR = 0;////////////
                has_one_or_zero = 0;////////////
                count = 0;  // Will represent the number of D, D_BAR and non-controlling values in the gate input
                temp_to_be_modified = -1;  // Will mark the index of the gate input which is X
                for (j = 0; j < opt_node->fin; j++){
                  if ((opt_node->unodes[j]->value_stack.back() == ZERO) || (opt_node->unodes[j]->value_stack.back() == D) || (opt_node->unodes[j]->value_stack.back() == D_BAR)){
                    count = count + 1;
                    ////////////
                    if (opt_node->unodes[j]->value_stack.back() == D){
                      has_D = 1;
                    }
                    if (opt_node->unodes[j]->value_stack.back() == D_BAR){
                      has_D_BAR = 1;
                    }
                    ////////////
                  }
                  else if (opt_node->unodes[j]->value_stack.back() == X){
                    temp_to_be_modified = j;
                  }
                  else{
                    has_one_or_zero = 1;////////////
                    break;

                  }
                }
                ////////////
                if ((count == opt_node->fin - 1) && (temp_to_be_modified >= 0)){  // If there is only one X in the input and all the other inputs are non controlling
                  if ((has_one_or_zero) || (has_D && has_D_BAR)){
                    ;
                  }
                  else{
                    imply_stack_node.push_back(opt_node->unodes[temp_to_be_modified]->num);
                    imply_stack_dir.push_back(0);
                    imply_stack_val.push_back(ONE);
                  }
                  j_frontier_stack.back()[opt_node->indx] = 0;
                }
                else{  // If more than one X or no X in the gate input
                  if (temp_to_be_modified < 0){ // If no X in the gate input
                    j_frontier_stack.back()[opt_node->indx] = 0;
                  }
                  else{  //If more than one X in the gate input
                    if (has_one_or_zero){  // If there is a controlling value in the gate input
                      j_frontier_stack.back()[opt_node->indx] = 0;
                    }
                    else if ((has_D && has_D_BAR)){
                      j_frontier_stack.back()[opt_node->indx] = 0;
                    }
                    else{
                       j_frontier_stack.back()[opt_node->indx] = 1;
                    }
                  }
                }
                ////////////
              }
              else if (opt_node->value_stack.back() == X){  // If gate output is not specified
                has_D = 0;////////////
                has_D_BAR = 0;////////////
                has_one_or_zero = 0;////////////
                for (j = 0; j < opt_node->fin; j++){
                    if (j == 0){
                      temp = opt_node->unodes[j]->value_stack.back();
                    }
                    else{
                      temp = temp | opt_node->unodes[j]->value_stack.back();
                    }
                    if (opt_node->unodes[j]->value_stack.back() == D){
                      has_D = 1;
                    }
                    else if (opt_node->unodes[j]->value_stack.back() == D_BAR){
                      has_D_BAR = 1;
                    }
                    else if (opt_node->unodes[j]->value_stack.back() == ONE){
                      has_one_or_zero = 1;
                    }
                }
                if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                  imply_stack_node.push_back(opt_node->num);
                  imply_stack_dir.push_back(1);
                  imply_stack_val.push_back(temp);
                  j_frontier_stack.back()[opt_node->indx] = 0;
                  d_frontier_stack.back()[opt_node->indx] = 0;
                }
                // If not , temp = X, then no need to imply
                else{
                  j_frontier_stack.back()[opt_node->indx] = 0;
                  if ((has_D) && (has_D_BAR)){  // If both D and D_Bar exist
                    d_frontier_stack.back()[opt_node->indx] = 0;
                  }
                  else{
                    if(has_D || has_D_BAR){  // If only one of the D or D_bar exists
                      d_frontier_stack.back()[opt_node->indx] = 1;
                    }
                    else{  // No fault value in the gate input
                      d_frontier_stack.back()[opt_node->indx] = 0;
                    }
                  }
                }
              }
            }
            else if (current_node->value_stack.back() != X){  // If the current node is assigned a D or D_BAR
              // Run the gate simulation
              for (j = 0; j < opt_node->fin; j++){
                if (j == 0){
                  temp = opt_node->unodes[j]->value_stack.back();
                }
                else{
                  temp = temp | opt_node->unodes[j]->value_stack.back();
                }
              }
              if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                if (opt_node == fault_node_struct){
                  if (((temp == ONE) || (temp == D) ) && (opt_node->value_stack.back() == D)){
                    ;
                  }
                  else if (((temp == ZERO) || (temp == D_BAR) ) && (opt_node->value_stack.back() == D_BAR)){
                    ;
                  }
                  else{
                    imply_stack_node.push_back(opt_node->num);
                    imply_stack_dir.push_back(1);
                    imply_stack_val.push_back(temp);
                  }
                }
                else{
                  if (opt_node->value_stack.back() != temp){
                    imply_stack_node.push_back(opt_node->num);
                    imply_stack_dir.push_back(1);
                    imply_stack_val.push_back(temp);
                  }
                }
                d_frontier_stack.back()[opt_node->indx] = 0;
                j_frontier_stack.back()[opt_node->indx] = 0;
              }
              else{ // If temp = X
                d_frontier_stack.back()[opt_node->indx] = 1;
              }
            }
            break;
          }
            
          case NOR:{
            if (current_node->value_stack.back() == ONE){  // If the current node is assigned a controlling value
              if (opt_node->value_stack.back() != current_node->value_stack.back()){
                imply_stack_node.push_back(opt_node->num);
                imply_stack_dir.push_back(1);
                imply_stack_val.push_back(ZERO);
              }
              // Definitely not J-frontier
              j_frontier_stack.back()[opt_node->indx] = 0;
              // Definitely not D-frontier
              d_frontier_stack.back()[opt_node->indx] = 0;
            }
            else if (current_node->value_stack.back() == ZERO){  // If the current node is assigned a non-controlling value
              if (opt_node->value_stack.back() == ZERO){  // In this condition, only when one of the input is ZERO or there are D and D_BAR. 
                // Definitely not D-frontier
                d_frontier_stack.back()[opt_node->indx] = 0;
                has_D = 0;////////////
                has_D_BAR = 0;////////////
                has_one_or_zero = 0;////////////
                count = 0;  // Will represent the number of D, D_BAR and non-controlling values in the gate input
                temp_to_be_modified = -1;  // Will mark the index of the gate input which is X
                for (j = 0; j < opt_node->fin; j++){
                  if ((opt_node->unodes[j]->value_stack.back() == ZERO) || (opt_node->unodes[j]->value_stack.back() == D) || (opt_node->unodes[j]->value_stack.back() == D_BAR)){
                    count = count + 1;
                    ////////////
                    if (opt_node->unodes[j]->value_stack.back() == D){
                      has_D = 1;
                    }
                    if (opt_node->unodes[j]->value_stack.back() == D_BAR){
                      has_D_BAR = 1;
                    }
                    ////////////
                  }
                  else if (opt_node->unodes[j]->value_stack.back() == X){
                    temp_to_be_modified = j;
                  }
                  else{
                    has_one_or_zero = 1;////////////
                    break;

                  }
                }
                ////////////
                if ((count == opt_node->fin - 1) && (temp_to_be_modified >= 0)){  // If there is only one X in the input and all the other inputs are non controlling
                  if ((has_one_or_zero) || (has_D && has_D_BAR)){
                    ;
                  }
                  else{
                    imply_stack_node.push_back(opt_node->unodes[temp_to_be_modified]->num);
                    imply_stack_dir.push_back(0);
                    imply_stack_val.push_back(ONE);
                  }
                  j_frontier_stack.back()[opt_node->indx] = 0;
                }
                else{  // If more than one X or no X in the gate input
                  if (temp_to_be_modified < 0){ // If no X in the gate input
                    j_frontier_stack.back()[opt_node->indx] = 0;
                  }
                  else{  //If more than one X in the gate input
                    if (has_one_or_zero){  // If there is a controlling value in the gate input
                      j_frontier_stack.back()[opt_node->indx] = 0;
                    }
                    else if ((has_D && has_D_BAR)){
                      j_frontier_stack.back()[opt_node->indx] = 0;
                    }
                    else{
                       j_frontier_stack.back()[opt_node->indx] = 1;
                    }
                  }
                }
                ////////////
              }
              else if (opt_node->value_stack.back() == X){  // If gate output is not specified
                has_D = 0;////////////
                has_D_BAR = 0;////////////
                has_one_or_zero = 0;////////////
                for (j = 0; j < opt_node->fin; j++){
                    if (j == 0){
                      temp = opt_node->unodes[j]->value_stack.back();
                    }
                    else{
                      temp = temp | opt_node->unodes[j]->value_stack.back();  ////*/*/*/*
                    }
                    if (opt_node->unodes[j]->value_stack.back() == D){
                      has_D = 1;
                    }
                    else if (opt_node->unodes[j]->value_stack.back() == D_BAR){
                      has_D_BAR = 1;
                    }
                    else if (opt_node->unodes[j]->value_stack.back() == ONE){
                      has_one_or_zero = 1;
                    }
                }
                if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                  imply_stack_node.push_back(opt_node->num);
                  imply_stack_dir.push_back(1);
                  imply_stack_val.push_back((unsigned int)15 - temp);
                  j_frontier_stack.back()[opt_node->indx] = 0;
                  d_frontier_stack.back()[opt_node->indx] = 0;
                }
                // If not , temp = X, then no need to imply
                else{
                  j_frontier_stack.back()[opt_node->indx] = 0;
                  if ((has_D) && (has_D_BAR)){  // If both D and D_Bar exist
                    d_frontier_stack.back()[opt_node->indx] = 0;
                  }
                  else{
                    if(has_D || has_D_BAR){  // If only one of the D or D_bar exists
                      d_frontier_stack.back()[opt_node->indx] = 1;
                    }
                    else{  // No fault value in the gate input
                      d_frontier_stack.back()[opt_node->indx] = 0;
                    }
                  }
                }
              }
            }
            else if (current_node->value_stack.back() != X){  // If the current node is assigned a D or D_BAR
              // Run the gate simulation
              for (j = 0; j < opt_node->fin; j++){
                if (j == 0){
                  temp = opt_node->unodes[j]->value_stack.back();
                }
                else{
                  temp = temp | opt_node->unodes[j]->value_stack.back(); ////*/*/*/
                }
              }
              temp = (unsigned int)15 - temp;
              if ((temp == ONE) || (temp == ZERO) || (temp == D) || (temp == D_BAR)){
                if (opt_node == fault_node_struct){
                  if (((temp == ONE) || (temp == D) ) && (opt_node->value_stack.back() == D)){
                    ;
                  }
                  else if (((temp == ZERO) || (temp == D_BAR) ) && (opt_node->value_stack.back() == D_BAR)){
                    ;
                  }
                  else{
                    imply_stack_node.push_back(opt_node->num);
                    imply_stack_dir.push_back(1);
                    imply_stack_val.push_back(temp);
                  }
                }
                else{
                  if (opt_node->value_stack.back() != temp){
                    imply_stack_node.push_back(opt_node->num);
                    imply_stack_dir.push_back(1);
                    imply_stack_val.push_back(temp);
                  }
                }
                d_frontier_stack.back()[opt_node->indx] = 0;
                j_frontier_stack.back()[opt_node->indx] = 0;
              }
              else{ // If temp = X
                d_frontier_stack.back()[opt_node->indx] = 1;
              }
            }
            break;
          }  
        }
      }
    }
    // *******************Backward implication***************************
    else{
    // For the faulty node, if its input indicate a D/D_bar value, we still make it 1/0
      if (current == fault_node){
        if (current_node->value_stack.back() == D){
          current_node->value_stack.back() = ONE;
        }
        else if (current_node->value_stack.back() == D_BAR){
          current_node->value_stack.back() = ZERO;
        }
      }
      switch (current_node->type){ // enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND}; 
          case IPT:{  
            break;
          }
          case BRCH:{
            ipt_node = current_node->unodes[0];
            // TODO: See if the direction has some influence on the result
            // Stem backward
            imply_stack_node.push_back(ipt_node->num);
            imply_stack_dir.push_back(0);
            imply_stack_val.push_back(current_node->value_stack.back());
            // Stem forward
            imply_stack_node.push_back(ipt_node->num);
            imply_stack_dir.push_back(1);
            imply_stack_val.push_back(current_node->value_stack.back());
            break;
          }
          case XOR:{
           // For backwarding, definitely the node is not D-frontier
           d_frontier_stack.back()[current_node->indx] = 0;
           if (current_node->unodes[0]->value_stack.back() != X){  // If one of the gate input is NOT X
             ipt_node = current_node->unodes[1];
              // backward
             if (current_node == fault_node_struct){
                 if (fault_node_struct->value_stack.back() == D){
                   temp = current_node->unodes[0]->value_stack.back() ^ ONE;
                 }
                 else if (fault_node_struct->value_stack.back() == D_BAR){
                   temp = current_node->unodes[0]->value_stack.back() ^ ZERO;
                 }
                 else{
                   temp = current_node->unodes[0]->value_stack.back() ^ current_node->value_stack.back();
                 }
             }
             else{
               temp = current_node->unodes[0]->value_stack.back() ^ current_node->value_stack.back();
             }
             if (temp != ipt_node->value_stack.back()){  // If the implied value is not the same as the node value
                imply_stack_node.push_back(ipt_node->num);
                imply_stack_dir.push_back(0);
                imply_stack_val.push_back(temp);
             }
             j_frontier_stack.back()[current_node->indx] = 0;
           }
           else if (current_node->unodes[1]->value_stack.back() != X){
             ipt_node = current_node->unodes[0];
              // backward
             if (current_node == fault_node_struct){
                 if (fault_node_struct->value_stack.back() == D){
                   temp = current_node->unodes[1]->value_stack.back() ^ ONE;
                 }
                 else if (fault_node_struct->value_stack.back() == D_BAR){
                   temp = current_node->unodes[1]->value_stack.back() ^ ZERO;
                 }
                 else{
                   temp = current_node->unodes[1]->value_stack.back() ^ current_node->value_stack.back();
                 }
             }
             else{
               temp = current_node->unodes[1]->value_stack.back() ^ current_node->value_stack.back();
             }
             if (temp != ipt_node->value_stack.back()){  // If the implied value is not the same as the node value
                imply_stack_node.push_back(ipt_node->num);
                imply_stack_dir.push_back(0);
                imply_stack_val.push_back(temp);
             }
             j_frontier_stack.back()[current_node->indx] = 0;
           }
           else{
            if (current_node == fault_node_struct){
              j_frontier_stack.back()[current_node->indx] = 0;
            }
            else{
              j_frontier_stack.back()[current_node->indx] = 1;
            }
           }
            break;
          }

          case OR:{
            d_frontier_stack.back()[current_node->indx] = 0;
            if (current_node->value_stack.back() == ZERO){ //when output is 0, all input should be 0
              for (i = 0; i < current_node->fin; i++){
                if (current_node->unodes[i]->value_stack.back() != current_node->value_stack.back()){
                  imply_stack_node.push_back(current_node->unodes[i]->num);
                  imply_stack_dir.push_back(0);
                  imply_stack_val.push_back(ZERO);
                }
                j_frontier_stack.back()[current_node->indx] = 0;
              }
            }
            else { //when output is 1
              count = 0;
              temp_to_be_modified = -1;
              has_D = 0;
              has_D_BAR = 0;
              has_one_or_zero = 0;
              for (i = 0; i < current_node->fin; i++){
                if ((current_node->unodes[i]->value_stack.back() == ZERO) || (current_node->unodes[i]->value_stack.back() == D) || (current_node->unodes[i]->value_stack.back() == D_BAR)){ //when output is 0, all input should be 0
                  count = count + 1;
                  if (current_node->unodes[i]->value_stack.back() == D){
                    has_D = 1;
                  }
                  else if(current_node->unodes[i]->value_stack.back() == D_BAR){
                    has_D_BAR = 1;
                  }
                }
                else if (current_node->unodes[i]->value_stack.back() == X){
                  temp_to_be_modified = i;
                }
                else{  // If one of the input is ONE, break, no need to update
                  j_frontier_stack.back()[current_node->indx] = 0;
                  has_one_or_zero = 1;
                  break;
                }
              }

              if ((count == current_node->fin - 1) && (temp_to_be_modified >= 0)){  // If there is only one X in the input and all the other inputs are non controlling
              if ((has_one_or_zero) || (has_D && has_D_BAR)){
                  ;
              }
                else{
                  imply_stack_node.push_back(current_node->unodes[temp_to_be_modified]->num);
                  imply_stack_dir.push_back(0);
                  imply_stack_val.push_back(ONE);
                }
                j_frontier_stack.back()[current_node->indx] = 0;
              }
            else{  // If more than one X or no X in the gate input
                if (temp_to_be_modified < 0){ // If no X in the gate input
                  j_frontier_stack.back()[current_node->indx] = 0;
                }
                else{  //If more than one X in the gate input
                  if (has_one_or_zero){  // If there is a controlling value in the gate input
                    j_frontier_stack.back()[current_node->indx] = 0;
                  }
                  else if ((has_D && has_D_BAR)){
                    j_frontier_stack.back()[current_node->indx] = 0;
                  }
                  else{
                     j_frontier_stack.back()[current_node->indx] = 1;
                  }
                }
            }
            }
            break;
          }

          case NOR:{
            d_frontier_stack.back()[current_node->indx] = 0;
            if (current_node->value_stack.back() == ONE){ //when output is 1, all input should be 0
              for (i = 0; i < current_node->fin; i++){
                if (current_node->unodes[i]->value_stack.back() != current_node->value_stack.back()){
                  imply_stack_node.push_back(current_node->unodes[i]->num);
                  imply_stack_dir.push_back(0);
                  imply_stack_val.push_back(ZERO);
                }
                j_frontier_stack.back()[current_node->indx] = 0;
              }
            }
            else { //when output is 0
              count = 0;
              temp_to_be_modified = -1;
              has_D = 0;
              has_D_BAR = 0;
              has_one_or_zero = 0;
              for (i = 0; i < current_node->fin; i++){
                if ((current_node->unodes[i]->value_stack.back() == ZERO) || (current_node->unodes[i]->value_stack.back() == D) || (current_node->unodes[i]->value_stack.back() == D_BAR)){ //when output is 0, all input should be 0
                  count = count + 1;
                  if (current_node->unodes[i]->value_stack.back() == D){
                    has_D = 1;
                  }
                  else if(current_node->unodes[i]->value_stack.back() == D_BAR){
                    has_D_BAR = 1;
                  }
                }
                else if (current_node->unodes[i]->value_stack.back() == X){
                  temp_to_be_modified = i;
                }
                else{  // If one of the input is ONE, break, no need to update
                  j_frontier_stack.back()[current_node->indx] = 0;
                  has_one_or_zero = 1;
                  break;
                }
              }

              if ((count == current_node->fin - 1) && (temp_to_be_modified >= 0)){  // If there is only one X in the input and all the other inputs are non controlling
              if ((has_one_or_zero) || (has_D && has_D_BAR)){
                  ;
              }
                else{
                  imply_stack_node.push_back(current_node->unodes[temp_to_be_modified]->num);
                  imply_stack_dir.push_back(0);
                  imply_stack_val.push_back(ONE);
                }
                j_frontier_stack.back()[current_node->indx] = 0;
              }
            else{  // If more than one X or no X in the gate input
                if (temp_to_be_modified < 0){ // If no X in the gate input
                  j_frontier_stack.back()[current_node->indx] = 0;
                }
                else{  //If more than one X in the gate input
                  if (has_one_or_zero){  // If there is a controlling value in the gate input
                    j_frontier_stack.back()[current_node->indx] = 0;
                  }
                  else if ((has_D && has_D_BAR)){
                    j_frontier_stack.back()[current_node->indx] = 0;
                  }
                  else{
                     j_frontier_stack.back()[current_node->indx] = 1;
                  }
                }
            }
            }
            break;
          }

          case AND:{
            d_frontier_stack.back()[current_node->indx] = 0;
            if (current_node->value_stack.back() == ONE){ //when output is 1, all input should be 1
              for (i = 0; i < current_node->fin; i++){
                if (current_node->unodes[i]->value_stack.back() != current_node->value_stack.back()){
                  imply_stack_node.push_back(current_node->unodes[i]->num);
                  imply_stack_dir.push_back(0);
                  imply_stack_val.push_back(ONE);
                }
                j_frontier_stack.back()[current_node->indx] = 0;
              }
            }
            else { //when output is 0
              count = 0;
              temp_to_be_modified = -1;
              has_D = 0;
              has_D_BAR = 0;
              has_one_or_zero = 0;
              for (i = 0; i < current_node->fin; i++){
                if ((current_node->unodes[i]->value_stack.back() == ONE) || (current_node->unodes[i]->value_stack.back() == D) || (current_node->unodes[i]->value_stack.back() == D_BAR)){ //when output is 0, all input should be 0
                  count = count + 1;
                  if (current_node->unodes[i]->value_stack.back() == D){
                    has_D = 1;
                  }
                  else if(current_node->unodes[i]->value_stack.back() == D_BAR){
                    has_D_BAR = 1;
                  }
                }
                else if (current_node->unodes[i]->value_stack.back() == X){
                  temp_to_be_modified = i;
                }
                else{  // If one of the input is ZERO, break, no need to update
                  j_frontier_stack.back()[current_node->indx] = 0;
                  has_one_or_zero = 1;
                  break;
                }
              }

              if ((count == current_node->fin - 1) && (temp_to_be_modified >= 0)){  // If there is only one X in the input and all the other inputs are non controlling
              if ((has_one_or_zero) || (has_D && has_D_BAR)){
                  ;
              }
                else{
                  imply_stack_node.push_back(current_node->unodes[temp_to_be_modified]->num);
                  imply_stack_dir.push_back(0);
                  imply_stack_val.push_back(ZERO);
                }
                j_frontier_stack.back()[current_node->indx] = 0;
              }
            else{  // If more than one X or no X in the gate input
                if (temp_to_be_modified < 0){ // If no X in the gate input
                  j_frontier_stack.back()[current_node->indx] = 0;
                }
                else{  //If more than one X in the gate input
                  if (has_one_or_zero){  // If there is a controlling value in the gate input
                    j_frontier_stack.back()[current_node->indx] = 0;
                  }
                  else if ((has_D && has_D_BAR)){
                    j_frontier_stack.back()[current_node->indx] = 0;
                  }
                  else{
                     j_frontier_stack.back()[current_node->indx] = 1;
                  }
                }
            }
            }
            break;
          }

          case NAND:{
            d_frontier_stack.back()[current_node->indx] = 0;
            if (current_node->value_stack.back() == ZERO){ //when output is 0, all input should be 1
              for (i = 0; i < current_node->fin; i++){
                if (current_node->unodes[i]->value_stack.back() != current_node->value_stack.back()){
                  imply_stack_node.push_back(current_node->unodes[i]->num);
                  imply_stack_dir.push_back(0);
                  imply_stack_val.push_back(ONE);
                }
                j_frontier_stack.back()[current_node->indx] = 0;
              }
            }
            else { //when output is 1
              count = 0;
              temp_to_be_modified = -1;
              has_D = 0;
              has_D_BAR = 0;
              has_one_or_zero = 0;
              for (i = 0; i < current_node->fin; i++){
                if ((current_node->unodes[i]->value_stack.back() == ONE) || (current_node->unodes[i]->value_stack.back() == D) || (current_node->unodes[i]->value_stack.back() == D_BAR)){ //when output is 0, all input should be 0
                  count = count + 1;
                  if (current_node->unodes[i]->value_stack.back() == D){
                    has_D = 1;
                  }
                  else if(current_node->unodes[i]->value_stack.back() == D_BAR){
                    has_D_BAR = 1;
                  }
                }
                else if (current_node->unodes[i]->value_stack.back() == X){
                  temp_to_be_modified = i;
                }
                else{  // If one of the input is ZERO, break, no need to update
                  j_frontier_stack.back()[current_node->indx] = 0;
                  has_one_or_zero = 1;
                  break;
                }
              }

              if ((count == current_node->fin - 1) && (temp_to_be_modified >= 0)){  // If there is only one X in the input and all the other inputs are non controlling
              if ((has_one_or_zero) || (has_D && has_D_BAR)){
                  ;
              }
                else{
                  imply_stack_node.push_back(current_node->unodes[temp_to_be_modified]->num);
                  imply_stack_dir.push_back(0);
                  imply_stack_val.push_back(ZERO);
                }
                j_frontier_stack.back()[current_node->indx] = 0;
              }
            else{  // If more than one X or no X in the gate input
                if (temp_to_be_modified < 0){ // If no X in the gate input
                  j_frontier_stack.back()[current_node->indx] = 0;
                }
                else{  //If more than one X in the gate input
                  if (has_one_or_zero){  // If there is a controlling value in the gate input
                    j_frontier_stack.back()[current_node->indx] = 0;
                  }
                  else if ((has_D && has_D_BAR)){
                    j_frontier_stack.back()[current_node->indx] = 0;
                  }
                  else{
                     j_frontier_stack.back()[current_node->indx] = 1;
                  }
                }
            }
            }
            break;
          }

          case NOT:{
            ipt_node = current_node->unodes[0];
            if (ipt_node->value_stack.back() != current_node->value_stack.back()){
              imply_stack_node.push_back(ipt_node->num);
              imply_stack_dir.push_back(0);
              if (current_node->value_stack.back() == ONE){
                imply_stack_val.push_back(ZERO);
              }
              else{
                imply_stack_val.push_back(ONE);
              }
            }
            break;
          }
          default:;
      }
      // For the faulty node, if its input indicate a 1/0 value, we still make it D/D_bar
      if (current == fault_node){
        if (current_node->value_stack.back() == ONE){
          current_node->value_stack.back() = D;
        }
        else if (current_node->value_stack.back() == ZERO){
          current_node->value_stack.back() = D_BAR;
        }
      }
    }

    last_imply_node = current;  // Record the current node. Will be used in the next iteration. 
  }
  return 1;
}


int podem(char *cp)
{ 
  pc(Cktname);
  lev((char *)(""));
  // Get the faults from user input
  int fault_node, fault_type;
  int seed_podem, seed_podem_temp;
  podem_fail_count = 0;
  //sscanf(cp, "%d %d", &fault_node, &fault_type);
  //
  d_frontier_array = new int[Nnodes];
  // Create stacks for D, J, and imply&check
  //d_frontier = new int[Nnodes];
  //j_frontier = new int[Nnodes];  
   //std::vector<int> imply_stack_node;
   //std::vector<int> imply_stack_dir; // 1 means forward and 0 means backward
   //std::vector<unsigned int> imply_stack_val;  // 0 - zero; 7 - one; 6 - D; 1 - D_bar; 3 = X
  if (!atpg_flag)
   {
     sscanf(cp, "%d %d", &fault_node, &fault_type);
   }
   else
   {
    fault_node = atpg_to_alg_fault_node.back();
    fault_type = atpg_to_alg_fault_type.back();
    atpg_to_alg_fault_node.pop_back();
    atpg_to_alg_fault_type.pop_back();
   }
   //printf("Faulty node: %d s@%d\n", fault_node, fault_type);
   d_frontier_stack.clear();
   obj_output_node.clear();
   obj_output_val.clear();
   struct n_struc *current_node;
   current_node = starting_node;
   //printf("0\n");
   while(1){
      //printf("current_node->index:%d\n",current_node->indx);
      d_frontier_array[current_node->indx] = 0;
      if (current_node->next_node == NULL){
        current_node = starting_node;
        break;
      }
      current_node = current_node->next_node;
   }
   //printf("3\n");
   d_frontier_stack.push_back(d_frontier_array);
  // Initialize fault value for each node
  //current_node = starting_node;
  while(1){                       
    current_node->value_stack.push_back(X); // Initialize for PODEM
    //d_frontier[current_node->indx] = 0;
    //j_frontier[current_node->indx] = 0;
    if (current_node->next_node == NULL){
      break;
    }
    current_node = current_node->next_node;
  }
  // initialize
  podem_start_iter = 1;
  //printf("2\n");
  // Call real PODEM
  int podem_flag;
  podem_flag = real_podem(fault_node, fault_type);
  if (podem_flag){
    if (!atpg_flag){  // If called by terminal directly
      printf("PODEM success!\n");
      // Print the result to file
      char output_file_name[81];
      char fault_node_str[20], fault_type_str[20];
      for (int ind = 0; ind < 81; ind++){
        output_file_name[ind] = Cktname[ind];
      }
      strcat(output_file_name, "_PODEM_");
      sprintf(fault_node_str, "%d", fault_node);
      strcat(output_file_name, fault_node_str);
      strcat(output_file_name, "@");
      sprintf(fault_type_str, "%d", fault_type);
      strcat(output_file_name, fault_type_str);
      strcat(output_file_name, ".txt");
      FILE * podem_report;
      podem_report = fopen(output_file_name, "w");
      for (int i = 0; i < Npi; i++){
        if (Pinput[i]->value_stack.back() == ONE){
          fprintf(podem_report, "%d,1\n", Pinput[i]->num);
        }
        else if (Pinput[i]->value_stack.back() == ZERO){
          fprintf(podem_report, "%d,0\n", Pinput[i]->num);
        }
        else if (Pinput[i]->value_stack.back() == D){
          fprintf(podem_report, "%d,1\n", Pinput[i]->num);
        }
        else if (Pinput[i]->value_stack.back() == D_BAR){
          fprintf(podem_report, "%d,0\n", Pinput[i]->num);
        }
        else{
          fprintf(podem_report, "%d,X\n", Pinput[i]->num);
        }
      }
      fclose(podem_report);
    }
    else      //called by atpg
    {
      //printf("atpg call D-alg success!\n");
      podem_to_atpg_success = 1;       //flag to atapg is set
      for (int i = 0; i < Npi; i++)
      {
        if (Pinput[i]->value_stack.back() == ONE){
          //printf("%d,1\n", Pinput[i]->num);
          alg_return_test_vector.push_back(1);
        }
        else if (Pinput[i]->value_stack.back() == ZERO){
          //printf("%d,0\n", Pinput[i]->num);
          alg_return_test_vector.push_back(0);
        }
        else if (Pinput[i]->value_stack.back() == D){
          //printf("%d,1\n", Pinput[i]->num);
          alg_return_test_vector.push_back(1);
        }
        else if (Pinput[i]->value_stack.back() == D_BAR){
          //printf("%d,0\n", Pinput[i]->num);
          alg_return_test_vector.push_back(0);
        }
        else{
          //printf("%d,X\n", Pinput[i]->num);
          seed_podem = rand() % 6335;
          srand(seed_podem);
          seed_podem_temp = rand() % 2;
          alg_return_test_vector.push_back(seed_podem_temp);  //if x shows, provide default 0
        }
      }
    }
    return 1;
  }
  else{
    if (!atpg_flag){  // If called by terminal directly
      printf("PODEM failed!\n");
    }
    else{  // TODO_ZM: Add the atpg required information here. 
      podem_to_atpg_success = 0;
      printf("atpg called, then PODEM failed!\n");
    }
    return 0;
  }

  return 0;
}

int real_podem(int fault_node, int fault_type)
{
  std::vector<int> imply_stack_node;
  std::vector<int> imply_stack_dir; // 1 means forward and 0 means backward
  std::vector<unsigned int> imply_stack_val;  // 0 - zero; 7 - one; 6 - D; 1 - D_bar; 3 = X
  std::vector<int> backtrace_output_node_vector;
  std::vector<unsigned int> backtrace_output_val_vector;

  int count;
  int obj_flag;   //1 means obj success, 0 means fail
  int error_at_PO; //1 means X/D/DBAR at obj output
  int i;
  error_at_PO = 0;
  struct n_struc *current_node; //zh

  podem_fail_count = podem_fail_count + 1;
  // printf("%d\n", podem_fail_count);
  if (podem_fail_count > 5000)
  {
    return 0;
  }
  // printf("---------------------------------\n");
  current_node = starting_node;
  while(1){
    // printf("%d, %u\n", current_node->num, current_node->value_stack.back());
    if (current_node->next_node == NULL){
      break;
    }
    current_node = current_node->next_node;
  }

  for (i = 0; i < Npo; i++){
    if ((Poutput[i]->value_stack.back() == D) || (Poutput[i]->value_stack.back() == D_BAR)){
       error_at_PO = 1;
      return 1;  // If error at PO, success. 
    }
  }
   count = 0;
   for (i = 0; i < Npo; i++)
   {
     if ((Poutput[i]->value_stack.back() == ONE) || (Poutput[i]->value_stack.back() == ZERO)){
       count = count + 1;
     }
   }
   if(count == Npo){
    return 0;
   }

  obj_flag = obj(fault_node, fault_type);
  if (obj_flag == 0){
    return 0;
  }
  // printf("obj is finished\n");
  // Call backtrace function
  backtrace();
  // printf("backtrace is finished\n");
  backtrace_output_node_vector.push_back(backtrace_output_node);
  backtrace_output_val_vector.push_back(backtrace_output_val);
  // Call imply
  
  current_node = map_num_node[backtrace_output_node_vector.back()];
  current_node->value_stack.push_back(backtrace_output_val);
  //For debug 
  // printf("podem current number %d\n", current_node->num);
  imply_stack_val.push_back(backtrace_output_val); 
  imply_stack_dir.push_back(1);
  imply(fault_node,  fault_type,imply_stack_dir,imply_stack_val);
  int podem_success;
  podem_success = real_podem(fault_node, fault_type);
  if (podem_success){
    return 1;
  }
  // printf("second podem call current node number %d\n", backtrace_output_node_vector.back());
  current_node = map_num_node[backtrace_output_node_vector.back()];
  current_node->value_stack.push_back((unsigned int)15 - backtrace_output_val_vector.back());
  imply_stack_dir.push_back(1);
  imply(fault_node,  fault_type,imply_stack_dir,imply_stack_val);
  podem_success = real_podem(fault_node, fault_type);
  if (podem_success){
    return 1;
  }
  current_node = map_num_node[backtrace_output_node_vector.back()];
  current_node->value_stack.push_back(X);
  imply_stack_dir.push_back(1);
  imply(fault_node, fault_type, imply_stack_dir,imply_stack_val);
  obj_output_node.pop_back();
  obj_output_val.pop_back();
  return 0;
}

int obj(int fault_node, int fault_type){
  // printf("begin obj\n");
  //fault node value is unknown
  if (map_num_node[fault_node]->value_stack.back() == X){       
    obj_output_node.push_back(fault_node);
    if (fault_type){
      obj_output_val.push_back(ZERO);
    }
    else{
      obj_output_val.push_back(ONE);
    }
    return 1;                   //return since nothing need to be done
  }
  // falut node do not excite
  if (fault_type){
    if (map_num_node[fault_node]->value_stack.back() != D_BAR){
      return 0;
    }
  }
  else {
    if (map_num_node[fault_node]->value_stack.back() != D){
      return 0;
    }
  }
  //
  int i;
  for (i = 0; i < Nnodes; i++){
    // printf("d_frontier:%d valid:%d\n", i, d_frontier_stack.back()[i]);
    if (d_frontier_stack.back()[i] == 1){
      break;
    }
    else if (i == Nnodes-1){
      if (d_frontier_stack.back()[i] == 0)
      {
        return 0;
      }
    }
  }
  int j;
  for (j = 0; j < map_num_node[map_indx_num[i]]->fin; j++){
    if (map_num_node[map_indx_num[i]]->unodes[j]->value_stack.back() == X){
      break;
    }
    else if (j == map_num_node[map_indx_num[i]]->fin - 1)
    {
      if (map_num_node[map_indx_num[i]]->unodes[map_num_node[map_indx_num[i]]->fin - 1]->value_stack.back() != X)
      {
        return 0;
      }
    }
  }
  obj_output_node.push_back(map_num_node[map_indx_num[i]]->unodes[j]->num);
  if ((map_num_node[map_indx_num[i]]->type == AND) || (map_num_node[map_indx_num[i]]->type == NAND)){
    obj_output_val.push_back(ONE);
  }
  else if ((map_num_node[map_indx_num[i]]->type == OR) || (map_num_node[map_indx_num[i]]->type == NOR)){
    obj_output_val.push_back(ZERO);
  }
  else if (map_num_node[map_indx_num[i]]->type == XOR){
    obj_output_val.push_back(ONE);
  }
  // printf("end 4 obj\n");
  return 1;
}

int backtrace(void){
  // printf("begin backtrace\n");
  unsigned int v;
  int k;
  int j;
  unsigned int i;
  int counter_one = 0;
  int counter_zero = 0;

  v = obj_output_val.back();
  k = obj_output_node.back();
  // printf("backtrace k:%d\n", k);
  while (map_num_node[k]->type != IPT){
    if ((map_num_node[k]->type == AND) || (map_num_node[k]->type == OR)){
      i = ZERO;
      for (j = 0; j < map_num_node[k]->fin; j++){
        if (map_num_node[k]->unodes[j]->value_stack.back() == X){ //choose one input whose value is x
          break;
        }
      }
      v = v ^ i;
      k = map_num_node[k]->unodes[j]->num;
    }
    else if ((map_num_node[k]->type == NAND) || (map_num_node[k]->type == NOR) || (map_num_node[k]->type == NOT)){
      i = ONE;
      for (j = 0; j < map_num_node[k]->fin; j++){
        if (map_num_node[k]->unodes[j]->value_stack.back() == X){ //choose one input whose value is x
          break;
        }
      }
      v = v ^ i;
      k = map_num_node[k]->unodes[j]->num;
    }
    else if (map_num_node[k]->type == XOR){
      counter_one = 0;
      counter_zero = 0;
      for (j = 0; j < map_num_node[k]->fin; j++){
        if (map_num_node[k]->unodes[j]->value_stack.back() == ONE){
          counter_one = counter_one + 1;
        }
        else if (map_num_node[k]->unodes[j]->value_stack.back() == ZERO){
          counter_zero = counter_zero + 1;
          // printf("counter_zero:%d\n", counter_zero);
        }
      }
      // printf("%u\n", v);
      if (v == ONE){
        // printf("enter ONE, counter_one:%d, counter_zero:%d\n",counter_one, counter_zero);
        if (counter_one % 2 == 0){
          for (j = 0; j < map_num_node[k]->fin; j++){
            if (map_num_node[k]->unodes[j]->value_stack.back() == X){
              v = ONE;
              k = map_num_node[k]->unodes[j]->num;
              break;
            }
          }
        }
        else if(counter_one % 2 == 1){
          for (j = 0; j < map_num_node[k]->fin; j++){
            if (map_num_node[k]->unodes[j]->value_stack.back() == X){
              v = ZERO;
              k = map_num_node[k]->unodes[j]->num;
              break;
            }
          }
        }
      }
      else if (v == ZERO){
        // printf("enter ZERO, counter_one:%d, counter_zero:%d\n",counter_one, counter_zero);
        if (counter_one % 2 == 0){
          // printf("enter a, counter_one:%d, counter_zero:%d\n",counter_one, counter_zero);
          for (j = 0; j < map_num_node[k]->fin; j++){
            if (map_num_node[k]->unodes[j]->value_stack.back() == X){
              // printf("enter if, counter_one:%d, counter_zero:%d\n",counter_one, counter_zero);
              v = ZERO;
              k = map_num_node[k]->unodes[j]->num;
              // printf("ZERO k:%d\n", k);
              break;
            }
          }
        }
        else if(counter_one % 2 == 1){
          for (j = 0; j < map_num_node[k]->fin; j++){
            if (map_num_node[k]->unodes[j]->value_stack.back() == X){
              v = ONE;
              k = map_num_node[k]->unodes[j]->num;
              // printf("ONE k:%d\n", k);
              break;
            }
          }
        }
      }
    }
    else{ //BRANCH
      i = ZERO;
      v = v ^ i;
      k = map_num_node[k]->unodes[0]->num;
    }
  }
  backtrace_output_val = v;
  backtrace_output_node = k;
  //printf("PI %d value: %u\n",backtrace_output_node, backtrace_output_val);
  return 0;
}

int imply(int fault_node,int fault_type, std::vector<int> imply_stack_dir,std::vector<unsigned int> imply_stack_val){
  // printf("begin imply\n");
  //int current;
  d_frontier_array = new int[Nnodes];
  //int d_frontier_array[Nnodes];
  //unsigned int current_val;
  int current_dir;
  struct n_struc *current_node;
  int i, j;
  int count;
  int Dfont_flag=0;
  unsigned int temp;
  unsigned int temp_value;
  //int has_D, has_D_BAR, has_one_or_zero;  // Used in some forward implication cases
  current_node = starting_node; 
  //--------d_frontier_array initial------------//
  while(1){
    d_frontier_array[current_node->indx] = 0;
    if (current_node->next_node == NULL){
      current_node = starting_node;
      break;
    }
    current_node = current_node->next_node;
  }
  
  while (1){  
    //current_dir = imply_stack_dir.back();
    // Forward propagation
    if (current_node->num == fault_node) {
    Dfont_flag = 1; //find D/D_bar
    // printf("flag successful\n");
    }
    switch (current_node->type){ // enum e_gtype {IPT, BRCH, XOR, OR, NOR, NOT, NAND, AND}; 
      case BRCH:{
        current_node->value_stack.push_back(current_node->unodes[0]->value_stack.back());       //modified
        break;
      }
      case XOR:{
        if ((current_node->unodes[0]->value_stack.back() != X) || (current_node->unodes[1]->value_stack.back() != X))
        {//because two X xor results ZERO
          temp_value = current_node->unodes[0]->value_stack.back() ^ current_node->unodes[1]->value_stack.back();
        }
        else{
          temp_value = X;
        }
        if ((temp_value!=ONE)&&(temp_value!=ZERO)&&(temp_value!=D)&&(temp_value!=D_BAR)){
          temp_value = X;
        }
        current_node->value_stack.push_back(temp_value) ;
      
        if (current_node->value_stack.back()==X){
          for (i = 0; i < current_node->fin; i++){
            if (current_node->unodes[i]->value_stack.back()== D ||current_node->unodes[i]->value_stack.back()== D_BAR){
              d_frontier_array[current_node->indx] = 1;
              //printf("d_frontier is %d\n", current_node->num);
            }
          }
        }
        // printf("XOR: %d %d\n", current_node->num, current_node->value);
        break;
      }
      case OR:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value_stack.back();
          }
          else{
            temp_value = temp_value | (current_node->unodes[i]->value_stack.back());
          }
        }
        if ((temp_value!=ONE)&&(temp_value!=ZERO)&&(temp_value!=D)&&(temp_value!=D_BAR)){
          temp_value = X;
        }
        current_node->value_stack.push_back(temp_value) ;
        if (current_node->value_stack.back()==X){
          for (i = 0; i < current_node->fin; i++){
            if (current_node->unodes[i]->value_stack.back()== D ||current_node->unodes[i]->value_stack.back()== D_BAR){
              d_frontier_array[current_node->indx] = 1;
              
            }
          }
        }
        break;
      }
      case NOR:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value_stack.back();
          }
          else{
            temp_value = temp_value | (current_node->unodes[i]->value_stack.back());
          }
        }
        temp_value = (unsigned int)15 - temp_value;
        if ((temp_value!=ONE)&&(temp_value!=ZERO)&&(temp_value!=D)&&(temp_value!=D_BAR)){
          temp_value = X;
        }
        current_node->value_stack.push_back(temp_value) ;
        if (current_node->value_stack.back()==X){
          for (i = 0; i < current_node->fin; i++){
            if (current_node->unodes[i]->value_stack.back()== D ||current_node->unodes[i]->value_stack.back()== D_BAR){
              d_frontier_array[current_node->indx] = 1;
              
            }
          }
        }
        break;
      }
      case NOT:{
        temp_value = (unsigned int)15 - (current_node->unodes[0]->value_stack.back());
        if ((temp_value!=ONE)&&(temp_value!=ZERO)&&(temp_value!=D)&&(temp_value!=D_BAR)){
          temp_value = X;
        }
        current_node->value_stack.push_back(temp_value) ;
        if (current_node->value_stack.back()==X){
          for (i = 0; i < current_node->fin; i++){
            if (current_node->unodes[i]->value_stack.back()== D ||current_node->unodes[i]->value_stack.back()== D_BAR){
              d_frontier_array[current_node->indx] = 1;
            }
          }
        }
        // printf("%d %d \n", current_node->num, current_node->value);
        break;
      }
      case NAND:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value_stack.back();
          }
          else{
            temp_value = temp_value & (current_node->unodes[i]->value_stack.back());
          }
        }
        temp_value = (unsigned int)15-temp_value;
       
        if ((temp_value!=ONE)&&(temp_value!=ZERO)&&(temp_value!=D)&&(temp_value!=D_BAR)){
          temp_value = X;
        }
        current_node->value_stack.push_back(temp_value) ;
        // printf("current node %d  temp value %u\n",current_node->num,temp_value);
        if (current_node->value_stack.back()==X){
          for (i = 0; i < current_node->fin; i++){
            if (current_node->unodes[i]->value_stack.back()== D ||current_node->unodes[i]->value_stack.back()== D_BAR){
              d_frontier_array[current_node->indx] = 1;
              
            }
          }
        }
        break;
      }
      case AND:{
        for (i = 0; i < current_node->fin; i++){
          if (i == 0){
            temp_value = current_node->unodes[i]->value_stack.back();
          }
          else{
            temp_value = temp_value & (current_node->unodes[i]->value_stack.back());
          }
        }
        if ((temp_value!=ONE)&&(temp_value!=ZERO)&&(temp_value!=D)&&(temp_value!=D_BAR)){
          temp_value = X;
        }
        current_node->value_stack.push_back(temp_value) ;
        if (current_node->value_stack.back()==X){
          for (i = 0; i < current_node->fin; i++){
            if (current_node->unodes[i]->value_stack.back()== D ||current_node->unodes[i]->value_stack.back()== D_BAR){
              d_frontier_array[current_node->indx] = 1;
              
            }
          }
        }
        break;
        }
      default:;
    }
    if (Dfont_flag) { //change 1,0 to d/d_bar
      if (fault_type){
        temp = ONE;
      }else {
        temp = ZERO;
      }
      if((fault_node==current_node->num)&&(temp == ((unsigned int)15 - current_node->value_stack.back()))){
      //printf("enter second flag\n");
        if (current_node->value_stack.back()==ONE)
        {
          current_node->value_stack.pop_back();
          current_node->value_stack.push_back(D);
          //printf("change 1 to D\n");
        }
        else if (current_node->value_stack.back()==ZERO){
          current_node->value_stack.pop_back();
          current_node->value_stack.push_back(D_BAR);
        }   
      }
    }
    // printf("%d %d \n", current_node->num, current_node->value);
    
    if (current_node->next_node == NULL)
    { 
      d_frontier_flag = 0;
      for (j = 0; j < Nnodes; j++)
      {
        if (d_frontier_array[j] == 1)
        {
         d_frontier_flag = 1;
        }
      }
      d_frontier_stack.push_back(d_frontier_array); //save d_frontier to d_fontier_stack
      break;
    }
    current_node = current_node->next_node; 
  }
  return 1;
}

int atpg_det(char *cp)
{
    time_t start, finish;
    time(&start);
    //-----------time start here--------//
    FILE *ATPG_report;
    FILE *ATPG_patterns_report;
    unsigned long consumed_time;
    char circuit_name[20], alg_name[20];
    int break_counter, seed, tvector;         //bit for random number 
    int fault_count, fault_count_temp, Total_fault_counter;        //fault_count from fault dic for fault coverage calculation
    float fault_coverage, Overall_fault_coverage;
    sscanf(cp, "%s %s", circuit_name, alg_name);
    atpg_flag = 1;
    //----------------step 1------------------------//step 1: read circuit
    cread(circuit_name);              
    //----------------step 2------------------------//step 2: levelzation output: lev_report.txt
    //lev(circuit_name);                
    pc(circuit_name);
    lev((char*)(""));
    //----------------step 3------------------------//build input_vector array
    // printf("%d\n", Npi); 
    //----------------step 4-----------------------//build vector of PI
    struct n_struc *current_node;         //
    current_node = starting_node;
    // Create the PI node list
    while(1)
    {
      if (current_node->type == IPT)  
        input_test_vector_node.push_back(current_node->num);
      if (current_node->next_node == NULL)
        break;
      current_node = current_node->next_node;
    }
    //----------------step 5-----------------------//random generator
    //printf("finish PI vector\n");

    //----------initate counter-------------//
    break_counter = 0;                        //initiate for break_counter
    fault_coverage = 0;                       //initiate for fault coverage
    fault_count = 0;
    fault_count_temp = 0;
    //----------atpg_fault_dic-------------//
    // Initialize the fault list
    int atpg_fault_dic[2 * Nnodes];             //fault dic//1 row
    for (int i = 0; i < 2 * Nnodes; i++)        //initate atpg_fault_dic
    {
      atpg_fault_dic[i] = 0;                  //1 means valid
    }
    //------------------------------------//
    //----------build local storage of test vector----------//
    std::vector<int> atpg_test_vector;        //local vector, store the test vector, group of Npi;
    std::vector<int> atpg_test_vector_fault_list_unpop;
    int atpg_test_vector_node[2 * Nnodes];      //build array to store the node: node5, node5, node6, node6, since sa0, sa1;
    //-------------------------------------------------------//
    int node_update_flag;       //only update the node number once when call atpg
    node_update_flag = 0;
  
    while(1)                    
    {
      break_counter = break_counter + 1;      //for each loop: counter=counter++
      fault_count = 0;
      seed = rand() % 6553;  // YH_atpg: Change the seed to real random if possible
      srand(seed);
      // Random input vector generation
      for (int i = 0; i < Npi; i++)
      { 
        tvector = rand() % 2;
        input_test_vector_value.push_back(tvector); //To dfs, random generate test vector
        atpg_test_vector.push_back(tvector);        //To storage
        //printf("%d\n", input_test_vector_value[i]);
      }
      dfs(circuit_name);                       //dfs will modify the fault dictionary

      // strcat(output_file_name, "_PODEM_");
      // sprintf(fault_node_str, "%d", fault_node);
      // strcat(output_file_name, fault_node_str);
      // strcat(output_file_name, "@");
      // sprintf(fault_type_str, "%d", fault_type);
      // strcat(output_file_name, fault_type_str);
      // strcat(output_file_name, ".txt");

      //printf("done dfs\n");
      //---------------step 6---------------------//fault coverage
      for (int i = 0; i <2 * Nnodes; i++)
      {
        atpg_test_vector_fault_list_unpop.push_back(fault_dic[i]);    //fault left shift <<  
        if (atpg_fault_dic[i] == 0)              //update local dictionary
        {
          atpg_fault_dic[i] = fault_dic[i];
        }
        // Count the fault coverage
        if (atpg_fault_dic[i] == 1)
        {
          fault_count = fault_count + 1;
        }
      }
      //---------------store node number------------//  YH_atpg: Move this out of while()
      if (node_update_flag == 0)
      {
        for (int i = 0; i < 2 * Nnodes; i++)
        {
          atpg_test_vector_node[i]  = fault_dic_node[i];
        } 
      }
      node_update_flag = 1;
      //--------------------------------------//
      fault_dic.clear();                 //clear the vector for next initate
      fault_dic_node.clear();
      if (fault_count > fault_count_temp) //fault_coverage change
      {
        fault_count_temp = fault_count;
        /*printf("fault_counter%d\n", fault_count);
        printf("2node%d\n", (2*Nnodes));*/
        fault_coverage = ((float)fault_count_temp) / (2 * Nnodes);
        break_counter = 0;                      //clear bread counter
        //printf("fault coverage is %.5f\n", fault_coverage);
      }
      
      // If no more improvement is made by random input vectors the break this loop and go to D or PODEM. 
      if (break_counter == Npi)     //break condition // YH_atpg: Reconsider this as a function of the circuit size
      {
        break;
      }
      //for debug
      // break;
    }
    
    //--------------step 7-----------------// To D_alg, PODEM
    
    char PODEM_ptr[] = "PODEM";
    char DALG_ptr[] = "DALG";
    int dfs_verify_dalg = 0;        //flag for dfs to verify: if the test vector provided by dalg could be dectected by dfs
    int dfs_verify_podem = 0;
    int fake_dalg_counter = 0;      //counter fake test vector from dalg
    int fake_podem_counter = 0;
    //char void_ptr[] = "void";
    std::vector<int> local_storage_valid_alg_PI_value_unpop;
    int local_storage_valid_alg_PI_depth;
    if (strcmp (alg_name, DALG_ptr) == 0)  // If D-alg is selected
    {
      printf("Running DALG... \n");
      //--------provide the node and fault type to dalg------//
      //------------debug-------------//
      /*
      for (int i = 0; i < 2*Nnodes; i++)
      {
        printf("atpg_test_vector_node: %d\n", atpg_test_vector_node[i]);
      }
      */
      //----------------------------//
      for (int i = 0; i < 2 * Nnodes; i++)
      {
        //------------------for 1 node--------------//
        if (atpg_fault_dic[i] == 0)   //this fault is not detected by random test vector
        {
          atpg_to_alg_fault_node.push_back(atpg_test_vector_node[i]);
          atpg_to_alg_fault_type.push_back((i+3)%2);
          alg_return_test_vector.clear();     //clear the temp container each time enter dalg
          dalg(circuit_name);       //enter void
          //----------if dalg return success----------//
          if (dalg_to_atpg_success)  // If D-alg fails
          {
            dalg_to_atpg_success = 0;    //reset flag
            //step 1: call dfs to detect 
            for (int j = 0; j < Npi; j++)
            { 
              input_test_vector_value.push_back(alg_return_test_vector[j]); //To dfs
              /*printf("atpg give to dfs to verify dalg%d\n", input_test_vector_value[j]);*/
            }
            //printf("prepare to enter dfs to check dalg\n");
            fault_dic.clear();      //clear before enter dfs
            dfs(circuit_name);
            //step 2: chechk if real dected by dfs
            for (int m = 0; m < 2 * Nnodes; m++)       //search the fault dic
            {
              if ((fault_dic_node[m] == atpg_test_vector_node[i]) && ((m+3)%2 == (i+3)%2) && (fault_dic[m] == 1))  
              {                                 //if (same node && same sa fault && valid)                       //sa1 sa0
                dfs_verify_dalg = 1;
              }
              if ((fault_dic_node[m] == atpg_test_vector_node[m]) && (fault_dic[m] == 1))    //since 2*Nnode, if (same node && valid)
              {
                atpg_fault_dic[m] = 1;
              }
              atpg_test_vector_fault_list_unpop.push_back(fault_dic[m]); 
            }
            //if real detect, valid = 1
            if (dfs_verify_dalg == 1)
            {
              dfs_verify_dalg = 0;    //clear the flag
              atpg_fault_dic[i] = 1;       //fault can be dectected
              for (int k = 0; k < Npi; k++)//receieve the test vector bit after bit
              {
                atpg_test_vector.push_back(alg_return_test_vector[k]);
              }
            }
            else
            {
              fake_dalg_counter = fake_dalg_counter + 1; //dalg proveid a fake test vector   cannot dfs_verify_dalg = 0    
              // YH_aptg: This is not good whthout the following for loop. 
              for (int k = 0; k < Npi; k++)//receieve the test vector bit after bit
              {
                atpg_test_vector.push_back(alg_return_test_vector[k]);
              }

            }
            // printf("fake_dalg_counter is %d\n", fake_dalg_counter);
          }
        }
        // atpg_test_vector_fault_list_unpop.push_back(atpg_fault_dic[i]); 
      }
    //------------finish dalg------------//
      //copy the PI test vector to array---------//
      
    }


    else if (strcmp (alg_name, PODEM_ptr) == 0)  // If PODEM is selected
    {
      printf("Running PODEM... \n");
      //--------provide the node and fault type to dalg------//
      //------------debug-------------//
      /*
      for (int i = 0; i < 2*Nnodes; i++)
      {
        printf("atpg_test_vector_node: %d\n", atpg_test_vector_node[i]);
      }
      */
      //----------------------------//
      for (int i = 0; i < 2 * Nnodes; i++)
      {
        //------------------for 1 node--------------//
        if (atpg_fault_dic[i] == 0)   //this fault cannot be detected by random test vector
        {
          atpg_to_alg_fault_node.push_back(atpg_test_vector_node[i]);
          atpg_to_alg_fault_type.push_back((i+3)%2);
          alg_return_test_vector.clear();     //clear the temp container each time enter dalg
          podem(circuit_name);       //enter void
        //----------if podem return success----------//
          if (podem_to_atpg_success)
          {
            podem_to_atpg_success = 0;    //reset flag
            //step 1: call dfs to detect 
            for (int j = 0; j < Npi; j++)
            { 
              input_test_vector_value.push_back(alg_return_test_vector[j]); //To dfs
              /*printf("atpg give to dfs to verify dalg%d\n", input_test_vector_value[j]);*/
            }
            //printf("prepare to enter dfs to check dalg\n");
            fault_dic.clear();      //clear before enter dfs
            dfs(circuit_name);
            //step 2: chechk if real dected by dfs
            for (int m = 0; m < 2 * Nnodes; m++)       //search the fault dic
            {
              if ((fault_dic_node[m] == atpg_test_vector_node[i]) && ((m+3)%2 == (i+3)%2) && (fault_dic[m] == 1))  
              {                                 //if (same node && same sa fault && valid)                       //sa1 sa0
                dfs_verify_podem = 1;
              }
              if ((fault_dic_node[m] == atpg_test_vector_node[m]) && (fault_dic[m] == 1))    //since 2*Nnode, if (same node && valid)
              {
                atpg_fault_dic[m] = 1;
              }
              atpg_test_vector_fault_list_unpop.push_back(fault_dic[m]); 
            }
            //if real detect, valid = 1
            if (dfs_verify_podem == 1)
            {
              dfs_verify_podem = 0;    //clear the flag
              atpg_fault_dic[i] = 1;       //fault can be dectected
              for (int k = 0; k < Npi; k++)//receieve the test vector bit after bit
              {
                atpg_test_vector.push_back(alg_return_test_vector[k]);
              }
            }
            else
            {
              fake_podem_counter = fake_podem_counter + 1; //dalg proveid a fake test vector   cannot dfs_verify_dalg = 0  
              for (int k = 0; k < Npi; k++)//receieve the test vector bit after bit
              {
                atpg_test_vector.push_back(alg_return_test_vector[k]);
              }           
            }
            //printf("fake_podem_counter is %d\n", fake_podem_counter);
          }
        }
        // atpg_test_vector_fault_list_unpop.push_back(atpg_fault_dic[i]); 
      }
    //------------finish podem-----------//
    }

    else  // Invalid command
    {
      printf("Alg doesn't exist\n");
    }

    //------------------------------------//
    //-------------step 8-----------------// Shrink the test vector
    

    unsigned long atpg_test_vector_depth;
    atpg_test_vector_depth = (atpg_test_vector.size())/ Npi;  // The depth is the sum of random and D/PODEM
    // atpg_test_vector_depth = (atpg_test_vector.size() + local_storage_valid_alg_PI_value_unpop.size())/ Npi;  // The depth is the sum of random and D/PODEM
    // printf("Size of the random: %d, size of the algorithm: %d\n", atpg_test_vector.size()/ Npi, local_storage_valid_alg_PI_value_unpop.size() / Npi);
    /*printf("atpg_test_vector_depth is %u\n", atpg_test_vector_depth);*/
    //--------------local storage---------------//
    int atpg_test_vector_test_vector[atpg_test_vector_depth][Npi];     //build a 2d array to store all input vector, # row = test vector depth, # column = test vector
    int atpg_test_vector_fault_list[atpg_test_vector_depth][2 * Nnodes]; //build a 2d array to store all fault list, # row = test vector depth, # column = all sa fault 
    int atpg_test_vector_valid[atpg_test_vector_depth][1];             //build valid bit
    
    //-------------initate the 2d array--------------//
    for (int i = 0; i < atpg_test_vector_depth; i++)
    {
      atpg_test_vector_valid[i][0] = 1;                                 //initate valid bit 
      for (int j = 0; j < Npi; j++){
        atpg_test_vector_test_vector[i][j] = atpg_test_vector[i*Npi+j];  //copy the vector to array
      }
    }
    for (int i = 0; i < atpg_test_vector_depth; i++)
    {
      for (int j = 0; j < 2 * Nnodes; j++)
        atpg_test_vector_fault_list[i][j] = atpg_test_vector_fault_list_unpop[i*(2*Nnodes)+j];
    }
    
    //----------------------------------------------//clear local vector storage after copy to the array
    atpg_test_vector.clear();
    atpg_test_vector_fault_list_unpop.clear();
    //-----------------Compare-------------------//start shrink the test vector
    int atpg_test_vector_fault_list_i, atpg_test_vector_fault_list_j, atpg_test_vector_fault_list_not_equal;
    atpg_test_vector_fault_list_i = 0;
    atpg_test_vector_fault_list_j = 0;
    atpg_test_vector_fault_list_not_equal = 0;           //not equal = 1 means equal = 0 
    for (int i = 0; i < atpg_test_vector_depth; i++)    //select one test vector 
    {
      if (atpg_test_vector_valid[i][0] == 1)            //first selected is valid
      {
        for (int j = 0; j < atpg_test_vector_depth; j++)  //select other test vector
        {
          if ((i != j) && (atpg_test_vector_valid[j][0] == 1))     //not the same test vector && selected test vector valid
          {
            for (int k = 0; k < 2 * Nnodes; k++)            //compare between 2 test vector
            {
              if ((atpg_test_vector_fault_list[i][k] == 1) && (atpg_test_vector_fault_list[j][k] == 0))
              {
                atpg_test_vector_fault_list_i = 1;        //atpg_test_vector_fault_list[i] domin atpg_test_vector_fault_list[j]
              }
              if ((atpg_test_vector_fault_list[i][k] == 0) && (atpg_test_vector_fault_list[j][k] == 1))
              {
                atpg_test_vector_fault_list_j = 1;      //atpg_test_vector_fault_list[i] is domined atpg_test_vector_fault_list[j]
              }
              if (atpg_test_vector_fault_list[i][k] != atpg_test_vector_fault_list[j][k])   //once different, flag = dectect different fault 
              {                                                                              
                atpg_test_vector_fault_list_not_equal = 1;
              }
            }
            if ((atpg_test_vector_fault_list_i == 0) && (atpg_test_vector_fault_list_j == 1))      //the first selected maybe domin other
            {
              atpg_test_vector_valid[i][0] = 0;
              atpg_test_vector_fault_list_i = 0;        //clear flag 
              atpg_test_vector_fault_list_j = 0; 
              atpg_test_vector_fault_list_not_equal = 0;
              break;
            }
            if ((atpg_test_vector_fault_list_i == 1) && (atpg_test_vector_fault_list_j == 0))   
            {
              atpg_test_vector_valid[j][0] = 0;
            }
            // }
            // if ((atpg_test_vector_fault_list_i == 0) && (atpg_test_vector_fault_list_j == 0))
            // {
            //   atpg_test_vector_valid[j][0] = 0;
            // }
            if (atpg_test_vector_fault_list_not_equal == 0)     //no one touch the flag
            {
              atpg_test_vector_valid[j][0] = 0;                 //cannot be i, since i may be domin other test vector
            }
            atpg_test_vector_fault_list_i = 0;        //clear flag 
            atpg_test_vector_fault_list_j = 0; 
            atpg_test_vector_fault_list_not_equal = 0;
          }
        }
      }
    }
    //-------------------------------------------//
    int reduced_test_vector_number;
    reduced_test_vector_number = 0;
    for (int i = 0; i < atpg_test_vector_depth; i++)
    {
      if (atpg_test_vector_valid[i][0] == 1)
      {
        reduced_test_vector_number = reduced_test_vector_number + 1;
        /*
        for (int j = 0; j < Npi; j++)
        {
          printf("%d", atpg_test_vector_test_vector[i][j]);
        }
        printf("test vector is \n");
        */
      }
    }
    //--------build an array to store the random valid vector-------//
    
    int valid_random_test_vector[reduced_test_vector_number][Npi];
    int valid_random_test_vector_counter = 0;
    for (int i = 0; i < atpg_test_vector_depth; i++)
    {
      if (atpg_test_vector_valid[i][0] == 1)
      { 
        for (int j = 0; j < Npi; j++)
        {
          valid_random_test_vector[valid_random_test_vector_counter][j] = atpg_test_vector_test_vector[i][j];
        }
        valid_random_test_vector_counter = valid_random_test_vector_counter + 1;
      }
    }

    //------------count total fault coverage--------//
    Total_fault_counter = 0;  //clear the counter
    Overall_fault_coverage = 0;
    for (int i = 0; i < 2 * Nnodes; i++)
    {
      if (atpg_fault_dic[i] == 1)     //fault can be dectected
      {
        Total_fault_counter = Total_fault_counter + 1;
      }
    }
    Overall_fault_coverage = ((float)Total_fault_counter) / (2*Nnodes);
    printf("Overall_fault_coverage is %.5f%\n", Overall_fault_coverage*100);

    //--------------print to file-----------------//
    //--------------step 1: print random test vector array-----------//
    //--------------step 2: print alg array-------------//
    //-----------fprintf first report--------//
    char atpt_pattern_report_name[81];
    int atpg_pattern_ind;

    for (atpg_pattern_ind = 0; atpg_pattern_ind < 81; atpg_pattern_ind++){
      atpt_pattern_report_name[atpg_pattern_ind] = Cktname[atpg_pattern_ind];
    }
    if (strcmp (alg_name, DALG_ptr) == 0){
      strcat(atpt_pattern_report_name, "_DALG_ATPG_patterns.txt");
    }
    else if (strcmp (alg_name, PODEM_ptr) == 0){
      strcat(atpt_pattern_report_name, "_PODEM_ATPG_patterns.txt");
    }
    else{
      strcat(atpt_pattern_report_name, "_No_algo_ATPG_patterns.txt");
    }

    ATPG_patterns_report = fopen(atpt_pattern_report_name, "w");
    for (int j = 0; j < Npi; j++)   //PI node number
    {
      if (j == (Npi-1))         //last one
      {
        fprintf(ATPG_patterns_report, "%d\n", Pinput[j]->num);
        printf("%d\n", Pinput[j]->num);
      }
      else
      {
        fprintf(ATPG_patterns_report, "%d,", Pinput[j]->num);
        printf("%d,", Pinput[j]->num);
      }
    }
    //random part
    for (int i = 0; i < reduced_test_vector_number; i++)
    {
      //print first row
      for (int k = 0; k < Npi; k++)   //PI node value
      {
        if (k == (Npi-1))
        {
          fprintf(ATPG_patterns_report, "%d\n", valid_random_test_vector[i][k]);
          printf("%d\n", valid_random_test_vector[i][k]);
        }
        else
        {
          fprintf(ATPG_patterns_report, "%d,", valid_random_test_vector[i][k]);
          printf("%d,", valid_random_test_vector[i][k]);
        }
      }
    }

    fclose(ATPG_patterns_report);

    //-------------fprintf second report-------//
    atpg_flag = 0;
    //------------time finish-------------//

    printf("reduced_test_vector_number is %d\n", reduced_test_vector_number);
    
    // printf("fault coverage is %.5f%\n", fault_coverage*100);
    // printf("fake_dalg_counter is %d\n", fake_dalg_counter);
    // printf("fake_podem_counter is %d\n", fake_podem_counter);
    //------------time finish-------------//
    time(&finish);
    consumed_time = difftime(finish, start);
    // printf("consumed_time is %d seconds\n", consumed_time);

    char atpt_report_name[81];
    int atpg_ind;
    for (atpg_ind = 0; atpg_ind < 81; atpg_ind++){
      atpt_report_name[atpg_ind] = Cktname[atpg_ind];
    }
    if (strcmp (alg_name, DALG_ptr) == 0){
      strcat(atpt_report_name, "_DALG_ATPG_report.txt");
    }
    else if (strcmp (alg_name, PODEM_ptr) == 0){
      strcat(atpt_report_name, "_PODEM_ATPG_report.txt");
    }
    else{
      strcat(atpt_report_name, "_No_algo_ATPG_report.txt");
    }

    ATPG_report = fopen(atpt_report_name, "w");
    //-------fprintf first row---------------//
    if (strcmp (alg_name, DALG_ptr) == 0)
    {
      fprintf(ATPG_report, "Algorithm: DALG\n");
      printf("\n----------------\nAlgorithm: DALG\n");
    }
    else if (strcmp (alg_name, PODEM_ptr) == 0)
    {
      fprintf(ATPG_report, "Algorithm: PODEM\n");
      printf("\n----------------\nAlgorithm: PODEM\n");
    }
    else
    {
      fprintf(ATPG_report, "Algorithm: Not exist\n");
      printf("Algorithm: Not exist\n");
    }
    //--------------------------------------//
    //-------fprint second row--------------//
    fprintf(ATPG_report, "Circuit: %s\n", netlist_file_name);
    printf("Circuit: %s\n", netlist_file_name);
    //------------------------------------//
    //------fprintf third row-------------//
    fprintf(ATPG_report, "Fault Coverage: %.2f%\n", Overall_fault_coverage*100);
    printf("Fault Coverage: %.2f%\n", Overall_fault_coverage*100);
    //-----------------------------------//
    //------fprintf fourth row-----------//
    fprintf(ATPG_report, "Time: %lu seconds\n", consumed_time);
    printf("Time: %lu seconds\n", consumed_time);
    //-------------------------------------//
    fclose(ATPG_report);
    return 0;
    
}

int atpg(char *cp) {

}