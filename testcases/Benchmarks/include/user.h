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
#ifndef USER_H_
#define USER_H_

/** Set this to 0 to disable the outer loop on benchmarks (it will considerably reduce the execution time) **/
#define USE_ITERATIONS_LOOP 1

#include "dev_classes.h"

/**Uncomment the following constants to enable the use of user-defined images in the respective benchmarks  **/

/*#define USER_AVG_FILTER_RGB*/
/*#define USER_GAUSS_FILTER_RGB*/

/*#define USER_AVG_FILTER*/
/*#define USER_GAUSS_FILTER*/
/*#define USER_DILATE*/
/*#define USER_ERODE*/

/*#define USER_IMAGE_CANNY*/
/*#define USER_IMAGE_SCALING*/
/*#define USER_JPEG_COMPRESS*/

#define UINT32_T unsigned int

/** User-needed constants (e.g., for measurements) **/
#define BILLION 1000000000L
#define SEED 123U

#ifdef PLATFORM_LINUX_NUCLEO_F746ZG
    #include "platforms/nucleo_F746ZG.h"
#elif defined(PLATFORM_LINUX)
    #include "platforms/linux.h"
#else
   #define MEASURE_GLOBAL_VARIABLES()
   #define MEASURE_START()
   #define MEASURE_STOP() 
   #define CHECK_RESULT(x) while(!(x)){};
#endif

#define ANN_LOOP_BOUND(iters)
#define ANN_VAR(min,max)
#define ANN_VAR_NOBOUNDS()
#define ANN_ARRAY(min,max)
#define ANN_ARRAY_NOBOUNDS() 

#endif
