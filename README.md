## Information Flow and Decision-Making Simulation

This project is a modification based on UMich EECS281 Project2A. The page of original project: https://eecs281staff.github.io/p2-a-new-heap/ This project is to simulate the how the given information flow affects the decision making of multiple agents. 

### Details:

There are two opposite decisions: A and B. Multiple information sources influence multiple agents with evidence in favor of A or B. Each piece of information contains three features: type (i.e., A or B), priority (i.e., how "powerful" this piece is, which might be affected by which type of transmitter is used), and information intensity (i.e., how "many" words or figures are delivered). 

A and B have the following features: B will interfere with A only when B's priority is higher than A; when such a thing happens, we consider both pieces of information are mixed together and therefore we consider it as "losing both of them". Based on that, we have two conditions: 

1. if A first, then B, we say that retroactive interference (RI) happens
2. if B first, then A, we say that proactive interference (PI) happens

### Example output: 
<img width="1031" alt="Screen Shot 2023-03-17 at 15 09 16" src="https://user-images.githubusercontent.com/69283640/226075514-7fef940d-f382-4541-9e86-614b763c42ac.png">
