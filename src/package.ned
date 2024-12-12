package networksproject;

import ned.DelayChannel;
import ned.IdealChannel;


@license(LGPL);
//
// TODO documentation
//
network Network
{
    parameters:
        // Window Size
        int WS = default(4);
        // Sequence Number
        int SN = default(7);
        // Timeout interval
        int TO = default(10);
        // Processing Time
        double PT = default(0.5);
        // Transmission Delay
        double TD @unit(s) @unit(s) = default(1.0s);
        // Error Delay
        double ED = default(6.0);
        // Duplication Delay
        double DD = default(0.1);
        // Loss Probability
        double LP = default(0);
        @display("bgb=831,386");
    submodules:
        Node_0: Node {
            @display("p=152.3875,146.125");
        }
        Node_1: Node {
            @display("p=581.3688,150.3");
        }
    connections:
        Node_0.out --> DelayChannel {  delay = parent.TD; } --> Node_1.in;
        Node_1.out --> DelayChannel {  delay = parent.TD; } --> Node_0.in;
}

