# WSN_TDMA_CC3200
A Wireless Sensor Network Monitored Air Conditioning System 

## Description
This repo contains code for a wireless tempreture monitoring system consisting of [TI CC3200](https://www.ti.com/product/CC3200) nodes.
While the code for both sensing and sink nodes does not inherently limit the number of nodes in the network the protocols implemented were tested with a total of six nodes.

## Testing Environment
Validation was performed using six nodes, five of which are sensing nodes and one was used as a sink node for receiving data from all sensing nodes. 
The sink node collects and aggregates the data sent from the nodes for monitoring purposes. The system is to be designed to minimize collision, conserve power and adapt to any change in the topology if a new node is introduced or an existing node fail. The design proposed adopts a simple MAC protocol and a two level hierarchical IP protocol to coordinate communication between nodes. Testing was done in a typical university enviornment with a large student body and universal wifi access on campus. 

## Protocols
### MAC Protocol: 

A simple MAC Protocol is adopted for the application. 
The MAC header consists of the destination and source mac. 
In addition, a CRC field is added to the packet to verify its validity at the recieved. 
Once a packet is validated by a node via the CRC field the destination MAC address of the packet is checked. 
In case the destination MAC matches the MAC of the node that recieved the afformentioned packet or if the destination MAC is the predefined broadcast MAC (0xFF,0xFF,0xFF,0xFF,0xFF,0xFF) an acknowledgement packet is sent by reciever node to the sender of the packet. 
The sender node will attempt retransmission if a an acknowledgement is not received during a predefined period. After a configurable number of retries the sender aborts transmission if no acknowledge is received. 

### IP Protocol: 

A conventional IP scheme is adopted in this system. 
A subnet mask of 0.0.0.255 is used. 
Consequently, the first 3 bytes are fixed and the last one is to distinguish the different nodes in thes system. Therefore the theoretical maximum number of sensing nodes in this scheme is 254.
The sink node has an IP of 10.10.10.1. While all nodes are assigned an IP of 10.10.10.x where x is a unique number for each node. In the case of the discarding of a node, the IP it had becomes available to be assigned for new nodes. The IP scheme allows for a maximum of 254 nodes to join the system. However, if needed the scheme can allow for using more nodes by simply changing the subnet.  

#### Adding A new node: 
Once a node is added to the system, it broadcasts a packet to be processed by all its neighbours. If the sink node receives the packet, the MAC address of the node is added in the memory of the sink and an IP address is assigned for the new node. The node receives the assigned IP from the sink through a packet with the destination MAC being the MAC of the new node. The IP address assigned in this case will have a depth byte of 1 as it connects directly to the sink. If the new node cannot communicate with the sink node, the broadcasted packet sent by this node will be read by all neighbouring nodes. Consequently, each node responds by assigning an IP address with a depth byte equal to the byte of the assigning node added to 1. The new node then picks the IP with the highest depth byte and sends and  acknowledgment back to the assigning node informing it that it shall connect to it. The fact that a new node picks the highest depth is to guarantee a small branching factor in order to minimize the power consumption. 

#### Collision Avoidance
A TDMA communicaton scheme has been adopted for this project. Sensing nodes are assigned a communication slot within a predetermined application period to avoid costly collisions. Slots are assigned based on allocated IP. Synchronization of system time is done once for each sensing node during ip request. Clock variability is taken advantage of to reduce the probability of two nodes attempting to communicate at the same time. Resynchronization is not handled and is currently a limitation of this approach. 

#### Removing an idle node: 
The sink node keeps track of nodes based on the time they last communicated with it. If nodes remain inactive for too long they become stale and that will be reflected in the last contact variable kept on the sink for that node. After a predetermined cleanup period the sink checks the last active variable for each node and if it's too far back in time it erases that node from the allocated ip list and freeze up the ip for a new node. This translates into a new free TDMA slot for future communication with a new node
