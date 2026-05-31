#pragma once
#include <stdbool.h>

#define _tracks_ 4  // Number of tracks
#define _steps_  16 // Number of steps


typedef struct
{
    unsigned char current;  // Current step
    unsigned departed;      // Current time in samples
    unsigned length;        // Step duration in samples

} sequencer;


void    init_sequence(sequencer* o, int l);

void    loop_forward (sequencer* o);
void    loop_backward(sequencer* o);
void    loop_pingpong(sequencer* o);
void    loop_random  (sequencer* o);

