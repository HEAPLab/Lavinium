/******************************************************************************
*   Copyright 2021 Politecnico di Milano
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*******************************************************************************/
#ifndef PLATFORM_LINUX_H_
#define PLATFORM_LINUX_H_

/** User-needed libraries (e.g., for measurements) **/
#include <assert.h>
#include <stdio.h>
#include <time.h>

/** Variables declared in the global scope to support measurements **/
#define MEASURE_GLOBAL_VARIABLES()  static struct timespec start,stop; \
                                    double result;

/** The code to be executed when the time measurement starts **/
#define MEASURE_START()  do { \
                              result=0; \
                              clock_gettime(CLOCK_MONOTONIC, &start); \
                           } while(0);

/** The code to be executed when the time measurement stops **/
#define MEASURE_STOP() do { \
                              clock_gettime(CLOCK_MONOTONIC, &stop); \
                              result=(stop.tv_sec - start.tv_sec)*BILLION + (stop.tv_nsec - start.tv_nsec); \
                           } while(0);

#define CHECK_RESULT(x) assert(x);

#endif
