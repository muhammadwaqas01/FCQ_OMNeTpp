#include "QueuePolicy.h"

namespace processor {

cMessage* FIFOQueuePolicy::peekNextJob(const cQueue& queue, int availableCPU) const {
    if (queue.isEmpty()) return nullptr;
    return static_cast<cMessage*>(queue.front());
}

cMessage* PriorityCPUQueuePolicy::peekNextJob(const cQueue& queue, int availableCPU) const {
    if (queue.isEmpty()) {
        return nullptr;
    }
    cMessage* jobWithHighestPriority = nullptr;
    long highestPriorityValue = LONG_MIN;
    for (cQueue::Iterator iter(queue, false); !iter.end(); iter++) {
        cMessage* currentJob = dynamic_cast<cMessage*>(*iter);
        if (currentJob) {
            long priorityValue = currentJob->par("requiredCPU").longValue();
            if (priorityValue > highestPriorityValue) {
                highestPriorityValue = priorityValue;
                jobWithHighestPriority = currentJob;
            }
        }
    }
    return jobWithHighestPriority;
}

cMessage* MostServerFitQueuePolicy::peekNextJob(const cQueue& queue, int availableCPU) const {
    if (queue.isEmpty()) {
        return nullptr;
    }
    cMessage* bestFitJob = nullptr;
    long bestFitValue = LONG_MIN;
    for (cQueue::Iterator iter(queue, false); !iter.end(); iter++) {
        cMessage* currentJob = dynamic_cast<cMessage*>(*iter);
        if (currentJob) {
            long requiredCPU = currentJob->par("requiredCPU").longValue();
            if (requiredCPU <= availableCPU && requiredCPU > bestFitValue) {
                bestFitValue = requiredCPU;
                bestFitJob = currentJob;
            }
        }
    }
    return bestFitJob;
}

} // namespace processor
