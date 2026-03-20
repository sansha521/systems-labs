#include "oslabs.h"
#include <limits.h>
static const struct PCB null_pcb = {
    .process_id = 0,
    .arrival_timestamp = 0,
    .total_bursttime = 0,
    .execution_starttime = 0,
    .execution_endtime = 0,
    .remaining_bursttime = 0,
    .process_priority = 0
};
int test_null_pcb(struct PCB inpcb)
{
    if(inpcb.process_id == 0 &&
       inpcb.arrival_timestamp == 0 &&
       inpcb.total_bursttime == 0 &&
       inpcb.execution_starttime == 0 &&
       inpcb.execution_endtime == 0 &&
       inpcb.remaining_bursttime == 0 &&
       inpcb.process_priority == 0)
       return 1; // return True
    else
       return 0;
}
// lower number == higher priority
struct PCB handle_process_arrival_pp(struct PCB ready_queue[QUEUEMAX], int *queue_cnt, struct PCB current_process, struct PCB new_process, int timestamp)
{
    if(test_null_pcb(current_process)) 
    {
        new_process.execution_starttime = timestamp;
        new_process.execution_endtime = timestamp + new_process.total_bursttime;
        new_process.remaining_bursttime = new_process.total_bursttime;
        
        // nothing in the ready queue
        return new_process;
    
    }
    else if(new_process.process_priority >= current_process.process_priority) 
    {
        new_process.execution_starttime = 0;
        new_process.execution_endtime = 0;
        new_process.remaining_bursttime = new_process.total_bursttime;
        
        // add new process PCB to queue
        ready_queue[*queue_cnt] = new_process;
        *queue_cnt += 1;
        
        return current_process;
    }
    else 
    {
        current_process.execution_endtime = 0;
        current_process.remaining_bursttime -= timestamp - current_process.execution_starttime;

        ready_queue[*queue_cnt] = current_process;
        *queue_cnt += 1;

        new_process.execution_starttime = timestamp;
        new_process.execution_endtime = timestamp + new_process.total_bursttime;
        new_process.remaining_bursttime = new_process.total_bursttime;
        
        return new_process;
    }
    
}
// determine the process to execute next and returns its PCB
struct PCB handle_process_completion_pp(struct PCB ready_queue[QUEUEMAX], int *queue_cnt, int timestamp)
{
    // if the ready queue is empty, returns the NULLPCB
    if(*queue_cnt == 0)
    {
        return null_pcb;
    }
    else
    {
        // finds the PCB of the process in the ready queue with the highest priority
        int best_idx = -1;
        int best_priority = INT_MAX;

        for (int i = 0; i < *queue_cnt; ++i) {
            if (ready_queue[i].process_priority < best_priority) {
                best_priority = ready_queue[i].process_priority;
                best_idx = i;
            }
        }

        struct PCB best_pcb = (best_idx >= 0) ? ready_queue[best_idx] : null_pcb;

        if (best_idx >= 0) {
            // shiftremaining entries left to close the gap
            for (int i = best_idx; i < *queue_cnt - 1; ++i) {
                ready_queue[i] = ready_queue[i + 1];
            }
            //decrement queue count
            *queue_cnt -= 1;
    
            best_pcb.execution_starttime = timestamp;
            best_pcb.execution_endtime = timestamp + best_pcb.remaining_bursttime;
        }
    
        return best_pcb;
    }
}
struct PCB handle_process_arrival_srtp(struct PCB ready_queue[QUEUEMAX], int *queue_cnt, struct PCB current_process, struct PCB new_process, int time_stamp)
{
    if(test_null_pcb(current_process))
    {
        new_process.execution_starttime = time_stamp;
        new_process.execution_endtime = time_stamp + new_process.total_bursttime;
        new_process.remaining_bursttime = new_process.total_bursttime;
        
        return new_process;   
    }
    else {

        if (new_process.total_bursttime < current_process.remaining_bursttime) {
            // current process goes to the queue; zero its timing info
            current_process.remaining_bursttime -= (time_stamp - current_process.execution_starttime);
            current_process.execution_starttime = 0;
            current_process.execution_endtime = 0;
            
            ready_queue[*queue_cnt] = current_process;
            *queue_cnt += 1;

            // new process takes over immediately
            new_process.execution_starttime = time_stamp;
            new_process.execution_endtime = time_stamp + new_process.total_bursttime;
            new_process.remaining_bursttime = new_process.total_bursttime;

            return new_process;

        } else {
            // newcomer waits; clear its timing info before enqueuing
            new_process.execution_starttime = 0;
            new_process.execution_endtime = 0;
            new_process.remaining_bursttime = new_process.total_bursttime;

            ready_queue[*queue_cnt] = new_process;
            *queue_cnt += 1;

            return current_process;
        }
    }
}
struct PCB handle_process_completion_srtp(struct PCB ready_queue[QUEUEMAX], int *queue_cnt, int timestamp)
{
    // If the ready queue is empty, return NULLPCB
    if (*queue_cnt == 0)
    {
        return null_pcb;
    }
    else
    {
        int best_idx = -1;
        int smallest_rbt = INT_MAX;

        // Find the process with the smallest remaining burst time
        for (int i = 0; i < *queue_cnt; i++)
        {
            if (ready_queue[i].remaining_bursttime < smallest_rbt)
            {
                smallest_rbt = ready_queue[i].remaining_bursttime;
                best_idx = i;
            }
        }

        struct PCB next_process = ready_queue[best_idx];

        // Shift queue to remove the selected process
        for (int i = best_idx; i < *queue_cnt - 1; i++)
        {
            ready_queue[i] = ready_queue[i + 1];
        }
        *queue_cnt -= 1;

        // Set its new execution times
        next_process.execution_starttime = timestamp;
        next_process.execution_endtime = timestamp + next_process.remaining_bursttime;

        return next_process;
    }
}
struct PCB handle_process_arrival_rr(struct PCB ready_queue[QUEUEMAX], int *queue_cnt, struct PCB current_process, struct PCB new_process, int timestamp, int time_quantum)
{
    if (test_null_pcb(current_process))
    {
        // No process currently running so start new one immediately
        new_process.execution_starttime = timestamp;
        // end time is min(time_quantum, total_bursttime)
        int burst = (new_process.total_bursttime < time_quantum) ? new_process.total_bursttime : time_quantum;
        new_process.execution_endtime = timestamp + burst;
        new_process.remaining_bursttime = new_process.total_bursttime;

        return new_process;
    }
    else
    {
        // Add new process to the ready queue
        new_process.execution_starttime = 0;
        new_process.execution_endtime = 0;
        new_process.remaining_bursttime = new_process.total_bursttime;

        ready_queue[*queue_cnt] = new_process;
        *queue_cnt += 1;

        // Current process keeps running
        return current_process;
    }
}
struct PCB handle_process_completion_rr(struct PCB ready_queue[QUEUEMAX], int *queue_cnt, int timestamp, int time_quantum)
{
    // If the ready queue is empty, return NULLPCB
    if (*queue_cnt == 0)
    {
        return null_pcb;
    }
    else
    {
        int best_idx = -1;
        int earliest_arrival = INT_MAX;

        // Find the process with the earliest arrival time
        for (int i = 0; i < *queue_cnt; i++)
        {
            if (ready_queue[i].arrival_timestamp < earliest_arrival)
            {
                earliest_arrival = ready_queue[i].arrival_timestamp;
                best_idx = i;
            }
        }

        struct PCB next_process = ready_queue[best_idx];

        // Remove chosen process from the queue
        for (int i = best_idx; i < *queue_cnt - 1; i++)
        {
            ready_queue[i] = ready_queue[i + 1];
        }
        *queue_cnt -= 1;

        // Update execution timing
        next_process.execution_starttime = timestamp;
        int run_time = (next_process.remaining_bursttime < time_quantum)
                           ? next_process.remaining_bursttime
                           : time_quantum;
        next_process.execution_endtime = timestamp + run_time;

        return next_process;
    }
}
