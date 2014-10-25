/*
 * student.c
 *
 * Multithreaded OS Simulation: Implementing Processor Scheduler using pthreads
 * 
 * Yuen Hsi Chang
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "os-sim.h"
#include "student.h"

// Local helper functions 
static void addReadyProcess(pcb_t* proc); 
static pcb_t* getReadyProcess(void); 
static void schedule(unsigned int cpu_id);

int schedulerType; // 0 is FCFS, 1 is Round Robin, 2 is Static Priority
int timeSlice; // Keeps track of the timeslice
int cpu_count; // Keeps track of the number of CPUs (required to check for empty CPUs in problem 3)

/*
 * main() simply parses command line arguments, then calls start_simulator().
 * Supports the -r and -p modifiers, but does not error check and assumes the user knows the syntax if 
 * they enter an input of length 2, 3 or 4 
 */
int main(int argc, char *argv[])
{
  // Parse command-line arguments and set cpu_count
  if (argc == 2) {
    schedulerType = 0;
  }
  else if (argc == 4) {
    if (strcmp(argv[2], "-r") == 0)
    {
      schedulerType = 1;
      timeSlice = atoi(argv[3]);
    }
    else {
      fprintf(stderr, "3Multithreaded OS Simulator\n"
      "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
      "    Default : FCFS Scheduler\n"
      "         -r : Round-Robin Scheduler\n"
      "         -p : Static Priority Scheduler\n\n");
      return -1;      
    }
  }
  else if (argc == 3) {
    if (strcmp(argv[2], "-p") == 0) {
      schedulerType = 2;
    }
    else {
      fprintf(stderr, "3Multithreaded OS Simulator\n"
      "Usage: ./os-sim <# CPUs> [ -r <time slice> | -p ]\n"
      "    Default : FCFS Scheduler\n"
      "         -r : Round-Robin Scheduler\n"
      "         -p : Static Priority Scheduler\n\n");
      return -1;      
    }
  }
  cpu_count = atoi(argv[1]);

  // Allocate the current[] array 
  current = malloc(sizeof(pcb_t*) * cpu_count);
  assert(current != NULL);

  // Initialize necessary mutexes
  pthread_mutex_init(&current_mutex, NULL);
  pthread_mutex_init(&ready_mutex, NULL);
  pthread_cond_init(&ready_empty, NULL);

  // Start the simulator 
  printf("starting simulator\n");
  fflush(stdout);
  start_simulator(cpu_count);

  return 0;
}

/*
 * idle() is called by the simulator when the idle process is scheduled.
 * It blocks until a process is added to the ready queue, and then calls
 * schedule() to select the next process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
  pthread_mutex_lock(&ready_mutex);
  while (head == NULL) {
    pthread_cond_wait(&ready_empty, &ready_mutex);
  }
  pthread_mutex_unlock(&ready_mutex);
  schedule(cpu_id);
}

/*
 * schedule() is the CPU scheduler.  It performs the following tasks in order:
 *
 *   1. Selects and removes a runnable process from the ready queue. 
 *
 *   2. Sets the process state to RUNNING and updates the current array with this process
 *
 *   3. Calls context_switch() and tells the simulator which process to execute
 *      next on the CPU.  If no process is runnable, calls context_switch()
 *      with a pointer to NULL to select the idle process.
 */
static void schedule(unsigned int cpu_id) {
  int i = -1;
  // If the user selects the Round Robin Scheduler
  if (schedulerType == 1) {
    i = timeSlice;
  }
  
  pcb_t *newProcess = getReadyProcess();

  // If there is a process in the Ready Queue, run the "idle" process
  if (newProcess == NULL){
    context_switch(cpu_id, newProcess, -1);
  }
  else {
    pthread_mutex_lock(&current_mutex);

    newProcess->state = PROCESS_RUNNING;
    current[cpu_id] = newProcess;

    pthread_mutex_unlock(&current_mutex);
    context_switch(cpu_id, newProcess, i);
  }
}

/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * It places the currently running process back in the ready queue, then calls 
 * schedule() and selects a new runnable process.
 */
extern void preempt(unsigned int cpu_id) {
  pthread_mutex_lock(&current_mutex);
  
  pcb_t* currentProcess = current[cpu_id];
  currentProcess->state = PROCESS_READY;
  addReadyProcess(currentProcess);

  pthread_mutex_unlock(&current_mutex);
  schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It marks the process as WAITING, then calls schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id) {
  pthread_mutex_lock(&current_mutex);

  pcb_t* currentProcess = current[cpu_id];
  current[cpu_id] = NULL;
  currentProcess->state = PROCESS_WAITING;

  pthread_mutex_unlock(&current_mutex);
  schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 *
 * It marks the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id) { 
  pthread_mutex_lock(&current_mutex); 

  pcb_t* currentProcess = current[cpu_id];
  current[cpu_id] = NULL;
  currentProcess->state = PROCESS_TERMINATED;

  pthread_mutex_unlock(&current_mutex);
  schedule(cpu_id);
}


/*
 * wake_up() is the handler called by the simulator for a new process and when a 
 * process's I/O request completes.  It performs the following tasks depending on
 * the scheduler type:
 *
 * FCFS and RR: Mark the process as READY, and insert it into the ready queue
 *
 * SP:  1. Mark the process as READY, and insert it into the ready queue
        2. Check whether any of the CPUs are currently idle, and if so, run the process
            on the idle CPU
        3. If none of the CPUs are idle, find the CPU running the lowest priority process,
            and check whether the priority number of this process is lower than the 
            process just woken up. If so, call force_preempt on this CPU. 
 */
extern void wake_up(pcb_t *process) {

  if (schedulerType != 2) {    
    process->state = PROCESS_READY;
    addReadyProcess(process);
  }
  else {
    int i = 0;
    int priorityNumber = process->static_priority;
    process->state = PROCESS_READY;
    addReadyProcess(process);
    // Check whether any of the CPUs are currently idle
    while(i < cpu_count) {
      if (current[i] == NULL) {
        return;
      }
      i++;
    }
    // Find the CPU running the process with the lowest priority number
    int lowestCPUPrio = 9999;
    int lowestPrioCPU = -1;
    for (i = 0; i < cpu_count; i++) {
      if (current[i]->static_priority < lowestCPUPrio) {
        lowestCPUPrio = current[i]->static_priority;
        lowestPrioCPU = i;
      }
    }
    // Checks whether this CPU's process's priority number is lower than the priority of 
    //the proces just woken up, and call force_preempt on the CPU if so. 
    if (current[lowestPrioCPU]->static_priority < priorityNumber) {
      force_preempt(lowestPrioCPU);
    }
  }
}


/* The following 2 functions implement a FIFO ready queue of processes */

/* 
 * addReadyProcess takes no arguments and returns the first process in the ready 
 * queue, or NULL if the ready queue is empty. It performs the following tasks depending on
 * the scheduler type:  
 * 
 * FCFS and RR: add a process to the end of a pseudo linked list. 
 * 
 * SP: Traverse the list, and insert the new process such that it is behind all processes that
 *      share the same priority number as itself. 
 */
static void addReadyProcess(pcb_t* proc) {
  // ensure no other process can access ready list while we update it
  pthread_mutex_lock(&ready_mutex);

  // for FCFS and RR schedulers
  if (schedulerType != 2) {
    // add this process to the end of the ready list
    if (head == NULL) {
      head = proc;
      tail = proc;
      proc->next = NULL;
      // if list was empty may need to wake up idle process
      pthread_cond_signal(&ready_empty);
    }
    else {
      tail->next = proc;
      tail = proc;
    }
    // ensure that this proc points to NULL
    proc->next = NULL;
  }
  // for the SP scheduler
  else {
    // add this process to the ready list if the list is empty
    if (head == NULL) {
      head = proc;
      tail = proc;
      proc->next = NULL;
      // if list was empty may need to wake up idle process
      pthread_cond_signal(&ready_empty);
    }
    else {
      pcb_t* currentProc = head;

      /*
       * if this process has a priority number greater than all processes,  
       * replace the head with this process
       */
      if (proc->static_priority > currentProc->static_priority) {
        proc->next = currentProc;
        head = proc;
      }
      /*
       * traverse the list and place this process behind any process that has
       * a priority number that is the same or greater than this process. 
       */ 
      else {
        while (currentProc->next != NULL) {
          if (proc->static_priority > currentProc->next->static_priority) {
            proc->next = currentProc->next;
            currentProc->next = proc;
            pthread_mutex_unlock(&ready_mutex);
            return;
          }
          currentProc = currentProc->next;
        }
        /*
         * if this process has a priority number lesser than all processes, 
         * replace the tail with this process. 
         */
        currentProc->next = proc;
        proc->next = NULL;
        tail = proc;
      }
    }
  }
  pthread_mutex_unlock(&ready_mutex);
}

/* 
 * getReadyProcess removes a process from the front of a pseudo linked list. 
 * it takes no arguments and returns the first process in the ready queue, or NULL 
 * if the ready queue is empty. 
 */
static pcb_t* getReadyProcess(void) {
  // ensure no other process can access ready list while we update it
  pthread_mutex_lock(&ready_mutex);

  // if list is empty, unlock and return null
  if (head == NULL) {
	  pthread_mutex_unlock(&ready_mutex);
	  return NULL;
  }

  // get first process to return and update head to point to next process
  pcb_t* first = head;
  head = first->next;

  // if there was no next process, list is now empty, set tail to NULL
  if (head == NULL) {
    tail = NULL;
  }

  pthread_mutex_unlock(&ready_mutex);
  return first;
}

