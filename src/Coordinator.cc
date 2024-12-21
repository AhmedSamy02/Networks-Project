//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "Coordinator.h"
#include "file_reader.h"
#include "MyMessage_m.h"
Define_Module(Coordinator);

bool node_to_start;
void Coordinator::initialize()
{
    auto p = coordinator_file_read();
    // first = which node
    // second = time to start
    auto node_to_start = p.first;
    //    auto time_to_start = p.second - 1 >= 0 ? p.second - 1 : 0;
    auto m = new MyMessage_Base();
    m->setPayload("Start sending");
    this->sendDelayed(m, p.second, ("out" + std::to_string(node_to_start)).c_str());
}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
