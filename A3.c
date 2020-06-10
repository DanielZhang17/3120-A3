/*
 * File:	A3.c
 * Author:	Anran Zhang B00747547
 * Date:	2020-06-10
 * Version:	0.9
 *
 * Purpose:	simulate different cpu scheduling algorithms.
 */


#include    <stdio.h>
#include    <stdlib.h>
#include    <stdbool.h>
#include    <string.h>

#ifdef DEBUG
#define D_PRNT(...) fprintf(stderr, __VA_ARGS__)
#else
#define D_PRNT(...)
#endif


#define     DEFAULT_INIT_JOBS        5
#define     DEFAULT_TOTAL_JOBS        100
#define     DEFAULT_LAMBDA        ((double)1.0)
#define     DEFAULT_SCHED_TIME        10            // usec
#define     DEFAULT_CONT_SWTCH_TIME 50            // usec
#define     DEFAULT_TICK_TIME        10            // msec
#define     DEFAULT_PROB_NEW_JOB    ((double)0.15)
#define     DEFAULT_RANDOMIZE        false

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


/*
 * Name:	process_args()
 * Purpose:	Process the arguments, checking for validity where possible.
 * Arguments:	argc, argv and a struct to hold the values in.
 * Outputs:	Error messages only.
 * Modifies:	The struct argument.
 * Returns:	0 on success, non-0 otherwise.
 * Assumptions:	The pointers are valid.
 * Bugs:	This is way too long.  Maybe I should reconsider how
 *		much I dislike getopt().
 * Notes:	Sets the struct to the default values before
 *		processing the args.
 */

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
        } else if (!strcmp(argv[i], "-init_jobs")) {
            i++;
            if (sscanf(argv[i], "%d%c", &sps->init_jobs, &c) != 1
                || sps->init_jobs < 0) {
                usage("Error: invalid argument to -init_jobs\n");
                return 1;
            }
        }

/// MORE CODE HERE!!

    }
    return 0;
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

    double average_response_time = 0.42;
    double average_waiting_time = 0.53;
    double average_turnaround_time = 98.1;

    if (process_args(argc, argv, &sim_params) != 0)
        return EXIT_FAILURE;

    // RUN THE SIMULATION HERE

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
