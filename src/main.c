// MIT License
// 
// Copyright (c) 2023 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include "utility.h"
#include "star.h"
#include "float.h"
#include <pthread.h>

#define NUM_STARS 30000 
#define MAX_LINE 1024
#define DELIMITER " \t\n"

struct Star star_array[ NUM_STARS ];
uint8_t   (*distance_calculated)[NUM_STARS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

double  min  = FLT_MAX;
double  max  = FLT_MIN;

double mean = 0;
double threadMean = 0;
uint64_t threadCount = 0;
int num_threads = 0;

struct thread_arguments {
  int thread_id;
  int start_index;
  int end_index;
  double result;
  uint64_t t_count;
};

void showHelp()
{
  printf("Use: findAngular [options]\n");
  printf("Where options are:\n");
  printf("-t          Number of threads to use\n");
  printf("-h          Show this help\n");
}

 
// Embarassingly inefficient, intentionally bad method
// to calculate all entries one another to determine the
// average angular separation between any two stars 

// THREAD VERSION
void* determineAverageAngularDistanceTHREAD(void* argv)
{

    struct thread_arguments* thread_args = (struct thread_arguments *) argv;

    int id = thread_args->thread_id;
    int start_index = thread_args->start_index;
    int end_index = thread_args->end_index;

    uint32_t i, j;
    uint64_t count = 0;

    double inner_mean = 0;


    printf("Thread %d starts at %d and ends at %d\n", id, start_index, end_index);

    for (i = start_index; i <= end_index; i++)
    {
      for (j = 0; j < NUM_STARS; j++)
      {
        if( i!=j && distance_calculated[i][j] == 0 )
        {
          double distance = calculateAngularDistance( star_array[i].RightAscension, star_array[i].Declination,
                                                      star_array[j].RightAscension, star_array[j].Declination ) ;
          distance_calculated[i][j] = 1;
          distance_calculated[j][i] = 1;

          pthread_mutex_lock(&mutex);
          threadCount++;
          if( min > distance )
          {
            min = distance;
          }

          if( max < distance )
          {
            max = distance;
          }
          threadMean = threadMean + (distance-threadMean)/threadCount;
          pthread_mutex_unlock(&mutex);

        }
      }
    }

    // printf("count: %ld\n", threadCount);
    thread_args->t_count = count;
    thread_args->result = inner_mean;
    // return mean;
}

// NONTHREAD
float determineAverageAngularDistance( struct Star arr[] )
{
    double mean = 0;

    uint32_t i, j;
    uint64_t count = 0;


    for (i = 0; i < NUM_STARS; i++)
    {
      for (j = 0; j < NUM_STARS; j++)
      {
        if( i!=j && distance_calculated[i][j] == 0 )
        {
          double distance = calculateAngularDistance( arr[i].RightAscension, arr[i].Declination,
                                                      arr[j].RightAscension, arr[j].Declination ) ;
          distance_calculated[i][j] = 1;
          distance_calculated[j][i] = 1;
          count++;

          if( min > distance )
          {
            min = distance;
          }

          if( max < distance )
          {
            max = distance;
          }
          mean = mean + (distance-mean)/count;
        }
      }
    }

    // printf("count: %ld\n", count);
    return mean;
}


int main( int argc, char * argv[] )
{

  FILE *fp;
  uint32_t star_count = 0;

  uint32_t n;

  // multithreading on = 1, multithreading off = 0
  int mode = 0;

  distance_calculated = malloc(sizeof(uint8_t[NUM_STARS][NUM_STARS]));

  if( distance_calculated == NULL )
  {
    uint64_t num_stars = NUM_STARS;
    uint64_t size = num_stars * num_stars * sizeof(uint8_t);
    printf("Could not allocate %ld bytes\n", size);
    exit( EXIT_FAILURE );
  }

  int i, j;
  // default every thing to 0 so we calculated the distance.
  // This is really inefficient and should be replace by a memset
  for (i = 0; i < NUM_STARS; i++)
  {
    for (j = 0; j < NUM_STARS; j++)
    {
      distance_calculated[i][j] = 0;
    }
  }

  for( n = 1; n < argc; n++ )          
  {
    if( strcmp(argv[n], "-help" ) == 0 )
    {
      showHelp();
      exit(0);
    }

    if( strcmp(argv[n], "-t") == 0)
    {
      num_threads = atoi(argv[n+1]);
      // printf("number of threads is %d\n", num_threads);
      mode = 1;
    }

  }

  fp = fopen( "data/tycho-trimmed.csv", "r" );

  if( fp == NULL )
  {
    printf("ERROR: Unable to open the file data/tycho-trimmed.csv\n");
    exit(1);
  }

  char line[MAX_LINE];
  while (fgets(line, 1024, fp))
  {
    uint32_t column = 0;

    char* tok;
    for (tok = strtok(line, " ");
            tok && *tok;
            tok = strtok(NULL, " "))
    {
       switch( column )
       {
          case 0:
              star_array[star_count].ID = atoi(tok);
              break;
       
          case 1:
              star_array[star_count].RightAscension = atof(tok);
              break;
       
          case 2:
              star_array[star_count].Declination = atof(tok);
              break;

          default: 
             printf("ERROR: line %d had more than 3 columns\n", star_count );
             exit(1);
             break;
       }
       column++;
    }
    star_count++;
  }
  printf("%d records read\n", star_count );

  double distance = 0;

  // Find the average angular distance in the most inefficient way possible
  pthread_t tid[num_threads];
  int thread_id[num_threads];
  struct thread_arguments thread_args[num_threads];

  double sum = 0;

  uint64_t total_count = 0;

  if(mode == 1)
  {
    for(int i = 0; i < num_threads; i++)
    {
      thread_args[i].thread_id = i;
      thread_args[i].start_index = (NUM_STARS/num_threads)*thread_args[i].thread_id;
      thread_args[i].end_index = ((NUM_STARS/num_threads)*(thread_args[i].thread_id+1)-1);

      pthread_create(&tid[i], NULL, (void*) determineAverageAngularDistanceTHREAD, &thread_args[i]);
    }

    for(int i = 0; i < num_threads; i++)
    {
      pthread_join(tid[i], NULL);
    }
  }
  else
  {
    mean = determineAverageAngularDistance(star_array);
  }


  printf("Average distance found is %lf\n", mode == 1 ? threadMean : mean);
  printf("Minimum distance found is %lf\n", min );
  printf("Maximum distance found is %lf\n", max );

  return 0;
}

