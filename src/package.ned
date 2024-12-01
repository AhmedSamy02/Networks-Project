package networksproject;

import ned.IdealChannel;


@license(LGPL);
//
// TODO documentation
//
network Network
{
    @display("bgb=831,386");
    submodules:
        Node_0: Node {
            @display("p=210,116");
        }
        Node_1: Node {
            @display("p=422,116");
        }
    connections:
        Node_0.out --> IdealChannel --> Node_1.in;
        Node_1.out --> IdealChannel --> Node_0.in;
}

