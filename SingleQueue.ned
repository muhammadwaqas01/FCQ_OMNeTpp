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
