#ifndef QUEUEPOLICY_H_
#define QUEUEPOLICY_H_

#include <omnetpp.h>
using namespace omnetpp;

namespace processor {

class QueuePolicy {
public:
    virtual cMessage* peekNextJob(const cQueue& queue, int availableResource) const = 0; // Updated signature
    virtual ~QueuePolicy() {}
};

class FIFOQueuePolicy : public QueuePolicy {
public:
    virtual cMessage* peekNextJob(const cQueue& queue, int availableResource) const override;
};

class PriorityCPUQueuePolicy : public QueuePolicy {
public:
    virtual cMessage* peekNextJob(const cQueue& queue, int availableResource) const override;
};


} // namespace processor

#endif /* QUEUEPOLICY_H_ */
