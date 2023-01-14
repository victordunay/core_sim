/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>

#define NUM_OF_REGISTERS (8)
#define MAX_NUM_OF_INSTRUCTIONS (100)

typedef int registers_table_t[NUM_OF_REGISTERS];
typedef enum
{
  IDLE = 0,
  BUSY = 1,
  DONE = 2
} state_t;

typedef enum
{
  FINE = 0,
  BLOCK = 1,
} configuration_t;


class Thread 
{
public:
    state_t state;
    unsigned num_of_instructions;
    registers_table_t registers_table;
    Instruction * instuctions_table;
    
    Thread(void)
    {
        this->num_of_instructions = 0;
        this->state = IDLE;
        memset(this->registers_table, 0, sizeof(registers_table_t));
    }
    void update_num_of_instructions(unsigned num_of_instructions)
    {
      this->num_of_instructions = num_of_instructions;
    }

    void initialize_instructions_table(void)
    {
      instuctions_table = new Instruction[num_of_instructions];
      for (unsigned instruction_index = 0; instruction_index < num_of_instructions; ++instruction_index)
      memset(&instuctions_table[instruction_index], 0, sizeof(Instruction));
    }

    void update_instructions_table(Instruction instruction, unsigned instruction_index)
    {
      instuctions_table[instruction_index] = instruction;
    }
    ~Thread() {};
};

class ThreadsTable
{
public:
    Thread * threads;
    unsigned num_of_threads;
    unsigned num_of_total_instructions;
    int load_latency;
    int store_latency;
    int switch_cycles_overhead;
    configuration_t configuration;

    ThreadsTable(configuration_t configuration)
    {

      this->load_latency = SIM_GetLoadLat();
      this->store_latency = SIM_GetStoreLat();
      this->num_of_threads = SIM_GetThreadsNum();
      this->switch_cycles_overhead = SIM_GetSwitchCycles();
      this->configuration = configuration;
      this->num_of_total_instructions = 0;
      threads = new Thread[num_of_threads];
      (void)initialize_insturction_table_of_all_threads();
    }

    void initialize_insturction_table_of_all_threads(void)
    {
      unsigned num_of_thread_instructions = 0;
      for (unsigned thread_index = 0; thread_index < num_of_threads ; ++thread_index)
      {
        (void)get_num_of_instructions((unsigned)thread_index, &num_of_thread_instructions);
        (void)threads[thread_index].update_num_of_instructions(num_of_thread_instructions);
        (void)threads[thread_index].initialize_instructions_table();
        (void)update_instruction_table((unsigned)thread_index, num_of_thread_instructions);
        (void)display_thread_instructions_table(thread_index, num_of_thread_instructions);
      }
    }
  
    void get_num_of_instructions(int thread_index, unsigned * num_of_thread_instructions)
    {
      *num_of_thread_instructions = 0;
      Instruction memory_line;

      for (uint32_t instruction_index = 0; instruction_index < MAX_NUM_OF_INSTRUCTIONS; ++instruction_index)
      {
        (void)SIM_MemInstRead(instruction_index, &memory_line, thread_index);
        (*num_of_thread_instructions)++;
        if (CMD_HALT == memory_line.opcode)
        {
          break;
        }
      }
    }

    void update_instruction_table(int thread_index, unsigned num_of_thread_instructions)
    {
      Instruction instruction;
      for (uint32_t instruction_index = 0; instruction_index < num_of_thread_instructions; ++instruction_index)
      {
        (void)SIM_MemInstRead(instruction_index, &instruction, thread_index);
        (void)threads[thread_index].update_instructions_table(instruction, (unsigned)instruction_index);
      }
    }

    /* for DEBUG PURPOSE ONLY!!*/
    void display_thread_instructions_table(int thread_index, unsigned num_of_instructions)
    {
      printf("=================== INSTRUCTION TABLE of TID %d ============\n\n", thread_index);
      for( unsigned instruction_index = 0; instruction_index < num_of_instructions; ++instruction_index)
      {
        printf("opcode = %d dst = %d src1 = %d src2 = %d is imm = %d \n",  threads[thread_index].instuctions_table[instruction_index].opcode,  threads[thread_index].instuctions_table[instruction_index].dst_index,  threads[thread_index].instuctions_table[instruction_index].src1_index,  threads[thread_index].instuctions_table[instruction_index].src2_index_imm,  threads[thread_index].instuctions_table[instruction_index].isSrc2Imm);
      }
    }
 
    ~ThreadsTable() {};
};

void CORE_BlockedMT()
{
	ThreadsTable threads_table(FINE);


}

void CORE_FinegrainedMT() {
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
