# Networks Project  <img align= center width=60px height=60px src="https://github.com/user-attachments/assets/83851901-3919-4bf1-add3-c580b84a1234">
Data Link Layer Protocols Simulation

## <img align= center width=50px height=50px src="https://github.com/AhmedSamy02/Adders-Mania/assets/88517271/dba75e61-02dd-465b-bc31-90907f36c93a"> Table of Contents
- [Overview](#overview)
- [Technologies Used](#tech)
- [Key Features](#feat)
- [Input File Format](#in)
- [How To Run](#run)
- [Output Logs](#out)
- [Contributors](#contributors)

## <img src="https://github.com/AhmedSamy02/Adders-Mania/assets/88517271/9ed3ee67-0407-4c82-9e29-4faa76d1ac44" width="50" height="50" /> Overview <a name = "overview"></a>
This project simulates **Selective Repeat ARQ** with **Byte Stuffing** and **CRC error detection** between two nodes (`Node0` and `Node1`) connected via a noisy channel—the simulation models real-world network issues like packet corruption, loss, duplication, and delay.  

## <img src="https://github.com/user-attachments/assets/b48e8f26-0cd6-47d8-8ba8-1553c97927b5" width="50" height="50" /> Technologies Used  <a name = "tech"></a>
- **C++** (Core logic)  
- **OMNeT++** (Network simulation framework)  

## <img src="https://github.com/user-attachments/assets/97bcb0d9-e3e8-41b1-8bf3-f334b7b2b6af" width="50" height="50" />Key Features <a name = "feat"></a>
✅ Selective Repeat Protocol  
✅ Noisy Channel Simulation (Modification, Loss, Duplication, Delay)  
✅ Byte Stuffing Framing (Flag=`$`, Escape=`/`)  
✅ CRC-8 Error Detection  
✅ Dynamic Configuration (Window Size, Timeout, Delays)  
✅ Detailed Logging


## <img src = "https://github.com/user-attachments/assets/eddc7a43-065f-406d-b8ff-a1cebc4c7b33" width="50" height="50" />Input File Format <a name = "in"></a>
### Node Input (e.g., `input0.txt`)  
Each line: `[4-bit error code] [message]`  
- **Error Code**: `[Modification, Loss, Duplication, Delay]` (e.g., `1010` = Modify + Duplicate).
- Example:

  ```
  1000 Hi2
  ```

### Coordinator Input (`coordinator.txt`)  
- **Format**: `[NodeID] [StartTime]`
- Example:

  ```
  1 2.5  # Node1 starts at t=2.5s
  ```


## <img src="https://github.com/YaraHisham61/OS_Scheduler/assets/88517271/1c40c081-3619-449b-a9d7-605fc7b2baca" width="30" height="30" />  How To Run <a name = "run"></a>
1. **Clone the repo**:  
   ```bash
   git clone https://github.com/AhmedSamy02/Networks-Project.git
   ```
2. **Open in OMNeT++ IDE**:  
   - Import the project.  
   - Build the project (Ctrl+B).  

3. **Configure Inputs**:  
   - Edit `input0.txt`, `input1.txt`, and `coordinator.txt` in `src/input_texts`.  

4. **Run Simulation**:  
   - Execute `omnetpp.ini` in OMNeT++ (Right-click → Run As → OMNeT++ Simulation).  

5. **Check Outputs**:  
   - Logs are saved in `src/output_files/output.txt`.
  

## <img src = "https://github.com/user-attachments/assets/d86ffcbd-7861-4383-98d0-bb0b3cd59203" width="50" height="50" /> Output Logs <a name = "out"></a>
The simulation logs all events to `output.txt`, including:
- Message processing and transmission times.
- Error introductions (modification, loss, duplication, delay).
- Timeout events and retransmissions.
- ACK/NACK transmissions and losses.
- Payloads uploaded to the network layer.


## <img src="https://github.com/YaraHisham61/OS_Scheduler/assets/88517271/859c6d0a-d951-4135-b420-6ca35c403803" width="50" height="50" /> Contributors <a name = "contributors"></a>
<table>
  <tr>
   <td align="center">
    <a href="https://github.com/AhmedSamy02" target="_black">
    <img src="https://avatars.githubusercontent.com/u/96637750?v=4" width="150px;" alt="Ahmed Samy"/>
    <br />
    <sub><b>Ahmed Samy</b></sub></a>
    </td>
   <td align="center">
    <a href="https://github.com/kaokab33" target="_black">
    <img src="https://avatars.githubusercontent.com/u/93781327?v=4" width="150px;" alt="Kareem Samy"/>
    <br />
    <sub><b>Kareem Samy</b></sub></a>
    </td>
   <td align="center">
    <a href="https://github.com/nancyalgazzar" target="_black">
    <img src="https://avatars.githubusercontent.com/u/94644017?v=4" width="150px;" alt="Nancy Ayman"/>
    <br />
    <sub><b>Nancy Ayman</b></sub></a>
    </td>
   <td align="center">
    <a href="https://github.com/YaraHisham61" target="_black">
    <img src="https://avatars.githubusercontent.com/u/88517271?v=4" width="150px;" alt="Yara Hisham"/>
    <br />
    <sub><b>Yara Hisham</b></sub></a>
    </td>
  </tr>
 </table>
