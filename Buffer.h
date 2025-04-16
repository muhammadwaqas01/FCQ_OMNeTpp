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
