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





This project is licensed under the GPLv3 - see the LICENSE file for details

Fifo
====

Single-server queue model.

Demonstrates:
  - subclassing simple modules
  - using handleMessage()
  - decomposing handleMessage() into several functions
  - using simple statistics and output vectors
  - using finish()

The model contains three modules. The "gen" module generates jobs, and
sends them to the "fifo" module which is a single-server queue. Jobs are
stored in a queue (cQueue object) until they are served -- this queue can be
found and inspected in the graphical environment among the class members of
the "fifo" module -- either in the object tree in the main window, or in the
inspector of the "fifo" module (right-click "fifo" icon --> Inspect as object
--> click "Contents" tab). Processed jobs are disposed of in the "sink"
module. The "sink" module collects statistics which can also be inspected.

After running the simulation, fifo*.vec file will contain queueing time
data collected via cOutVector objects during the simulation. The data
can be plotted using the Analysis Tool in the IDE.

AbstractFifo can be used as a base class for modules that involve queueing.
One can subclass AbstractFifo and redefine the following member functions:
    void arrival(cMessage *msg)
    simtime_t startService(cMessage *msg)
    void endService(cMessage *msg)
These functions are called by AbstractFifo::handleMessage() whenever
a message arrives, begins service and ends service, respectively.
startService() should return the service time for the message.
ACBFifo and ACPFifo demonstrates how to do this.


