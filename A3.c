/*
 * File:	A3.c
 * Author:	Anran Zhang B00747547
 * Date:	2020-06-10
 * Modified:2020-06-17
 * Version:	0.9.5
 *
 * Purpose:	simulate different cpu scheduling algorithms.
 * NOTE:    did not fully implement RR, one issue took me 3 days to debug:(
 *          also with 50 or more jobs it may take quite a few seconds to run.
 *
 */


#include    <stdio.h>
#include    <stdlib.h>
#include    <stdbool.h>
#include    <string.h>
#include    <math.h>

#ifdef DEBUG
#define D_PRNT(...) fprintf(stderr, __VA_ARGS__)
#else
#define D_PRNT(...)
#endif
//define default values
#define     DEFAULT_INIT_JOBS        5
#define     DEFAULT_TOTAL_JOBS        100
#define     DEFAULT_LAMBDA        ((double)1.0)
#define     DEFAULT_SCHED_TIME        10            // usec
#define     DEFAULT_CONT_SWTCH_TIME 50            // usec
#define     DEFAULT_TICK_TIME        10            // msec
#define     DEFAULT_PROB_NEW_JOB    ((double)0.15)
#define     DEFAULT_RANDOMIZE        false
#define     MULTI                 2
//MULTI defines how many times the number of total job is allowed
enum sched_alg_T
{
    UNDEFINED, RR, SJF, FCFS
};
char *alg_names[] = {"UNDEFINED", "RR", "SJF", "FCFS"};

struct simulation_params
{
    enum sched_alg_T sched_alg;
    int init_jobs;
    int total_jobs;
    double lambda;
    int sched_time;
    int cont_swtch_time;
    int tick_time;
    double prob_new_job;
    bool randomize;
};

char *progname;

void usage(const char *message)
{
    fprintf(stderr, "%s", message);
    fprintf(stderr, "Usage: %s <arguments>\nwhere the arguments are:\n",
            progname);
    fprintf(stderr, "\t-alg [rr|sjf|fcfs]\n"
                    "\t[-init_jobs <n1 (int)>\n"
                    "\t[-total_jobs <n2 (int)>]\n"
                    "\t[-prob_comp_time <lambda (double)>]\n"
                    "\t[-sched_time <ts (int, microseconds)>]\n"
                    "\t[-cs_time <cs (int, microseconds)>]\n"
                    "\t[-tick_time <cs (int, milliseconds)>]\n"
                    "\t[-prob_new_job <pnj (double)>]\n"
                    "\t[-randomize]\n");
}

int process_args(int argc, char *argv[], struct simulation_params *sps)
{
    // Process the command-line arguments.
    // The only one which doesn't have a default (and thus must be
    // specified) is the choice of scheduling algorithm.

    char c;
    int i;

    for (i = 1; i < argc - 1; i++) {
        if (!strcmp(argv[i], "-alg")) {
            i++;
            if (!strcmp(argv[i], "rr"))
                sps->sched_alg = RR;
            else if (!strcmp(argv[i], "sjf"))
                sps->sched_alg = SJF;
            else if (!strcmp(argv[i], "fcfs"))
                sps->sched_alg = FCFS;
            else {
                usage("Error: invalid scheduling algorithm (-alg).\n");
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-init_jobs")) {
            i++;
            if (sscanf(argv[i], "%d%c", &sps->init_jobs, &c) != 1
                || sps->init_jobs < 0) {
                usage("Error: invalid argument to -init_jobs\n");
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-total_jobs")) {
            i++;
            if (sscanf(argv[i], "%d%c", &sps->total_jobs, &c) != 1
                || sps->total_jobs < 0) {
                usage("Error: invalid argument to -total_jobs\n");
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-prob_comp_time")) {
            i++;
            if (sscanf(argv[i], "%lf%c", &sps->lambda, &c) != 1
                || sps->lambda < 0) {
                usage("Error: invalid argument to -prob_comp_time\n");
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-sched_time")) {
            i++;
            if (sscanf(argv[i], "%d%c", &sps->sched_time, &c) != 1
                || sps->sched_time < 0) {
                usage("Error: invalid argument to -sched_time\n");
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-cs_time")) {
            i++;
            if (sscanf(argv[i], "%d%c", &sps->cont_swtch_time, &c) != 1
                || sps->cont_swtch_time < 0) {
                usage("Error: invalid argument to -cs_time\n");
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-tick_time")) {
            i++;
            if (sscanf(argv[i], "%d%c", &sps->tick_time, &c) != 1
                || sps->tick_time < 0) {
                usage("Error: invalid argument to -tick_time\n");
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-prob_new_job")) {
            i++;
            if (sscanf(argv[i], "%lf%c", &sps->prob_new_job, &c) != 1
                || sps->prob_new_job <= 0) {
                usage("Error: invalid argument to -prob_new_job\n");
                return 1;
            }
        }
        else if (!strcmp(argv[i], "-randomize"))
            sps->randomize = true;
        //check for invalid arguments
        else
        {
            fprintf(stderr,"Error: invalid argument %s",argv[i]);
            usage("\n");
            return 1;
        }
    }
    return 0;
}

//generates random compute time in secs
double rand_exp(double lambda)
{
    int64_t divisor = (int64_t)RAND_MAX + 1;
    double u_0_to_almost_1;
    double raw_value;
    u_0_to_almost_1 = (double)random() / divisor;
    raw_value = log(1 - u_0_to_almost_1) / -lambda;
    return round(raw_value * 1000000.) / 1000000.;
}
//define struct job
struct Job {
    //usec
    int64_t remaining;
    int64_t generated; // time when the job entered
    int64_t compute_time;
    int64_t passed_time;
    int state; //running = 0,waiting = 1,finished = 2
    int64_t wait_time;
    int64_t response_time;
    int64_t turnaround_time;
};
//function that generates a job and initialize it
struct Job getJob(double lambda, int64_t time)
{
    int64_t tmp = rand_exp(lambda)*1000000;
    struct Job j = {
            .generated = time,
            .compute_time = tmp,
            .remaining = tmp,
            .passed_time = 0,
            .state = 1,
            .wait_time = 0,
            .response_time = 0,
            .turnaround_time = 0
    };
    return j;
}
//returns the job index that has the shortest remaining time
int shortest(struct Job * job,int c)
{
    int x = 0;//index
    int temp = job[0].remaining;
    for (int i = 1; i < c; ++i)
    {
        if (job[i].remaining<temp&&job[i].state!=2)
        {
            temp = job[i].remaining;
            x = i;
        }
    }
    return x;
}
int main(int argc, char *argv[])
{
    progname = argv[0];
    struct simulation_params sim_params = {
            .sched_alg = UNDEFINED,
            .init_jobs = DEFAULT_INIT_JOBS,
            .total_jobs = DEFAULT_TOTAL_JOBS,
            .lambda = DEFAULT_LAMBDA,
            .sched_time = DEFAULT_SCHED_TIME,
            .cont_swtch_time = DEFAULT_CONT_SWTCH_TIME,
            .tick_time = DEFAULT_TICK_TIME,
            .prob_new_job = DEFAULT_PROB_NEW_JOB,
            .randomize = DEFAULT_RANDOMIZE
    };

    if (process_args(argc, argv, &sim_params) != 0)
        return EXIT_FAILURE;

    //set random flags
    if (sim_params.randomize == true)
        srandom(NULL);
    if (sim_params.sched_alg == UNDEFINED)
    {
        //quit if no scheduling algorithm is specified
        usage("No schedule algorithm is specified\n");
        return EXIT_FAILURE;
    }
    double average_response_time = 0;
    double average_waiting_time = 0;
    double average_turnaround_time = 0;

    int64_t clock_usec = -1;
    int64_t tick = 0;
    int64_t p_tick = -1; //previous tick
    //2 times the amount of total jobs just in case
    //The program break if I don't do that
    struct Job * jobs = malloc(MULTI*sim_params.total_jobs*8*sizeof(int64_t));
    int job_count = 0;
    int finished_jobs = 0;
    //initialize the jobs
    for (int i = 0; i < sim_params.init_jobs; ++i)
    {
        jobs[i] = getJob(sim_params.lambda, 0);
        D_PRNT("t=%d,job %d is added, needing %ld usec\n",0,i,jobs[i]
        .compute_time);

    }
    job_count = sim_params.init_jobs;
    int64_t scheduler_start_time = 0; //the time a scheduler starts
    int64_t cs_start_time = sim_params.sched_time; //time context switch starts
    bool scheduler_running = false;
    bool context_switch_running = false;
    bool init = true;//used for sjf
    bool job_scheduled = false;
    //to keep track of the jobs
    int previous_job_index = 0;
    int current_job_index = 0;
    while (finished_jobs<sim_params.total_jobs)
    {
        clock_usec++;//increments time in usec
        if (clock_usec%(sim_params.tick_time*1000)==0)
        {
            tick++;
            p_tick++;
            //clock tick and runs the scheduler
            //if the current job is running, it is stopped
            if (jobs[current_job_index].state==0)
            {
                D_PRNT("t=%ld,clock ticks,current running process %d stops\n",
                        clock_usec,current_job_index);
                jobs[current_job_index].state = 1;
            }
            if (context_switch_running&&cs_start_time<clock_usec)
                context_switch_running = false;
            scheduler_running = true;
            scheduler_start_time = clock_usec;
            //a new job generates and I put a hard limit here
            if((random()%(int)(100*sim_params.prob_new_job))==0
            &&job_count<sim_params.total_jobs*MULTI)
            {
                jobs[job_count] = getJob(sim_params.lambda, clock_usec);
                D_PRNT("t=%ld,job %d is added, needing %ld usec\n",clock_usec,
                       job_count, jobs[job_count].compute_time);
                job_count++;

            }

        }
        //scheduler is running
        if (scheduler_running)
        {
            if (clock_usec != scheduler_start_time + sim_params.sched_time)
                continue;
            else
            {
                scheduler_running = false;//scheduler finish
                //D_PRNT("t=%ld,scheduler done\n",clock_usec);
            }
        }
        //context switch is running
        if (context_switch_running &&clock_usec != cs_start_time + sim_params
        .cont_swtch_time)
        {
            continue;
        }
        if (clock_usec == cs_start_time + sim_params.cont_swtch_time) {
            context_switch_running = false;//cs finish
            D_PRNT("t=%ld,context switch done\n",clock_usec);
        }
        //if the current job is running
        if (jobs[current_job_index].state==0)
        {
            //increment time count
            jobs[current_job_index].passed_time++;
            jobs[current_job_index].remaining--;
            jobs[current_job_index].turnaround_time++;
            //if at current time the job finishes
            if (jobs[current_job_index].passed_time ==
            jobs[current_job_index].compute_time || jobs[current_job_index]
            .remaining ==0)
            {
                //current job finishes
                jobs[current_job_index].state=2;
                finished_jobs++;
                //adds to statistics in seconds
                average_response_time += (double)
                        jobs[current_job_index].response_time/sim_params
                        .total_jobs/1000000;
                average_turnaround_time += (double)
                        jobs[current_job_index].turnaround_time/sim_params
                        .total_jobs/1000000;
                average_waiting_time += (double)
                        jobs[current_job_index].wait_time/sim_params
                        .total_jobs/1000000;
                previous_job_index = current_job_index;
                D_PRNT("t=%ld,process %d finished\n", clock_usec,
                        current_job_index);
                D_PRNT("job %d respond=%ld,wait=%ld,turnaround=%ld\n",
                        current_job_index,jobs[current_job_index]
                        .response_time,jobs[current_job_index].wait_time,
                        jobs[current_job_index].turnaround_time);
            }
        }

        if (sim_params.sched_alg == FCFS)
        {
            //FCFS
            if (jobs[current_job_index].state == 1)
            {
                jobs[current_job_index].state = 0;
                D_PRNT("t=%ld,dispatching process %d,needing %ld usec\n",
                       scheduler_start_time, current_job_index,
                       jobs[current_job_index].compute_time);
            }
            if (jobs[current_job_index].state == 2 && job_count >
                                                      current_job_index + 1)
            {
                //previous job finished and there are jobs left
                current_job_index++;
                //runs scheduler at next usec
                scheduler_running = true;
                scheduler_start_time = clock_usec;
                //runs context switch after the scheduler finish
                context_switch_running = true;
                cs_start_time = scheduler_start_time + sim_params.sched_time;
                jobs[current_job_index] .response_time = scheduler_start_time -
                        jobs[current_job_index].generated;
                continue;

            }
        }
        if (sim_params.sched_alg == SJF)
        {
            if (jobs[current_job_index].state==1)
                job_scheduled = false;
            int tmp = current_job_index;
            //job finish
            if (jobs[current_job_index].state==2)
            {
                previous_job_index = current_job_index;
                //runs scheduler at next usec
                scheduler_running = true;
                scheduler_start_time = clock_usec;
                //runs context switch after the scheduler finish
                context_switch_running = true;
                cs_start_time = scheduler_start_time + sim_params.sched_time+1;
                current_job_index = shortest(jobs,job_count);
                jobs[current_job_index] .response_time = scheduler_start_time -
                        jobs[current_job_index].generated;
                continue;
            }
            if (!job_scheduled)
            {
                current_job_index = shortest(jobs,job_count);
                D_PRNT("t=%ld,dispatching process %d,needing %ld usec\n",
                       scheduler_start_time, current_job_index,
                       jobs[current_job_index].compute_time);
                job_scheduled = true;
            }
            jobs[current_job_index].state = 0;//start the job

        }
        if (sim_params.sched_alg == RR) {
            //clock ticks
            if (tick==p_tick)
            {
                previous_job_index = current_job_index;
                while (jobs[current_job_index].state==2)
                {
                    current_job_index++;
                    if (current_job_index == job_count - 1)
                        current_job_index = 0;
                }
                //respond time is not calculated correctly
                jobs[current_job_index] .response_time = scheduler_start_time -
                        jobs[current_job_index].generated;
            }

            if (jobs[current_job_index].state == 1)
            {
                jobs[current_job_index].state = 0;
                D_PRNT("t=%ld,dispatching process %d,needing %ld usec\n",
                       scheduler_start_time, current_job_index,
                       jobs[current_job_index].compute_time);
            }
            if (jobs[current_job_index].state == 2)
            {
                //runs scheduler
                scheduler_running = true;
                scheduler_start_time = clock_usec;
                //runs context switch after the scheduler finish
                context_switch_running = true;
                cs_start_time = scheduler_start_time + sim_params.sched_time;
                continue;

            }
        }
        //count the wait and turnaround time for the process in the queue
        //note if the scheduler or context switch is going on, it won't get here
        for (int i = 0; i < job_count; ++i) {
            if (jobs[i].state==1&&!context_switch_running&&!scheduler_running)
            {
                jobs[i].wait_time++;
                jobs[i].turnaround_time++;
            }
        }
    }


    //Print info using provided code
    printf("For a simulation using the %s scheduling algorithm\n",
           alg_names[sim_params.sched_alg]);
    printf("with the following parameters:\n");
    printf("    init jobs           = %d\n", sim_params.init_jobs);
    printf("    total jobs          = %d\n", sim_params.total_jobs);
    printf("    lambda              = %.6f\n", sim_params.lambda);
    printf("    sched time          = %d\n", sim_params.sched_time);
    printf("    context switch time = %d\n", sim_params.cont_swtch_time);
    printf("    tick time           = %d\n", sim_params.tick_time);
    printf("    prob of new job     = %.6f\n", sim_params.prob_new_job);
    printf("    randomize           = %s\n",
           sim_params.randomize ? "true" : "false");
    printf("the following results were obtained:\n");
    printf("    Average response time:   %10.6lf\n", average_response_time);
    printf("    Average turnaround time: %10.6lf\n",
           average_turnaround_time);
    printf("    Average waiting time:    %10.6lf\n", average_waiting_time);

    return EXIT_SUCCESS;
}
