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




simple GenericSource
{
    parameters:
        string sourceId;  // Add this parameter for unique source identification
        volatile double interarrivalTime @unit(s);
        volatile double serviceTime @unit(s);
        int requiredResource = default(10);  // Default value, can be overridden
        @display("i=block/source");
        @signal[msgGenerated](type="long");
        // Adjusted to use a static signal name for simplicity
        @statistic[msgGenerated](title="messages generated"; source="msgGenerated"; record=count);
    gates:
        output out;
}
