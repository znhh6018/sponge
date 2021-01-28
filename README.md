2021.1.28,finished all assignments. 

It takes a month and is worth it.I learned the bottom of computer network,how the TCP works.e.g. the receive part,the send part,three-way handshake,four-way wavehand,under which condition should we linger after the connection is done and send RST to terminate connection.Below the TCP layer,how the data is transmited between the IP layer and LINK layer,in this part serialize,parse and ARP protocol are involved.Finally we are expected a router table with longest-prefix match with O(N) time complexity, i made it with O(1) time complexity.

For all labs,something is descripted in the handout and you need to figure out more details by test case,there is bug with GDB,use LLDB instead,most of the commands are the same.

There is still something to do with the optimization,improve the write speed and read speed.Maybe later....

lab 1:
	The receive part.A reassembler for reassemble the strings you received. They may be out of order ,longer than the capacity or duplicate, or other circumstances you may not think of, e.g. an empty substring with eof.
	
lab 2:
	The receive part.I put the most receive logic in lab1,so this part didn't take too much time and codes.
	
lab 3:
	The send part.On one hand ,We need to keep writing to the window unless these is no more space or nothing to write.On the other hand,we need to determine ack number and windowSize of the peer by acknowledge segments we received.We also need to retransmit the seghment,increase the retransmition_count and double the retransmition_timeout,but if the windowSize of the peer is zero,we don't need to do the latter two.You can think why?I think if retransmittions occured while the peer's windowSize is zero,that means there is nothing to do with the network congestion,so we don't need to do that.
	
lab 4:
	Put receive part and send part together.three-way handshake and four-way wavehand and all the state transmittion are involved.Under which circumstances should we linger or not,I think linger is the most difficult part in this lab,you really need to figure out why.
	
lab 5:
	Below the TCP protocol,the IP layer and LINK layer.I spent a lot of time reading the serialize code and parse code.But in fact,you could finish without reading that.You need to store the mapping between the ip and ethernet.store the ip validity period,send a ARP segment if the IP is not stored.
	
lab 6:
	Implement a router table.The time complexity is not required,you can finish it in O(N),but we could finish this in O(1).At first,I wanted to do it with trie tree,but don't know how to deal with the prefix_length,e.g. 192.168.1.1/8,how do we take consideration of 8 into the trie tree.So I think of an alternative solution,also O(1) time complexicity. You could see it in router.hh,routeer.cc.
	
