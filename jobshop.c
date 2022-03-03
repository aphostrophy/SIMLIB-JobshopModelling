/* External definitions for job-shop model. */

#include "simlib.h"		/* Required for use of simlib.c. */

#define EVENT_ARRIVAL         1	/* Event type for arrival of a job to the
				   system. */
#define EVENT_DEPARTURE       2	/* Event type for departure of a job from a
				   particular station. */

#define EVENT_DEPARTURE_FIRST_JOBSHOP       3 /* Event type for departure of a job
from jobshop 1*/
#define EVENT_DEPARTURE_SECOND_JOBSHOP       4 /* Event type for departure of a job
from jobshop 2*/
#define EVENT_ARRIVAL_FINAL_JOBSHOP         5 /* Event type for arrival of a job
at jobshop 3*/
#define EVENT_END_SIMULATION  6	/* Event type for end of the simulation. */

#define STREAM_INTERARRIVAL   1	/* Random-number stream for interarrivals. */
#define STREAM_JOB_TYPE       2	/* Random-number stream for job types. */
#define STREAM_SERVICE        3	/* Random-number stream for service times. */
#define MAX_NUM_JOBSHOPS      3 /* Maximum number of jobshops */
#define MAX_NUM_STATIONS      5	/* Maximum number of stations. */
#define MAX_NUM_JOB_TYPES     3 	/* Maximum number of job types. */

#define FIRST_JOBSHOP         1
#define SECOND_JOBSHOP        2
#define THIRD_JOBSHOP         3

#define UNIFORM_STREAM        8

/* Declare non-simlib global variables. */

int num_stations, num_job_types, i, j, num_machines[MAX_NUM_STATIONS + 1],
  num_tasks[MAX_NUM_JOB_TYPES + 1],
  route[MAX_NUM_JOB_TYPES + 1][MAX_NUM_STATIONS + 1], 
  num_machines_busy[MAX_NUM_JOBSHOPS + 1][MAX_NUM_STATIONS + 1],
  job_type, task;
double mean_interarrival, length_simulation, prob_distrib_job_type[26],
  mean_service[MAX_NUM_JOB_TYPES + 1][MAX_NUM_STATIONS + 1];
FILE *infile, *outfile, *logfile;

int count1 = 0;
int count2 = 0;

void arrive_jobshop(int is_new_job, int jobshop_number);
void arrive_final_jobshop(int is_new_job, int jobshop_number);

void arrive()
{
  int jobshop_number;
  double random_value;

  event_schedule(sim_time + expon(mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL);
  job_type = random_integer(prob_distrib_job_type, STREAM_JOB_TYPE);

  random_value = uniform(0, 1, UNIFORM_STREAM);
  if(random_value < 0.5){
    jobshop_number = 1;
    count1++;
  } else{
    jobshop_number = 2;
    count2++;
  }
  fprintf(logfile, "simtime[%.4f] new job[%d] ARRIVING for jobshop[%d]\n", sim_time, job_type,jobshop_number);
  arrive_jobshop(1, jobshop_number);
}

void arrive_jobshop(int is_new_job, int jobshop_number)
{
  int station;

  if(is_new_job){
    task = 1;
  }

  station = route[job_type][task];

  fprintf(logfile, "simtime[%.4f] job[%d] ARRIVING for station[%d] in jobshop[%d]\n", sim_time, job_type, station, jobshop_number);

  if(num_machines_busy[jobshop_number][station] == num_machines[station]){
    /* All machines in this station are busy, so place the arriving job at
        the end of the appropriate queue. Note that the following data are
        stored in the record for each job:
        1. Time of arrival to this station.
        2. Job type.
        3. Current task number. */

    transfer[1] = sim_time;
    transfer[2] = job_type;
    transfer[3] = task;
    fprintf(logfile,"simtime[%.4f] job[%d] is QUEUED at station[%d] in jobshop[%d]\n", sim_time, job_type, station, jobshop_number);
    list_file(LAST, (jobshop_number-1)*num_stations + station);
  } else {
    /* A machine in this station is idle, so start service on the arriving
        job (which has a delay of zero). */

    sampst(0.0, station + num_stations*(jobshop_number-1)); /* For station */
    sampst(0.0, num_stations*MAX_NUM_JOBSHOPS + job_type); /* For job type. */
    ++num_machines_busy[jobshop_number][station];
    timest((double) num_machines_busy[jobshop_number][station], station + num_stations*(jobshop_number-1));

    /*
      Schedule a service completion. Note defining attributes beyond the
      first two for the event record before invoking event_schedule.
    */

    transfer[3] = job_type;
    transfer[4] = task;

    switch(jobshop_number){
        case FIRST_JOBSHOP:
          event_schedule(sim_time + erlang(2, mean_service[job_type][task], STREAM_SERVICE), EVENT_DEPARTURE_FIRST_JOBSHOP);
          break;
        case SECOND_JOBSHOP:
          event_schedule(sim_time + erlang(2, mean_service[job_type][task], STREAM_SERVICE), EVENT_DEPARTURE_SECOND_JOBSHOP);
          break;
        default:
          fprintf(stderr,"INCORRECT JOBSHOP NUMBER!\n");
          exit(0);
    }
  }
}

void depart_jobshop(int jobshop_number)
{
    int station, job_type_queue, task_queue;

    /* Determine the station from which the job is departing. */

    job_type = transfer[3];
    task = transfer[4];
    station = route[job_type][task];

    fprintf(logfile, "simtime[%.4f] job[%d] DEPARTED from station[%d] in jobshop[%d]\n", sim_time, job_type, station, jobshop_number);

    if(list_size[station + num_stations*(jobshop_number-1)]==0)
    {
      /* The queue for this station is empty, so make a machine in this
        station idle. */

      --num_machines_busy[jobshop_number][station];
      timest((double) num_machines_busy[jobshop_number][station], station + num_stations*(jobshop_number-1));

      fprintf(logfile, "simtime[%.4f] station[%d] in jobshop[%d] is IDLING\n", sim_time, station, jobshop_number);
    }

    else {

      fprintf(logfile,"simtime[%.4f] job[%d] is UNQUEUED at station[%d] in jobshop[%d]\n", sim_time, job_type, station, jobshop_number);
      /* The queue is nonempty, so start service on first job in queue. */

      list_remove(FIRST,(jobshop_number-1)*num_stations + station);

      /* Tally this delay for this station. */

      sampst(sim_time - transfer[1], station + num_stations*(jobshop_number-1));

      /* Tally this same delay for this job type. */

      job_type_queue = transfer[2];
      task_queue = transfer[3];
      sampst(sim_time - transfer[1], num_stations*MAX_NUM_JOBSHOPS + job_type_queue);

      /* Schedule end of service for this job at this station.  Note defining
      attributes beyond the first two for the event record before invoking
      event_schedule. */

      transfer[3] = job_type_queue;
      transfer[4] = task_queue;

      switch(jobshop_number){
        case FIRST_JOBSHOP:
          event_schedule (sim_time + erlang (2, mean_service[job_type_queue][task_queue], STREAM_SERVICE), EVENT_DEPARTURE_FIRST_JOBSHOP);
          break;
        case SECOND_JOBSHOP:
          event_schedule (sim_time + erlang (2, mean_service[job_type_queue][task_queue], STREAM_SERVICE), EVENT_DEPARTURE_SECOND_JOBSHOP);
          break;
        default:
          fprintf(stderr, "INVALID JOBSHOP NUMBER");
          exit(0);
      }
    }

    if(task < num_tasks[job_type]){
      ++task;
      arrive_jobshop(0, jobshop_number);
    } else{
      arrive_final_jobshop(1, 3);
    }
}

void depart_final(void)
{
  int station, job_type_queue, task_queue;

  /* Determine the station from which the job is departing. */

  job_type = transfer[3];
  task = transfer[4];
  station = route[job_type][task];

  fprintf(logfile, "simtime[%.4f] job[%d] DEPARTED from station[%d] in jobshop[3]\n",sim_time, job_type, station);

  /* Check to see whether the queue for this station is empty. */  

  if (list_size[station + num_stations*(THIRD_JOBSHOP-1)] == 0)
  {
    /* The queue for this station is empty, so make a machine in this
        station idle. */

    --num_machines_busy[THIRD_JOBSHOP][station];
    timest ((double) num_machines_busy[THIRD_JOBSHOP][station], station + num_stations*(THIRD_JOBSHOP-1));
    fprintf(logfile, "simtime[%.4f] station[%d] in jobshop[3] is IDLING\n", sim_time, station);
  }

  else
  {
    fprintf(logfile,"simtime[%.4f] job[%d] is UNQUEUED at station[%d] in jobshop[3]\n", sim_time, job_type, station);
    /* The queue is nonempty, so start service on first job in queue. */

    list_remove (FIRST, (THIRD_JOBSHOP-1)*num_stations + station);

    /* Tally this delay for this station. */

    sampst (sim_time - transfer[1], station + num_stations*(THIRD_JOBSHOP-1));

    /* Tally this same delay for this job type. */

    job_type_queue = transfer[2];
    task_queue = transfer[3];
    sampst (sim_time - transfer[1], num_stations*MAX_NUM_JOBSHOPS + job_type_queue);

    /* Schedule end of service for this job at this station.  Note defining
        attributes beyond the first two for the event record before invoking
        event_schedule. */

    transfer[3] = job_type_queue;
    transfer[4] = task_queue;
    event_schedule (sim_time + erlang (2, mean_service[job_type_queue][task_queue], STREAM_SERVICE), EVENT_DEPARTURE);
  }

  /* If the current departing job has one or more tasks yet to be done, send
     the job to the next station on its route. */

  if (task < num_tasks[job_type])
  {
    ++task;
    arrive_final_jobshop(0, THIRD_JOBSHOP);
  }
}

void arrive_final_jobshop(int is_new_job, int jobshop_number)
{
  int station;

  if(is_new_job){
    task = 1;
    fprintf(logfile, "simtime[%.4f] new job[%d] ARRIVING for jobshop[%d]\n", sim_time, job_type,jobshop_number);
  }

  station = route[job_type][task];

  fprintf(logfile, "simtime[%.4f] job[%d] ARRIVING for station[%d] in jobshop[%d]\n", sim_time, job_type, station, jobshop_number);

  if(num_machines_busy[jobshop_number][station] == num_machines[station]){
    /* All machines in this station are busy, so place the arriving job at
        the end of the appropriate queue. Note that the following data are
        stored in the record for each job:
        1. Time of arrival to this station.
        2. Job type.
        3. Current task number. */

    transfer[1] = sim_time;
    transfer[2] = job_type;
    transfer[3] = task;
    fprintf(logfile,"simtime[%.4f] job[%d] is QUEUED at station[%d] in jobshop[%d]\n", sim_time, job_type, station, jobshop_number);
    list_file(LAST, (jobshop_number-1)*num_stations + station);
  } else{
    /* A machine in this station is idle, so start service on the arriving
        job (which has a delay of zero). */

    sampst(0.0, station + num_stations*(jobshop_number-1)); /* For station */
    sampst(0.0, num_stations*MAX_NUM_JOBSHOPS + job_type);
    ++num_machines_busy[jobshop_number][station];
    timest((double) num_machines_busy[jobshop_number][station], station + num_stations*(jobshop_number-1));

    /*
      Schedule a service completion. Note defining attributes beyond the
      first two for the event record before invoking event_schedule.
    */

    transfer[3] = job_type;
    transfer[4] = task;

    event_schedule(sim_time + erlang(2, mean_service[job_type][task], STREAM_SERVICE), EVENT_DEPARTURE);
  }
}

void report (void)			/* Report generator function. */
{
  int i;
  double overall_avg_job_tot_delay, avg_job_tot_delay, sum_probs;

  /* Compute the average total delay in queue for each job type and the
     overall average job total delay. */

  fprintf (outfile, "\n\n\n\nJob type     Average total delay in queue");
  overall_avg_job_tot_delay = 0.0;
  sum_probs = 0.0;

  for (i = 1; i <= num_job_types; ++i)
  {
    avg_job_tot_delay = sampst (0.0, -(num_stations*MAX_NUM_JOBSHOPS + i)) * num_tasks[i];
    fprintf (outfile, "\n\n%4d%27.3f", i, avg_job_tot_delay);
    overall_avg_job_tot_delay += (prob_distrib_job_type[i] - sum_probs) * avg_job_tot_delay;
    sum_probs = prob_distrib_job_type[i];
  }

  fprintf (outfile, "\n\nOverall average job total delay =%10.3f\n", overall_avg_job_tot_delay);

  /* Compute the average number in queue, the average utilization, and the
     average delay in queue for each station. */

  fprintf (outfile, "\n\n\n Work      Average number      Average       Average delay");
  fprintf (outfile, "\nstation       in queue       utilization        in queue");
  for (i = 1; i <= MAX_NUM_JOBSHOPS; ++i)
  {
    for (j = 1; j <= num_stations; ++j)
      fprintf (outfile, "\n\n%4d%17.3f%17.3f%17.3f", (i-1)*num_stations + j, filest ((i-1)*num_stations + j), timest (0.0, -((i-1)*num_stations + j)) / num_machines[j], sampst (0.0, -((i-1)*num_stations + j)));
  } 
}

/* Main function. */
int main ()
{
  /* Open input and output files. */

  infile = fopen ("jobshop.in", "r");
  outfile = fopen ("jobshop.out", "w");
  logfile = fopen("log.txt", "w");

  /* Read input parameters. */

  fscanf (infile, "%d %d %lg %lg", &num_stations, &num_job_types, &mean_interarrival, &length_simulation);
  for (j = 1; j <= num_stations; ++j)
    fscanf (infile, "%d", &num_machines[j]);
  for (i = 1; i <= num_job_types; ++i)
    fscanf (infile, "%d", &num_tasks[i]);

  for (i = 1; i <= num_job_types; ++i){
    for (j = 1; j <= num_tasks[i]; ++j)
      fscanf (infile, "%d", &route[i][j]);
    for (j = 1; j <= num_tasks[i]; ++j)
      fscanf (infile, "%lg", &mean_service[i][j]);
  }

  for (i = 1; i <= num_job_types; ++i)
    fscanf (infile, "%lg", &prob_distrib_job_type[i]);

  /* Write report heading and input parameters. */

  fprintf (outfile, "Job-shop model\n\n");
  fprintf (outfile, "Number of work stations%21d\n\n", num_stations);
  fprintf (outfile, "Number of machines in each station     ");
  for (j = 1; j <= num_stations; ++j)
    fprintf (outfile, "%5d", num_machines[j]);
  fprintf (outfile, "\n\nNumber of job types%25d\n\n", num_job_types);
  fprintf (outfile, "Number of tasks for each job type      ");
  for (i = 1; i <= num_job_types; ++i)
    fprintf (outfile, "%5d", num_tasks[i]);
  fprintf (outfile, "\n\nDistribution function of job types  ");
  for (i = 1; i <= num_job_types; ++i)
    fprintf (outfile, "%8.3f", prob_distrib_job_type[i]);
  fprintf (outfile, "\n\nMean interarrival time of jobs%14.2f hours\n\n", mean_interarrival);
  fprintf (outfile, "Length of the simulation%20.1f eight-hour days\n\n\n", length_simulation);
  fprintf (outfile, "Job type     Work stations on route");
  for (i = 1; i <= num_job_types; ++i)
    {
      fprintf (outfile, "\n\n%4d        ", i);
      for (j = 1; j <= num_tasks[i]; ++j)
	fprintf (outfile, "%5d", route[i][j]);
    }
  fprintf (outfile, "\n\n\nJob type     ");
  fprintf (outfile, "Mean service time (in hours) for successive tasks");
  for (i = 1; i <= num_job_types; ++i)
    {
      fprintf (outfile, "\n\n%4d    ", i);
      for (j = 1; j <= num_tasks[i]; ++j)
	fprintf (outfile, "%9.2f", mean_service[i][j]);
    }

  /* Initialize all machines in all stations to the idle state. */

  for (i = 1; i <= MAX_NUM_JOBSHOPS; ++i){
    for(j = 1; j <= num_stations; ++j){
      num_machines_busy[i][j] = 0;
    }
  }


  /* Initialize simlib */

  init_simlib ();

  /* Set maxatr = max(maximum number of attributes per record, 4) */

  maxatr = 4;			/* NEVER SET maxatr TO BE SMALLER THAN 4. */

  /* Schedule the arrival of the first job. */

  event_schedule (expon (mean_interarrival, STREAM_INTERARRIVAL), EVENT_ARRIVAL);

  /* Schedule the end of the simulation.  (This is needed for consistency of
     units.) */

  event_schedule (8 * length_simulation, EVENT_END_SIMULATION);

  /* Run the simulation until it terminates after an end-simulation event
     (type EVENT_END_SIMULATION) occurs. */

  do
    {

      /* Determine the next event. */

      timing ();

      /* Invoke the appropriate event function. */

      switch (next_event_type)
	{
	case EVENT_ARRIVAL:
	  arrive();
	  break;
	case EVENT_DEPARTURE:
	  depart_final();
	  break;
  case EVENT_DEPARTURE_FIRST_JOBSHOP:
    depart_jobshop(1);
    break;
  case EVENT_DEPARTURE_SECOND_JOBSHOP:
    depart_jobshop(2);
    break;
  case EVENT_ARRIVAL_FINAL_JOBSHOP:
    arrive_final_jobshop(1, 3);
    break;
	case EVENT_END_SIMULATION:
	  report();
	  break;
	}

      /* If the event just executed was not the end-simulation event (type
         EVENT_END_SIMULATION), continue simulating.  Otherwise, end the
         simulation. */

    }
  while (next_event_type != EVENT_END_SIMULATION);

  fclose (infile);
  fclose (outfile);

  return 0;
}
