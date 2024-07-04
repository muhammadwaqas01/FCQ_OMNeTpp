#ifndef BUFFER_H
#define BUFFER_H

#include <omnetpp.h>
#include "QueuePolicy.h" // Include the QueuePolicy for job selection

using namespace omnetpp;

namespace processor {

class Buffer {
public:
    Buffer(int size, QueuePolicy* policy);
    ~Buffer();

    bool insertMessage(cMessage* msg);
    cMessage* popNextMessage(int availableCPU);
    cMessage* peekNextMessage(int availableCPU) const;
    void removeMessage(cMessage* msg);

    int getQueueLength() const;
    bool isEmpty() const;
    void printQueueDetails() const;

    std::vector<int> getBufferCountsBySource() const; // Method to get the buffer counts by source

private:
    cQueue queue;              ///< The queue used to store messages.
    int bufferSize;            ///< The maximum size of the buffer.
    QueuePolicy* queuePolicy;  ///< The policy used for selecting the next job.
};

} // namespace processor

#endif // BUFFER_H
