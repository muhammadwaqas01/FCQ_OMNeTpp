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
            long priorityValue = currentJob->par("requiredResource").longValue();
            if (priorityValue > highestPriorityValue) {
                highestPriorityValue = priorityValue;
                jobWithHighestPriority = currentJob;
            }
        }
    }
    return jobWithHighestPriority;
}



} // namespace processor
