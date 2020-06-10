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
                || sps->prob_new_job < 0) {
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
    //quit if no scheduling algorithm is specified
    if (sim_params.sched_alg == UNDEFINED)
    {
        usage("No schedule algorithm is specified\n");
        return EXIT_FAILURE;
    }
    //set random flags
    if (sim_params.randomize == true)
        srandom(NULL);
    //TODO: RUN THE SIMULATION HERE
    struct Job {
        int64_t start;
        int64_t compute_time;
        int64_t passed_time;
        int state; //running = 0,waiting = 1
    };
    int64_t usec = 0;
    struct Job jobs [sim_params.total_jobs];

    //TODO: Calculate statistics
    double average_response_time = 0.42;
    double average_waiting_time = 0.53;
    double average_turnaround_time = 98.1;

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
