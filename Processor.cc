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




#include "Processor.h"
#include "Buffer.h"
#include <fstream>
#include "QueuePolicy.h"

namespace processor {

Define_Module(Processor);

Processor::~Processor() {
    cancelAndDelete(endServiceMsg);
    for (auto &msgPair : endServiceMsgs) {
        cancelAndDelete(msgPair.first);
    }
    delete buffer;
    EV << "Destructor: Cleaned up all endServiceMsgs, the main endServiceMsg, and the buffer object." << endl;
}

void Processor::initialize() {
    endServiceMsg = new cMessage("end-service");

    // Buffer size and policy are now encapsulated within Buffer
    int bufferSize = par("bufferSize").intValue();
    std::string policyName = par("schedulingPolicy").stdstringValue();
    QueuePolicy* policy = nullptr;
    if (policyName == "Priority") {
        policy = new PriorityCPUQueuePolicy();
    } else {
        policy = new FIFOQueuePolicy(); // Default to FIFO if no valid policy is specified
    }
    buffer = new Buffer(bufferSize, policy);

    ResourceCapacity = par("ResourceCapacity").intValue();
    checkInterval = par("checkInterval").doubleValue();

    msgProcessed.resize(2, 0);
    msgDropped.resize(2, 0);
    totalServiceTime.resize(2, 0.0);

    msgsInServiceCount.resize(2, 0);
    avgMsgsInService.resize(2, 0.0);


    totalWaitingTime.resize(2, 0.0);
    waitingCount.resize(2, 0);

    totalResponseTime.resize(2, 0.0);
    responseCount.resize(2, 0);

    msgsInBufferCount.resize(2, 0); // Initialize the buffer count vector
    avgMsgsInBuffer.resize(2, 0.0); // Initialize the average buffer vector



    registerDynamicSignals();

    scheduleAt(simTime() + checkInterval, new cMessage("checkResource"));

    EV << "Initialize: Queue system initialized with ResourceCapacity=" << ResourceCapacity
       << ", checkInterval=" << checkInterval << endl;
}


void Processor::handleMessage(cMessage *msg) {
    if (endServiceMsgs.count(msg) > 0) {
        // Extract the associated job
        cMessage *job = endServiceMsgs[msg];

        // Complete the service for this job
        endService(job);
        cancelAndDelete(msg);
        endServiceMsgs.erase(msg);

        EV << "Post-release: ActiveJobsCount=" << activeJobs.size() << ".\n";
        processQueue();
        printActiveJobsDetails(activeJobs);

    } else if (strcmp(msg->getName(), "checkResource") == 0) {
        handleResourceCheck();
        delete msg;
    } else {
        handleJobArrival(msg);
    }
}


void Processor::registerDynamicSignals() {
    for (int i = 0; i < 2; ++i) {
        std::string baseName = "source" + std::to_string(i);
        signalMap[baseName + "MsgDropped"] = registerSignal((baseName + "MsgDropped").c_str());
        signalMap[baseName + "MsgProcessed"] = registerSignal((baseName + "MsgProcessed").c_str());
        EV_DETAIL << "Registered dynamic signal for source" << i << "\n";

    }
}
void Processor::emitDynamicSignal(const std::string& signalName, double value, const std::string& sourceId) {
    std::string fullSignalName = sourceId + signalName;
    if (signalMap.find(fullSignalName) != signalMap.end()) {
        emit(signalMap[fullSignalName], value);
    } else {
        EV_ERROR << "Signal " << fullSignalName << " not found. Ensure it's registered correctly." << endl;
    }
}
void printQueueDetails(const cQueue &queue) {
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
               << ", Service Time: " << job->par("serviceTime").doubleValue()
               << ", Arrival Time: " << job->par("arrivalTime").doubleValue()
               << ", Req. Resource: " << static_cast<int>(job->par("requiredResource").longValue()) << "\n";
        }
    }
}

void Processor::printActiveJobsDetails(const std::vector<cMessage*>& activeJobs) {
    if (activeJobs.empty()) {
        EV << "No active jobs.\n";
        return;
    }
    EV << "Active jobs details (Total " << activeJobs.size() << " jobs):\n";
    for (auto job : activeJobs) {
        // Calculate remaining service time
        double serviceStartTime = job->par("serviceStartTime").doubleValue();
        double totalServiceTime = job->par("serviceTime").doubleValue();
        double remainingServiceTime = (serviceStartTime + totalServiceTime) - simTime().dbl();
        if (remainingServiceTime < 0) {
            remainingServiceTime = 0; // Ensure the remaining service time is not negative
        }

        EV << "  Job ID: " << job->getId()
           << ", Source: " << job->getSenderModule()->getFullName()
           << ", Arrival Time: " << job->par("arrivalTime").doubleValue()
           << ", Service Time: " << totalServiceTime
           << ", Req. CPU: " << static_cast<int>(job->par("requiredResource").longValue())
           << ", Remaining Service Time: " << remainingServiceTime << "\n"; // Log the remaining service time
    }
}

void Processor::handleResourceCheck() { //Resource usage of active jobs
//    cumulativePacketsInProgress += activeJobs.size();
    checkCounts++;

    // Calculate current resource usage
    long currentResourceUsage = sumOfResourceUsedByActiveJobs();


    // Accumulate the resource usage
    sumOfOccupiedResource += currentResourceUsage;

    // Log the starting point of resource check
    EV << "Resource check at time: " << simTime() << " with " << activeJobs.size() << " active jobs.\n";
    EV << "Current Resource Usage: " << currentResourceUsage << ", Total: " << sumOfOccupiedResource << "\n";

    // Calculate and accumulate the number of messages in service for each source
    std::vector<int> currentIntervalCount(2, 0); // Assuming two sources; adjust size as necessary

    for (auto job : activeJobs) {
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
                    if (sourceIndex >= 0 && sourceIndex < currentIntervalCount.size()) {
                        currentIntervalCount[sourceIndex]++;
                    }
                } catch (const std::invalid_argument& ia) {
                    EV_ERROR << "Error parsing source index from " << sourceId << ". Error: " << ia.what() << std::endl;
                }
            }
        }
    }

    // Accumulate and log detailed counts
    for (int i = 0; i < currentIntervalCount.size(); ++i) {
        msgsInServiceCount[i] += currentIntervalCount[i];  // Accumulate counts
        EV << "Total messages from source" << i << " in service until now: " << msgsInServiceCount[i] << "\n";
    }

    // Record the number of messages in the buffer from each source
    std::vector<int> bufferCounts = buffer->getBufferCountsBySource();
    for (int i = 0; i < bufferCounts.size(); ++i) {
        msgsInBufferCount[i] += bufferCounts[i];
        EV << "Total messages from source" << i << " in buffer until now: " << msgsInBufferCount[i] << "\n";
    }

    // Schedule the next check
    scheduleAt(simTime() + checkInterval, new cMessage("checkResource"));
}


void Processor::handleJobArrival(cMessage* msg) {
    msg->addPar("arrivalTime");
    msg->par("arrivalTime").setDoubleValue(simTime().dbl());
    // Logic to handle job arrival using the Buffer instance
    if (!buffer->insertMessage(msg)) {
        // If message insertion fails, it means the buffer is full
        EV << "Buffer full, dropping: ID=" << msg->getId() << ".\n";
        // Increment dropped message count for the source
        std::string sourceId = msg->hasPar("origin") ? msg->par("origin").stringValue() : "unknown";
        int sourceIndex = std::stoi(sourceId.substr(6)); // Assuming sourceId follows "sourceXX" format
        msgDropped[sourceIndex]++;
        delete msg;
    } else {
        // Successfully queued message
        EV << "Message queued successfully.\n";
        // Optionally, print buffer details or handle queued message further
        buffer->printQueueDetails();
        processQueue(); // Ensure this call is here
    }
}

void Processor::processQueue() {
    while (!buffer->isEmpty() && canStartNextJob()) {
        cMessage* nextJob = buffer->popNextMessage(ResourceCapacity);
        if (nextJob) {
            // Now nextJob is declared and can be used
            long requiredResource = nextJob->par("requiredResource").longValue();

            // Ensure the conditions are met to start the job
            if (requiredResource <= ResourceCapacity) {
                startNextJob(nextJob);             }
        }
    }
}

long Processor::sumOfResourceUsedByActiveJobs() {
    long totalCPUUsed = 0;

    for (const auto& job : activeJobs) {
        totalCPUUsed += static_cast<int>(job->par("requiredResource").longValue());


    }
    return totalCPUUsed;

}


bool Processor::canStartNextJob() {
    // Get the next job, but first, make sure to pass the available CPU resources
    cMessage* nextJob = buffer->peekNextMessage(ResourceCapacity); // Pass ResourceCapacity as the available CPU
    if (!nextJob) return false;

    long requiredResource = static_cast<long>(nextJob->par("requiredResource").longValue());


    return requiredResource <= ResourceCapacity;
}


void Processor::startNextJob(cMessage *job) {
    // Record the start of service time
    simtime_t serviceStartTime = simTime();
    job->addPar("serviceStartTime");
    job->par("serviceStartTime").setDoubleValue(serviceStartTime.dbl());

    // Calculate waiting time
    simtime_t arrivalTime = job->par("arrivalTime").doubleValue();
    simtime_t waitingTime = serviceStartTime - arrivalTime;

    // Deduce the source index from the job's parameters or metadata
    std::string sourceId = job->getSenderModule()->getFullName();
    int sourceIndex = std::stoi(sourceId.substr(sourceId.find("[") + 1, sourceId.find("]") - sourceId.find("[") - 1));

    // Accumulate waiting times and count for averaging later
    totalWaitingTime[sourceIndex] += waitingTime.dbl();
    waitingCount[sourceIndex]++;

    // Process job resources
    long requiredResource = static_cast<long>(job->par("requiredResource").longValue());


    ResourceCapacity -= requiredResource; // Update the available resource capacity.


    activeJobs.push_back(job); // Add the job to the list of active jobs.

    // Assuming the source ID is stored in a parameter named "sourceID".
    if (!job->hasPar("origin")) {
        job->addPar("origin").setStringValue(sourceId.c_str());
    }

    EV << "Resource Update: Job started: ID=" << job->getId()
       << ", SourceID=" << sourceId
       << ", ConsumedResource=" << requiredResource
       << ", RemainingResource=" << ResourceCapacity<< ".\n";


    // After adding the job to active jobs, print the details of all active jobs.
    EV << "After starting new job, active jobs details:\n";
    printActiveJobsDetails(activeJobs);

    // Schedule end of service
    cMessage *endServiceMsg = new cMessage("end-service", job->getId());
    endServiceMsgs[endServiceMsg] = job;
    scheduleAt(simTime() + job->par("serviceTime").doubleValue(), endServiceMsg);
}




void Processor::logQueueDetails() {
    logDetailsCount++;
    std::map<std::string, int> messagesInService, messagesInBuffer;

    for (auto& job : activeJobs) {
        messagesInService[job->getSenderModule()->getName()]++;
    }

    for (cQueue::Iterator iter(queue); !iter.end(); ++iter) {
        cMessage* job = (cMessage*)*iter;
        messagesInBuffer[job->getSenderModule()->getName()]++;
    }
}


simtime_t Processor::startService(cMessage *msg) {
    simtime_t serviceTime = msg->par("serviceTime").doubleValue();
    EV << "Starting service of " << msg->getName() << " with service time: " << serviceTime << endl;
    return serviceTime;
}

void Processor::endService(cMessage *msg) {
    simtime_t finishTime = simTime();
    simtime_t arrivalTime = msg->par("arrivalTime").doubleValue();
    simtime_t serviceStartTime = msg->par("serviceStartTime").doubleValue();
    simtime_t serviceTime = finishTime - serviceStartTime;
    simtime_t waitTime = serviceStartTime - arrivalTime;
    simtime_t responseTime = waitTime + serviceTime;

    // Extract the source ID from the message and calculate the source index
    std::string sourceId = msg->par("origin").stringValue();
    int sourceIndex = std::stoi(sourceId.substr(6)); // Assuming the sourceId is of the form "sourceXX"

    // Here we perform the accumulation
    totalServiceTime[sourceIndex] += serviceTime.dbl();
    totalResponseTime[sourceIndex] += responseTime.dbl();

    // Increase the count of processed messages for this source
    msgProcessed[sourceIndex]++;
    responseCount[sourceIndex]++;

    // Emit signal to indicate the message has been processed
    emitDynamicSignal("MsgProcessed", msgProcessed[sourceIndex], sourceId);

    // Log the wait time, service time, cumulative wait time, and other details of the job
    EV << "Job ID=" << msg->getId() << "From: " << sourceId << "\n"
       << "Start of Service Time: " << serviceStartTime << "\n"
       << "Wait Time: " << waitTime << "\n"
       << "Service Time: " << serviceTime << "\n"
       << "Completed service of " << msg->getName() << "\n"
       << "Total processed from this source: " << msgProcessed[sourceIndex] << "\n"
       << "Cumulative Wait Time for this source: " << totalWaitingTime[sourceIndex] << "\n"
       << "Cumulative Service Time for this source: " << totalServiceTime[sourceIndex] << "\n";

    // Resource release and logging
    int releasedResource = static_cast<int>(msg->par("requiredResource").longValue());

    ResourceCapacity += releasedResource;


    EV << "Releasing resources: Job ID=" << msg->getId()
       << ", Source ID=" << sourceId
       << ", releasedResource=" << releasedResource
       << ", NewTotalResource=" << ResourceCapacity<< ".\n";

    // Remove the job from activeJobs
    auto it = std::find(activeJobs.begin(), activeJobs.end(), msg);
    if (it != activeJobs.end()) {
        activeJobs.erase(it);
        EV << "Job ID=" << msg->getId() << " removed from active jobs.\n";
    } else {
        EV << "Job ID=" << msg->getId() << " not found in active jobs on completion.\n";
    }

    // Pass the message to the out gate
    send(msg, "out");

    processQueue();
}

void Processor::finish() {
    for (int i = 0; i < 2; ++i) {
        if (msgProcessed[i] > 0) {
            double averageWaitingTime = totalWaitingTime[i] / msgProcessed[i];
            double averageServiceTime = totalServiceTime[i] / msgProcessed[i];
            double averageResponseTime = totalResponseTime[i] / msgProcessed[i]; // Compute average response time

            recordScalar(("Average Waiting Time Source " + std::to_string(i)).c_str(), averageWaitingTime);
            recordScalar(("Average Service Time Source " + std::to_string(i)).c_str(), averageServiceTime);
            recordScalar(("Average Response Time Source " + std::to_string(i)).c_str(), averageResponseTime); // Record average response time
        }
    }

    if (checkCounts > 0) {
        // Compute average resource usage
        double avgResourceUsage = sumOfOccupiedResource / checkCounts;

        // Compute percentage utilization
        double initialResourceCapacity = par("ResourceCapacity").intValue();

        double avgResourceUtilization = (avgResourceUsage / initialResourceCapacity) * 100.0;

        // Record the average utilizations
        recordScalar("Resource Utilization (%)", avgResourceUtilization);

        // Log the final averages for quick visual confirmation
        EV << "Final Sum of Resource Usage: " << sumOfOccupiedResource << "\n";
        EV << "Total Number of Checks: " << checkCounts << "\n";

        EV << "Average Resource Usage: " << avgResourceUsage << " (" << avgResourceUtilization << "%)\n";
    }

    for (int i = 0; i < msgsInServiceCount.size(); ++i) {
        if (checkCounts > 0) {
            avgMsgsInService[i] = static_cast<double>(msgsInServiceCount[i]) / static_cast<double>(checkCounts);
            recordScalar(("source" + std::to_string(i) + " Average Messages In Service").c_str(), avgMsgsInService[i]);
        }
    }

    for (int i = 0; i < 2; ++i) {
        if (checkCounts > 0) {
            avgMsgsInBuffer[i] = static_cast<double>(msgsInBufferCount[i]) / static_cast<double>(checkCounts);
            recordScalar(("source" + std::to_string(i) + " Average Messages In Buffer").c_str(), avgMsgsInBuffer[i]);
        }
    }

    for (int i = 0; i < 2; ++i) {
        if (msgProcessed[i] > 0) {
            double avgServiceTime = totalServiceTime[i] / msgProcessed[i];
            recordScalar(("source" + std::to_string(i) + " AvgServiceTime").c_str(), avgServiceTime);
        }
    }

    for (int i = 0; i < 2; i++) {
        std::string sourceId = "source" + std::to_string(i);
        recordScalar((sourceId + " Messages Processed").c_str(), msgProcessed[i]);
        recordScalar((sourceId + " Messages Dropped").c_str(), msgDropped[i]);
    }

    EV << "Simulation finished. Processed and dropped message statistics per source have been recorded.\n";
}
}
