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




simple Processor
{
    parameters:
        @display("i=block/activeq;q=queue");
        // Example dynamic signal/statistic configuration (consider handling in C++)
        double checkInterval @unit(s) = default(0.5s);        
        int bufferSize = default(10); // The maximum number of messages the FIFO can hold
        int ResourceCapacity = default(20); // The total resource capacity of the FIFO

        string schedulingPolicy = default("FIFO"); // Add this line
        
        @signal[msgDropped](type="long");
        @statistic[msgDropped](title="messages dropped"; source="msgDropped"; record=vector; interpolationmode=none);
    gates:
        input in[2]; // Adjusted for 29 sources
        output out;
}
