# factory
I will be implementing a producer-consumer model using counting and binary semaphores.  This program will be used to encrypt and decrypt files.  The producer and the consumers will all be threads.  For this program, I will have one producer and a variable number of consumers.  The user of the program will select the number of consumers.  When implementing a producer-consumer model, I use shared memory. The shared memory for this program will be a warehouse that can store 10 storage item.  Each storage item will hold (2 characters and their location in the file).  The warehouse should be setup using first in first out.
