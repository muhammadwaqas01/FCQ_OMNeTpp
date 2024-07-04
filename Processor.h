#ifndef __PROCESSOR_H
#define __PROCESSOR_H

#include <omnetpp.h>
#include <vector>
#include <map>
#include "Buffer.h"
#include "QueuePolicy.h"
#include <string>
using namespace omnetpp;

namespace processor {

class Processor : public cSimpleModule
{
  protected:
    cMessage *endServiceMsg = nullptr;
    cQueue queue;
    Buffer* buffer;
    int bufferSize;
    long ResourceCapacity;

    std::string schedulingPolicy;
    std::vector<cMessage*> activeJobs;
    std::map<cMessage*, cMessage*> endServiceMsgs;


    long sumOfOccupiedResource = 0;

    int checkCounts = 0;
    double checkInterval;

    long numOfCheckIntervals = 0;
//    long cumulativePacketsInProgress = 0;
    long logDetailsCount = 0;
    std::vector<long> msgProcessed;
    std::map<std::string, simsignal_t> signalMap;
    std::vector<long> msgDropped;
    std::vector<double> totalServiceTime;

    std::vector<int> msgsInServiceCount; // Holds the number of messages in service for each check interval
    std::vector<double> avgMsgsInService; // Holds the average number of messages in service

    std::vector<double> totalWaitingTime;
    std::vector<double> waitingCount;
    std::vector<double> totalResponseTime;
    std::vector<int> responseCount;

    QueuePolicy* policy = nullptr; // Policy member variable

    std::vector<int> msgsInBufferCount; // Holds the cumulative number of messages in buffer from each source
    std::vector<double> avgMsgsInBuffer; // Holds the average number of messages in buffer from each source

  public:
    virtual ~Processor();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual simtime_t startService(cMessage *msg);
    virtual void endService(cMessage *msg);
    virtual void finish() override;

    // Existing declarations
    virtual void handleResourceCheck();
    virtual void handleJobArrival(cMessage *msg);
    virtual void processQueue();
    virtual bool canStartNextJob();
    virtual void startNextJob(cMessage *job);
    long sumOfResourceUsedByActiveJobs();


    // Utility functions
    void printQueueDetails(const cQueue &queue);
    void printActiveJobsDetails(const std::vector<cMessage*>& activeJobs);
    void logQueueDetails();
    void registerDynamicSignals();
    void emitDynamicSignal(const std::string& signalName, double value, const std::string& sourceId); // Added declaration
};

};

#endif // __PROCESSOR_H
