/* External definitions for job-shop model. */

#include "simlib.h"		/* Required for use of simlib.c. */

#define EVENT_ARRIVAL         1	/* Event type for arrival of a job to the
				   system. */
#define EVENT_DEPARTURE       2	/* Event type for departure of a job from a
				   particular station. */
#define EVENT_END_SIMULATION  3	/* Event type for end of the simulation. */
#define STREAM_INTERARRIVAL   1	/* Random-number stream for interarrivals. */
#define STREAM_JOB_TYPE       2	/* Random-number stream for job types. */
#define STREAM_SERVICE        3	/* Random-number stream for service times. */
#define MAX_NUM_STATIONS      5	/* Maximum number of stations. */
#define MAX_NUM_JOB_TYPES     3	/* Maximum number of job types. */

/* Declare non-simlib global variables. */

int num_stations, num_job_types, i, j, num_machines[MAX_NUM_STATIONS + 1],
  num_tasks[MAX_NUM_JOB_TYPES + 1],
  route[MAX_NUM_JOB_TYPES + 1][MAX_NUM_STATIONS + 1], num_machines_busy[MAX_NUM_STATIONS + 1], job_type, task;
double mean_interarrival, length_simulation, prob_distrib_job_type[26],
  mean_service[MAX_NUM_JOB_TYPES + 1][MAX_NUM_STATIONS + 1];
FILE *infile, *outfile;

/* Main function. */
int main()
{
    /* Open input and output files */

    infile = fopen("jobshop.in","r");
    outfile = fopen("jobshop.out","w");

    /* Read input parameters */

    fscanf(infile, "%d %d %lf %lf", &num_stations, &num_job_types, &mean_interarrival, &length_simulation);


    fprintf(outfile, "%d %d %lf %lf", num_stations, num_job_types, mean_interarrival, length_simulation);

    return 0;
}