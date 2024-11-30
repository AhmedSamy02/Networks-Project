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
        node0: Node {
            @display("p=210,116");
        }
        node1: Node {
            @display("p=422,116");
        }
    connections:
        node0.out --> IdealChannel --> node1.in;
        node1.out --> IdealChannel --> node0.in;
}

