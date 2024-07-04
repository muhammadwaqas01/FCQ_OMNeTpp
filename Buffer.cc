#include "Buffer.h"
#include "QueuePolicy.h" // Assuming QueuePolicy definitions are used for selecting jobs

namespace processor {

Buffer::Buffer(int size, QueuePolicy* policy) : bufferSize(size), queuePolicy(policy) {
    queue.setName("queue");
}

Buffer::~Buffer() {
    delete queuePolicy; // Ensure the policy object is cleaned up
}

bool Buffer::insertMessage(cMessage* msg) {
    if (queue.getLength() >= bufferSize) {
        return false; // Buffer is full
    } else {
        simtime_t currentTime = simTime();
        msg->addPar("arrivalTime");
        msg->par("arrivalTime").setDoubleValue(currentTime.dbl());
        queue.insert(msg);
        return true;
    }
}

cMessage* Buffer::peekNextMessage(int availableCPU) const {
    if (isEmpty()) {
        return nullptr;
    }
    return queuePolicy->peekNextJob(queue, availableCPU);
}

cMessage* Buffer::popNextMessage(int availableCPU) {
    if (isEmpty()) {
        return nullptr; // No message to pop
    }
    cMessage* msg = peekNextMessage(availableCPU); // Use updated peekNextMessage with availableCPU
    if (msg) {
        queue.remove(msg); // Actually remove the message from the queue
    }
    return msg;
}

void Buffer::removeMessage(cMessage* msg) {
    queue.remove(msg);
}

int Buffer::getQueueLength() const {
    return queue.getLength();
}

bool Buffer::isEmpty() const {
    return queue.isEmpty();
}

void Buffer::printQueueDetails() const {
    if (queue.isEmpty()) {
        EV << "Queue is empty.\n";
        return;
    }
    EV << "Queue details (Total " << queue.getLength() << " jobs):\n";
    for (cQueue::Iterator iter(queue); !iter.end(); ++iter) {
        cMessage *job = dynamic_cast<cMessage *>(*iter);
        if (job) {
            EV << "  Job ID: " << job->getId()
               << ", Source: " << job->getSenderModule()->getFullName()
               << ", Arrival Time: " << job->par("arrivalTime").doubleValue()
               << ", Service Time: " << job->par("serviceTime").doubleValue()
               << ", Req. Resources: " << static_cast<int>(job->par("requiredResource").longValue())<< "\n";
        }
    }
}

std::vector<int> Buffer::getBufferCountsBySource() const {
    std::vector<int> bufferCounts(2, 0); // Adjust size as necessary for the number of sources
    for (cQueue::Iterator iter(queue); !iter.end(); ++iter) {
        cMessage* job = dynamic_cast<cMessage*>(*iter);
        if (job) {
            std::string sourceId = job->getSenderModule()->getFullName();
            std::string prefix = "source[";
            size_t startPos = sourceId.find(prefix);
            if (startPos != std::string::npos) {
                startPos += prefix.length(); // Move start position past "source["
                size_t endPos = sourceId.find(']', startPos);
                if (endPos != std::string::npos) {
                    std::string numPart = sourceId.substr(startPos, endPos - startPos);
                    try {
                        int sourceIndex = std::stoi(numPart);
                        if (sourceIndex >= 0 && sourceIndex < bufferCounts.size()) {
                            bufferCounts[sourceIndex]++;
                        }
                    } catch (const std::invalid_argument& ia) {
                        EV_ERROR << "Error parsing source index from " << sourceId << ". Error: " << ia.what() << std::endl;
                    }
                }
            }
        }
    }
    return bufferCounts;
}

} // namespace processor
