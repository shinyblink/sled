// block_for is a module to block the execution for a certain time
//
// Usage:
//
// #include <block_for.h>
//
// ...
//
// int draw(){
//     // Check if currently blocked and die
//     if (check_block()) return 1;
//
//     ...
//
//     if (want_to_exit) {
//         block_for(2); // block for 2 minutes
//         return 1; // die
//     } else {
//         return 0; // live
//     }
// }


#include <timers.h>

static oscore_time __earliest_execution_time;

static int check_block() {
    return (udate() < __earliest_execution_time);
}

static void block_for(int minutes) {
    __earliest_execution_time = udate() + minutes * 60UL*1000UL*1000UL;
}
