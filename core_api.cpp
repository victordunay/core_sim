/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>

#define NUM_OF_REGISTERS (8)
#define MAX_NUM_OF_INSTRUCTIONS (100)

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
    unsigned num_of_executed_instructions;
    tcontext registers_table;
    unsigned remained_cycles_at_busy_state;
    Instruction * instuctions_table;
    int load_latency;
    int store_latency;
    
    Thread(void)
    {
        this->num_of_instructions = 0;
        this->state = IDLE;
        this->remained_cycles_at_busy_state = 0;
        this->num_of_executed_instructions = 0;
        memset(&this->registers_table, 0, sizeof(tcontext));
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

    void initialize_latencies(int load_latency, int store_latency)
    {
      this->load_latency = load_latency;
      this->store_latency = store_latency;
    }
    void update_instructions_table(Instruction instruction, unsigned instruction_index)
    {
      instuctions_table[instruction_index] = instruction;
    }

    void execute_instruction()
    {
      Instruction current_instruction = instuctions_table[num_of_executed_instructions];
      int32_t memory_address = 0;

      switch (current_instruction.opcode) 
      { 
        case CMD_NOP:
          break;
        case CMD_SUB:
        case CMD_SUBI:
          registers_table.reg[current_instruction.dst_index] = registers_table.reg[current_instruction.src1_index] - registers_table.reg[current_instruction.src2_index_imm];
          break;
        case CMD_ADDI:
        case CMD_ADD:
          registers_table.reg[current_instruction.dst_index] = registers_table.reg[current_instruction.src1_index] + registers_table.reg[current_instruction.src2_index_imm];
        case CMD_LOAD:
          memory_address = current_instruction.src1_index + current_instruction.src2_index_imm;
          SIM_MemDataRead(memory_address, &current_instruction.dst_index);
          state = BUSY;
          remained_cycles_at_busy_state += load_latency;
          break;
        case CMD_STORE:
          memory_address = current_instruction.dst_index + current_instruction.src2_index_imm;
          SIM_MemDataWrite(memory_address, current_instruction.src1_index);
          state = BUSY;
          remained_cycles_at_busy_state += store_latency;
          break;
        case CMD_HALT:
          state = DONE;
          break;
      }

    }
    ~Thread() {};
};

class ThreadsTable
{
public:
    Thread * threads;
    unsigned num_of_threads;
    int load_latency;
    int store_latency;
    bool program_done;
    int switch_cycles_overhead;
    uint32_t memory_start_address;
    configuration_t configuration;
    unsigned thread_index_for_execute;
    unsigned previous_executed_thread_index;
    bool firt_execution;
    ThreadsTable(configuration_t configuration)
    {
      this->load_latency = SIM_GetLoadLat();
      this->store_latency = SIM_GetStoreLat();
      this->num_of_threads = SIM_GetThreadsNum();
      this->switch_cycles_overhead = SIM_GetSwitchCycles();
      this->configuration = configuration;
      this->thread_index_for_execute = 0;
      this->program_done = false;
      this->firt_execution = true;
      this->previous_executed_thread_index = -1;
      threads = new Thread[num_of_threads];
      initialize_threads_parameters();
      display_registers_table(0);
      display_registers_table(1);
      display_registers_table(2);

    }

    void execute_program(void)
    {
      unsigned thread_index_for_execute = 0;
      while(!program_done)
      {    
        get_thread_index_for_execution(&thread_index_for_execute);
        execute_instruction(thread_index_for_execute);
        update_threads_state();
        is_program_done(&program_done);
      }
    }

    void execute_instruction(unsigned execute_single_thread_instruction)
    {
      threads[thread_index_for_execute].execute_instruction();
    }

    void update_threads_state(void)
    {
      for (unsigned thread_index = 0; thread_index < num_of_threads ; ++thread_index)
      {
        if (BUSY == threads[thread_index].state)
        {
          threads[thread_index].remained_cycles_at_busy_state--;
          if (0 == threads[thread_index].remained_cycles_at_busy_state)
          {
            threads[thread_index].state = IDLE;
          }
        }
      }
    }

    void get_thread_index_for_execution(unsigned * thread_index_for_execute)
    {
      unsigned candidate_thread_index = 0;
      unsigned num_of_searched_threads = 0;
      bool found_thread_for_execution = false;

      if (firt_execution)
      {
        *thread_index_for_execute = 0;
      }
      else if (FINE == configuration)
      {
        candidate_thread_index = (previous_executed_thread_index + 1) % num_of_threads;

        for (num_of_searched_threads = 0; num_of_searched_threads < num_of_threads ; ++num_of_searched_threads)
        {
          if (IDLE != threads[candidate_thread_index].state)
          {
            candidate_thread_index = (previous_executed_thread_index + 1) % num_of_threads;
          }
          else
          {
            found_thread_for_execution = true;
            break;
          }
        }
        
        if (found_thread_for_execution)
        {
          *thread_index_for_execute = candidate_thread_index;
          previous_executed_thread_index = candidate_thread_index;
        }
        else
        {
          // TODO
        }
      }
     
    }

    void is_program_done(bool * program_done)
    {
      *program_done = true;
      for (unsigned thread_index = 0; thread_index < num_of_threads ; ++thread_index)
      {
        if (DONE != threads[thread_index].state)
        {
          *program_done = false;
        }
      }
    }

    void initialize_threads_parameters(void)
    {
      unsigned num_of_thread_instructions = 0;
      for (unsigned thread_index = 0; thread_index < num_of_threads ; ++thread_index)
      {
        get_num_of_instructions((unsigned)thread_index, &num_of_thread_instructions);
        threads[thread_index].update_num_of_instructions(num_of_thread_instructions);
        threads[thread_index].initialize_instructions_table();
        update_instruction_table((unsigned)thread_index, num_of_thread_instructions);
        display_thread_instructions_table(thread_index, num_of_thread_instructions);
        threads[thread_index].initialize_latencies(load_latency, store_latency);

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

     /* for DEBUG PURPOSE ONLY!!*/
    void display_registers_table(int thread_index)
    {
      printf("=================== REGISTER TABLE of TID %d ============\n\n", thread_index);
      for( unsigned register_index = 0; register_index < NUM_OF_REGISTERS; ++register_index)
      {
        printf("R%d=%d  ",register_index,  threads[thread_index].registers_table.reg[register_index]);
      }
      printf("\n\n");
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
