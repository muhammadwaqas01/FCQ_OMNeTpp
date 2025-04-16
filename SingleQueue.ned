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



network SingleQueue
{
    submodules:
        // Declare 29 GenericSource submodules
        source[2]: GenericSource {
            @display("p=68,195"); // Position each source vertically spaced
        }
        processor: Processor {
            @display("p=238,170");
        }
        sink: Sink {
            @display("p=300,100");
        }
    connections:
        for i=0..1 {
            source[i].out --> processor.in[i]; // Connect each GenericSource to a distinct input gate of Fifo
        }
        processor.out --> sink.in; // Connect Fifo to Sink
}
