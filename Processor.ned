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
