# HungryPhilosopher
This program is an implementation of the Hungry Philosopher problem and is implemented 
with multi-threading and semiphores. The idea is that there are an X number of philosophers 
and they are in either 2 states Hungry or thinking. They all have a bowl of rice and will 
be looking to eat as soon as they get a chance. However, their is a limited number of 
chopsticks and they must share them to make sure that they all don't starve to death. 
The objective of this program is to force the philosophers(each one is an independent thread) 
to share the chopsticks and time their eating.

Run the program:
1) download the repository and make sure that it is unziped. 
2) Run your linux command terminal and cd into the directroy of the file.
3) run the make command
4) run the excutible file with the ./run command
