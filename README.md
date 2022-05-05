# COMP 304: Project 2 Spacecraft Control with Pthreads

## Alp Özaslan - Onur Eren Arpacı

### https://github.com/alpozaslan/comp_304_project2.git

## Part 1:

We created producer threads and queues for each job type, namely landing, launch, and assembly. We also created consumer threads and queues for PadA and PadB. Each queue has its own mutex. Lastly there is the ControlTower thread to control the flow.
Producer threads are sleeping 2 seconds, randomly creating a job or not according to their probability, enqueueing the job to their respective queue and they repeat it until the simulation ends.
Consumer queues are sleeping for the duration of the first job in their queue, then they dequeue the job and they repeat until the simulation ends.
ControlTower thread first dumps all the landing jobs to the PadAQueue and PadBQueue. It distributes them so that the pad with the smaller total duration left in their queue takes the next landing job. It then checks whether the PadA and PadB queues are empty meaning there are no landing jobs left to do, if so It puts an assembly job to the PadAQueue and a launch job to the PadBQueue. It repeats this process until the simulation ends.
There are also some helper functions:
WriteLog takes the job and the pad name, produces the log string and appends it to the log.txt
PrintQueue takes a queue and prints the ids of the jobs from start to end.
PrintCurrentQueues prints the status of all the queues one by one

## Part 2:

The suggested solution can cause starvation because after there are more than 3 ground jobs waiting, the landing jobs will lose their priority and if there are enough ground jobs incoming so that there are always some of them past their maximum wait time, the control tower will never take another landing job.
We decided to implement the following solution:
If there are less than 3 assembly jobs and less than 3 launch jobs waiting, then we use the same algorithm with part 1, otherwise the control tower pushes a single job to the pads for each job type. So it pushes a launch job to PadAQueue, an assembly job to PadBQueue and a landing job to whichever pad has the least total duration left.

## Part 3:

We created a producer thread and a queue for emergency landings, we also created two separate emergency queues for PadA and PadB. Now, the control tower first checks whether there are emergency jobs waiting, if so it gives one of them to PadA and the other one to PadB.
Pads first check if they have something in their emergency queue and do the emergency job if there is one.
