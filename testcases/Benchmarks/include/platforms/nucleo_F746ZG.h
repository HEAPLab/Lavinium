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
#ifndef PLATFORM_NUCLEO_F746ZG_H_
#define PLATFORM_NUCLEO_F746ZG_H_
#include "stm32f7xx_hal.h"

#ifndef WITH_ANSI

    #define MEASURE_GLOBAL_VARIABLES()  extern TIM_HandleTypeDef htim5;\
                                       unsigned int result;
    #define MEASURE_START()  do { \
                              HAL_TIM_Base_Start(&htim5); \
                           } while(0);

    #define MEASURE_STOP() do { \
                                 HAL_TIM_Base_Stop(&htim5); \
                                  result=__HAL_TIM_GET_COUNTER(&htim5); \
                                 __HAL_TIM_SET_COUNTER(&htim5,0); \
                              } while(0);                    

#else
   #define MEASURE_GLOBAL_VARIABLES()  extern TIM_HandleTypeDef htim5;\
                                       long unsigned int result;
   #define MEASURE_START()  do { \
                              HAL_TIM_Base_Start(&htim5); \
                           } while(0);

   #define MEASURE_STOP() do { \
                                 HAL_TIM_Base_Stop(&htim5); \
                                  result=__HAL_TIM_GET_COUNTER(&htim5); \
                                 __HAL_TIM_SET_COUNTER(&htim5,0); \

                              } while(0);
#endif

#endif
