# Kiwi Search Engine Multithreading
Welcome to the "Kiwi Search Engine Multithreading" repository. This repository contains an implementation of a multithreading data storage engine,
This project was implemented during the sixth semester, in the course MYY601 of the curriculum of the University of Ioannina. The final grade of the
project is 50 out of 100.



## Overview
This project focuses on implementing multithreading in a data storage engine. The primary goal is to enhance performance through synchronization techniques
and efficient thread management.



## Key Features
- Implementation of global locks for synchronization
- Read-Write mechanism with multiple readers and a single writer
- Multi-threaded execution via command-line interface
- Read-Write operations with adjustable write percentage
- Performance measurements and output logging



## Instructions
- Objective: Implement a multithreading model in a database storage engine.
- How to Run: Compile the program using `make all`. Run various commands: `./kiwi-bench write <operations>`,
  `./kiwi-bench read <operations>`, `./kiwi-bench readwrite <operations> <write_percentage> <threads>`
- Feedback: Logs and output files will be generated to evaluate execution performance.



## Implementation Details
- Global locking via Peterson's Algorithm for mutual exclusion.
- Mutex locks for protecting shared memory in `db_add` and `db_get`.
- Condition variables for coordinating read-write operations.
- Multi-threading implementation using `pthread`.
- `thread_maker` function dynamically assigns workload among threads.
- `pthread_create()` and `pthread_join()` ensure proper execution flow.
- Execution time tracking.
- Output logging for benchmarking results.
- Handling bus errors and read inconsistencies through locking improvements.



## Collaboration
This project was a collaborative effort. Special thanks to [SpanouMaria](https://github.com/SpanouMaria), for the significant contributions to the development
and improvement of the application.
