#ifndef PIPELINE_H
#define PIPELINE_H

#include "it_port.h"

// --- Unified Connection Helper ---

template <typename T>
void ipl_linkPort(ItOutputPort<T>* output, ItInputPort<T>* input)
{
    if (input == NULL || output == NULL) return;
    input->setUpstreamPort(output);
    output->setDownstreamPort(input);
}

#endif // PIPELINE_H