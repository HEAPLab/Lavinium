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

#include "user.h"
#include "simple_random.h"

#define TIME_STEP 0.001

#define CLAMP(x, minim, maxim) (x < minim ? minim : (x > maxim ? maxim : x))

MEASURE_GLOBAL_VARIABLES()


typedef struct pid_controller_s {

    double p;
    double i;
    double d;
    double b;
    
    double prev_error;
    double integral_sum;
    double backpropagation;
    
} pid_controller_t;


double run_pid(pid_controller_t* pid, double error) {
    double output; 
    
    output = error * pid->p;
    pid->integral_sum += ((error * pid->i) + (pid->b * pid->backpropagation)) * TIME_STEP;
    output += pid->integral_sum;

    output += pid->d * (error - pid->prev_error) / TIME_STEP;

    pid->prev_error = error;

    return output;
}

double roll_limiter(double desired_roll, double speed) {
    double limit_perc,limit;

    if (speed <= 140) {
        return CLAMP(desired_roll, -30, 30);
    }
    if (speed >= 300) {
        return CLAMP(desired_roll, -40, 40);    
    }
        
    limit_perc = (speed < 220) ? (speed-140) / 80 : ((speed-220) / 80);
    limit = (speed < 220) ? (30 + limit_perc * 37) : (40 + (1-limit_perc) * 27);
    
    return CLAMP (desired_roll, -limit, limit);
}

double roll_rate_limiter(double desired_roll_rate, double roll) {
    if (roll < 20 && roll > -20) {
       return CLAMP (desired_roll_rate, -5, 5);
    } else if (roll < 30 && roll > -30) {
       return CLAMP (desired_roll_rate, -3, 3);    
    } else {
        return CLAMP (desired_roll_rate, -1, 1);
    }
}

double ailerons_limiter(double aileron) {
    return CLAMP(aileron, -30, 30);
}


void latnav() {
    pid_controller_t pid_roll_rate,pid_roll,pid_heading;
    double curr_heading,curr_roll,curr_roll_rate;
    int i;

    MEASURE_START();
    for(i=0; i<ITERATIONS; i++) {
    
        double desired_roll,actual_roll,desired_roll_rate,actual_roll_rate,desired_ailerons,actual_ailerons;
        
        desired_roll = run_pid(&pid_heading, curr_heading-random_get()); 
        actual_roll = roll_limiter(desired_roll, 400);
        pid_heading.backpropagation = actual_roll - desired_roll; 
        
        desired_roll_rate = run_pid(&pid_roll, curr_roll - actual_roll); 
        actual_roll_rate = roll_rate_limiter(desired_roll_rate, curr_roll);
        pid_roll.backpropagation = actual_roll_rate - desired_roll_rate; 

        
        desired_ailerons = run_pid(&pid_roll, curr_roll_rate - actual_roll_rate); 
        actual_ailerons = ailerons_limiter(desired_roll_rate);
        pid_roll.backpropagation = actual_ailerons - desired_ailerons; 

        /* Just a random plane model*/
        curr_heading += curr_roll/10 * TIME_STEP;
        curr_roll += curr_roll_rate * TIME_STEP;
        curr_roll_rate += desired_ailerons / 5;

    }
    MEASURE_STOP();
}
