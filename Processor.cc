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
    } else if (policyName == "MostServerFit") {
        policy = new MostServerFitQueuePolicy();
    } else {
        policy = new FIFOQueuePolicy(); // Default to FIFO if no valid policy is specified
    }
    buffer = new Buffer(bufferSize, policy);

    CPUCapacity = par("CPUCapacity").intValue();
    MemoryCapacity = par("MemoryCapacity").intValue();
    BandwidthCapacity = par("BandwidthCapacity").intValue();
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

    msgsInBufferCount.resize(2, 0LL); // Initialize the buffer count vector
    avgMsgsInBuffer.resize(2, 0.0); // Initialize the average buffer vector



    registerDynamicSignals();

    scheduleAt(simTime() + checkInterval, new cMessage("checkResource"));

    EV << "Initialize: Queue system initialized with CPUCapacity=" << CPUCapacity
       << ", MemoryCapacity=" << MemoryCapacity
       << ", BandwidthCapacity=" << BandwidthCapacity
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
               << ", Req. CPU: " << static_cast<int>(job->par("requiredCPU").longValue())
               << ", Req. Memory: " << static_cast<int>(job->par("requiredMemory").longValue())
               << ", Req. Bandwidth: " << static_cast<int>(job->par("requiredBandwidth").longValue()) << "\n";
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
           << ", Req. CPU: " << static_cast<int>(job->par("requiredCPU").longValue())
           << ", Req. Memory: " << static_cast<int>(job->par("requiredMemory").longValue())
           << ", Req. Bandwidth: " << static_cast<int>(job->par("requiredBandwidth").longValue())
           << ", Remaining Service Time: " << remainingServiceTime << "\n"; // Log the remaining service time
    }
}

void Processor::handleResourceCheck() { //Resource usage of active jobs
//    cumulativePacketsInProgress += activeJobs.size();
    checkCounts++;

    // Calculate current resource usage
    long currentCPUUsage = sumOfCPUUsedByActiveJobs();
    long currentMemoryUsage = sumOfMemoryUsedByActiveJobs();
    long currentBandwidthUsage = sumOfBandwidthUsedByActiveJobs();

    // Accumulate the resource usage
    sumOfOccupiedCPU += currentCPUUsage;
    sumOfOccupiedMemory += currentMemoryUsage;
    sumOfOccupiedBandwidth += currentBandwidthUsage;

    // Log the starting point of resource check
    EV << "Resource check at time: " << simTime() << " with " << activeJobs.size() << " active jobs.\n";
    EV << "Current CPU Usage: " << currentCPUUsage << ", Total: " << sumOfOccupiedCPU << "\n";
    EV << "Current Memory Usage: " << currentMemoryUsage << ", Total: " << sumOfOccupiedMemory << "\n";
    EV << "Current Bandwidth Usage: " << currentBandwidthUsage << ", Total: " << sumOfOccupiedBandwidth << "\n";

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
        msgsInBufferCount[i] += static_cast<long long>(bufferCounts[i]);        EV << "Total messages from source" << i << " in buffer until now: " << msgsInBufferCount[i] << "\n";
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
        cMessage* nextJob = buffer->popNextMessage(CPUCapacity, MemoryCapacity, BandwidthCapacity);
        if (nextJob) {
            // Now nextJob is declared and can be used
            long requiredCPU = nextJob->par("requiredCPU").longValue();
            long requiredMemory = nextJob->par("requiredMemory").longValue();
            long requiredBandwidth = nextJob->par("requiredBandwidth").longValue();

            // Ensure the conditions are met to start the job
            if (requiredCPU <= CPUCapacity && requiredMemory <= MemoryCapacity && requiredBandwidth <= BandwidthCapacity) {
                startNextJob(nextJob);             }
        }
    }
}

long Processor::sumOfCPUUsedByActiveJobs() {
    long totalCPUUsed = 0;

    for (const auto& job : activeJobs) {
        totalCPUUsed += static_cast<int>(job->par("requiredCPU").longValue());


    }
    return totalCPUUsed;

}
long Processor::sumOfMemoryUsedByActiveJobs() {
    long totalMemoryUsed = 0;

    for (const auto& job : activeJobs) {
        totalMemoryUsed += static_cast<int>(job->par("requiredMemory").longValue());


    }
    return totalMemoryUsed;

}
long Processor::sumOfBandwidthUsedByActiveJobs() {
    long totalbandwidthUsed = 0;

    for (const auto& job : activeJobs) {
        totalbandwidthUsed += static_cast<int>(job->par("requiredBandwidth").longValue());


    }
    return totalbandwidthUsed;

}

bool Processor::canStartNextJob() {
    // Get the next job, but first, make sure to pass the available CPU resources
    cMessage* nextJob = buffer->peekNextMessage(CPUCapacity); // Pass CPUCapacity as the available CPU
    if (!nextJob) return false;

    long requiredCPU = static_cast<long>(nextJob->par("requiredCPU").longValue());
    long requiredMemory = static_cast<long>(nextJob->par("requiredMemory").longValue());
    long requiredBandwidth = static_cast<long>(nextJob->par("requiredBandwidth").longValue());

    return requiredCPU <= CPUCapacity &&
           requiredMemory <= MemoryCapacity &&
           requiredBandwidth <= BandwidthCapacity;
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
    long requiredCPU = static_cast<long>(job->par("requiredCPU").longValue());
    long requiredMemory = static_cast<long>(job->par("requiredMemory").longValue());
    long requiredBandwidth = static_cast<long>(job->par("requiredBandwidth").longValue());

    CPUCapacity -= requiredCPU; // Update the available resource capacity.
    MemoryCapacity -= requiredMemory; // Update the available resource capacity.
    BandwidthCapacity -= requiredBandwidth; // Update the available resource capacity.

    activeJobs.push_back(job); // Add the job to the list of active jobs.

    // Assuming the source ID is stored in a parameter named "sourceID".
    if (!job->hasPar("origin")) {
        job->addPar("origin").setStringValue(sourceId.c_str());
    }

    EV << "Resource Update: Job started: ID=" << job->getId()
       << ", SourceID=" << sourceId
       << ", ConsumedCPU=" << requiredCPU
       << ", ConsumedMemory=" << requiredMemory
       << ", ConsumedBandwidth=" << requiredBandwidth
       << ", RemainingCPU=" << CPUCapacity
       << ", RemainingMemory=" << MemoryCapacity
       << ", RemainingBandwidth=" << BandwidthCapacity << ".\n";

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
    int releasedCPU = static_cast<int>(msg->par("requiredCPU").longValue());
    int releasedMemory = static_cast<int>(msg->par("requiredMemory").longValue());
    int releasedBandwidth = static_cast<int>(msg->par("requiredBandwidth").longValue());

    CPUCapacity += releasedCPU;
    MemoryCapacity += releasedMemory;
    BandwidthCapacity += releasedBandwidth;

    EV << "Releasing resources: Job ID=" << msg->getId()
       << ", Source ID=" << sourceId
       << ", releasedCPU=" << releasedCPU
       << ", releasedMemory=" << releasedMemory
       << ", releasedBandwidth=" << releasedBandwidth
       << ", NewTotalCPU=" << CPUCapacity
       << ", NewTotalMemory=" << MemoryCapacity
       << ", NewTotalBandwidth=" << BandwidthCapacity << ".\n";

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
//        double avgCPUUsage = sumOfOccupiedCPU / checkCounts;
//        double avgMemoryUsage = sumOfOccupiedMemory / checkCounts;
//        double avgBandwidthUsage = sumOfOccupiedBandwidth / checkCounts;

        double avgCPUUsage = static_cast<double>(sumOfOccupiedCPU) / checkCounts;
        double avgMemoryUsage = static_cast<double>(sumOfOccupiedMemory) / checkCounts;
        double avgBandwidthUsage = static_cast<double>(sumOfOccupiedBandwidth) / checkCounts;


        // Compute percentage utilization
        double initialCPUCapacity = par("CPUCapacity").intValue();
        double initialMemoryCapacity = par("MemoryCapacity").intValue();
        double initialBandwidthCapacity = par("BandwidthCapacity").intValue();

        double avgCPUUtilization = (avgCPUUsage / initialCPUCapacity) * 100.0;
        double avgMemoryUtilization = (avgMemoryUsage / initialMemoryCapacity) * 100.0;
        double avgBandwidthUtilization = (avgBandwidthUsage / initialBandwidthCapacity) * 100.0;
        EV << "Detailed CPU Utilization: " << (avgCPUUsage / initialCPUCapacity) << "\n";

        // Record the average utilizations
        recordScalar("CPU Utilization (%)", avgCPUUtilization);
        recordScalar("Memory Utilization (%)", avgMemoryUtilization);
        recordScalar("Bandwidth Utilization (%)", avgBandwidthUtilization);

        // Log the final averages for quick visual confirmation
        EV << "Final Sum of CPU Usage: " << sumOfOccupiedCPU << "\n";
        EV << "Final Sum of Memory Usage: " << sumOfOccupiedMemory << "\n";
        EV << "Final Sum of Bandwidth Usage: " << sumOfOccupiedBandwidth << "\n";
        EV << "Total Number of Checks: " << checkCounts << "\n";

        EV << "Average CPU Usage: " << avgCPUUsage << " (" << avgCPUUtilization << "%)\n";
        EV << "Average Memory Usage: " << avgMemoryUsage << " (" << avgMemoryUtilization << "%)\n";
        EV << "Average Bandwidth Usage: " << avgBandwidthUsage << " (" << avgBandwidthUtilization << "%)\n";
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



    for (int i = 0; i < 2; ++i) {
        // Compute throughput for source i
        double simulationTime = SIMTIME_DBL(simTime());  // Get the total simulation time
        double throughput = (simulationTime > 0) ? (msgProcessed[i] / simulationTime) : 0.0;
        recordScalar(("Throughput Source " + std::to_string(i)).c_str(), throughput);

        // Compute total jobs generated by source i
        long totalGenerated = msgProcessed[i] + msgDropped[i];

        // Compute dropping probability for source i
        double droppingProbability = (totalGenerated > 0) ? (static_cast<double>(msgDropped[i]) / totalGenerated) : 0.0;
        recordScalar(("Dropping Probability Source " + std::to_string(i)).c_str(), droppingProbability);
    }



    EV << "Simulation finished. Processed and dropped message statistics per source have been recorded.\n";
}
}
