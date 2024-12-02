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

package networksproject;

//
// TODO auto-generated module
//
simple Node
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
        double TD = default(1.0);
        // Error Delay
        double ED = default(4.0);
        // Duplication Delay
        double DD = default(0.1);
        // Loss Probability
        double LP = default(0);
    gates:
        input in;
        output out;
}
