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
