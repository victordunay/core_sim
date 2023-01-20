/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <stdio.h>
#include <unistd.h>

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
        case CMD_SUBI:
          registers_table.reg[current_instruction.dst_index] = registers_table.reg[current_instruction.src1_index] - current_instruction.src2_index_imm;
          break;
        case CMD_SUB:
          registers_table.reg[current_instruction.dst_index] = registers_table.reg[current_instruction.src1_index] - registers_table.reg[current_instruction.src2_index_imm];
          break;
        case CMD_ADDI:
          registers_table.reg[current_instruction.dst_index] = registers_table.reg[current_instruction.src1_index] + current_instruction.src2_index_imm;
          break;
        case CMD_ADD:
          registers_table.reg[current_instruction.dst_index] = registers_table.reg[current_instruction.src1_index] + registers_table.reg[current_instruction.src2_index_imm];
          break;
        case CMD_LOAD:
    
          memory_address = current_instruction.src1_index + current_instruction.src2_index_imm;
          SIM_MemDataRead(memory_address, &registers_table.reg[current_instruction.dst_index]);
          state = BUSY;
          remained_cycles_at_busy_state += load_latency + 1;
          break;
        case CMD_STORE:
          memory_address = 4 * registers_table.reg[current_instruction.dst_index] + current_instruction.src2_index_imm;

          SIM_MemDataWrite(memory_address, registers_table.reg[current_instruction.src1_index]);

          state = BUSY;
          remained_cycles_at_busy_state += store_latency + 1;
          break;
        case CMD_HALT:
          state = DONE;
          break;
      }
      num_of_executed_instructions++;
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
    bool first_execution;
    unsigned total_num_of_cycles;
    unsigned total_num_of_instructions;
    // TODO ADD OVERHEAD ADDITION FROM IMAGE FILE AND NOT 1 
    ThreadsTable(configuration_t configuration)
    {
      this->load_latency = SIM_GetLoadLat();
      this->store_latency = SIM_GetStoreLat();
      this->num_of_threads = SIM_GetThreadsNum();
      this->switch_cycles_overhead = SIM_GetSwitchCycles();
      this->configuration = configuration;
      this->thread_index_for_execute = 0;
      this->program_done = false;
      this->first_execution = true;
      this->total_num_of_cycles = 0;
      this->previous_executed_thread_index = -1;
      threads = new Thread[num_of_threads];
      initialize_threads_parameters();
    }

    void execute_program(void)
    {
      bool found_thread_for_execution = false;
      unsigned thread_index_for_execute = 0;
      bool core_in_idle_state = false;
      bool switch_context_is_required = false;

      while(!program_done)
      {  
        if (!switch_context_is_required)
        {
          get_thread_index_for_execution(&thread_index_for_execute, &found_thread_for_execution);
          
          if ((previous_executed_thread_index != thread_index_for_execute) & (!core_in_idle_state) & (configuration == BLOCK))
          {
            switch_context_is_required = true;
          }
        }
        else
        {
          switch_context_is_required = false;
        }

        if (found_thread_for_execution & !switch_context_is_required)
        {
          execute_instruction(thread_index_for_execute);
          previous_executed_thread_index = thread_index_for_execute;
        }

        core_in_idle_state =  is_core_idle();
        is_program_done(&program_done);
        first_execution = false;
        update_threads_state();
        if (switch_context_is_required)
        {
          total_num_of_cycles += switch_cycles_overhead;
        }
        else
        {
          total_num_of_cycles++;
        }
      }
    }

    bool is_core_idle(void)
    {
      bool core_is_idle = true;

      for (unsigned thread_index = 0; thread_index < num_of_threads ; ++thread_index)
      {

        if (IDLE == threads[thread_index].state)
        {
          core_is_idle = false;
          break;
        }
      }
      return core_is_idle;
    }
    void execute_instruction(unsigned thread_index_for_execute)
    {
      threads[thread_index_for_execute].execute_instruction();
    }

    void get_total_num_of_instructions( unsigned * get_total_num_of_instructions)
    {
      *get_total_num_of_instructions = 0;

      for (unsigned thread_index = 0; thread_index < num_of_threads ; ++thread_index)
      {
        *get_total_num_of_instructions += threads[thread_index].num_of_executed_instructions;
      }
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

    void get_thread_index_for_execution(unsigned * thread_index_for_execute, bool * found_thread_for_execution)
    {
      unsigned candidate_thread_index = 0;
      unsigned num_of_searched_threads = 0;
      *found_thread_for_execution = false;

  
      if (FINE == configuration)
      {
        if (first_execution)
          {
            *thread_index_for_execute = 0;
            *found_thread_for_execution = true;
            
          }
        else
        {
          candidate_thread_index = (previous_executed_thread_index + 1) % num_of_threads;

          for (num_of_searched_threads = 0; num_of_searched_threads < num_of_threads ; ++num_of_searched_threads)
          {
            if (IDLE != threads[candidate_thread_index].state)
            {
              candidate_thread_index = (candidate_thread_index + 1) % num_of_threads;
            }
            else
            {
              *found_thread_for_execution = true;
              break;
            }
          }
          
          if (*found_thread_for_execution)
          {
            *thread_index_for_execute = candidate_thread_index;
          }
   
        }
      }
      else if (BLOCK == configuration)
      {
        if (first_execution)
        {
          *thread_index_for_execute = 0;
          *found_thread_for_execution = true;
        }
        else
        {
          candidate_thread_index = previous_executed_thread_index;

          if (IDLE != threads[candidate_thread_index].state)
          {
            for (num_of_searched_threads = 0; num_of_searched_threads < num_of_threads ; ++num_of_searched_threads)
            {
              if (IDLE != threads[candidate_thread_index].state)
              {
                candidate_thread_index = (candidate_thread_index + 1) % num_of_threads;
              }
              else
              {
                *found_thread_for_execution = true;
                break;
              }
            }
          }
          else
          {
            *found_thread_for_execution = true;
          }
        }

        if (*found_thread_for_execution)
        {
          *thread_index_for_execute = candidate_thread_index;
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


ThreadsTable * threads_table_fine = NULL;
ThreadsTable * threads_table_block = NULL;

double get_cpi(ThreadsTable * threads_table)
{
  unsigned total_num_of_instructions = 0;
  double cycles_per_instruction = 0;

  threads_table->get_total_num_of_instructions(&total_num_of_instructions);

  if (0 != total_num_of_instructions)
  {
    cycles_per_instruction = (double)threads_table->total_num_of_cycles / total_num_of_instructions;
  }

	return cycles_per_instruction;
}

void assign_context(tcontext* context, int threadid, ThreadsTable * threads_table)
{
  unsigned register_index = 0;

  for(register_index = 0; register_index < NUM_OF_REGISTERS; ++register_index)
  {
    context[threadid].reg[register_index] = threads_table->threads[threadid].registers_table.reg[register_index];
  }
}


void CORE_BlockedMT()
{
  threads_table_block = new ThreadsTable(BLOCK);
  threads_table_block->execute_program();
}

void CORE_FinegrainedMT() 
{
  threads_table_fine = new ThreadsTable(FINE);
  threads_table_fine->execute_program();
}

double CORE_BlockedMT_CPI()
{
  return get_cpi(threads_table_block);
}

double CORE_FinegrainedMT_CPI()
{
  return get_cpi(threads_table_fine);
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) 
{
  assign_context(context, threadid, threads_table_block);
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) 
{
  assign_context(context, threadid, threads_table_fine);
}
