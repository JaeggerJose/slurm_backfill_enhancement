# slurm backfill algorithm enhancement
## Abstract
When we use the backfill algorithm, sometimes the high priority job, which also requests lots of resource, will queue for a long time.
The lower priority and requested jobs will be batched in their partition means the lower jobs cut in line that make high priority job keep waiting,
thus we want to code a mini program to solve the problem.
