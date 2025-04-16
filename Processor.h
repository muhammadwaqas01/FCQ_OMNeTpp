// Copyright (C) [2025] [Muhammad Waqas]
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.



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
