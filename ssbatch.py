import subprocess, re
from datetime import datetime

compute_node = ['ADMINQ']
class strace():
    def __init__(self):
        self.start_time_day = 0
        self.start_time_hour = 0
        self.job_id = ''
        self.job_submit_user = ''
        self.reason = ''
        self.dependency = False
        
    def dependency_check(self):
        output = subprocess.getoutput("scontrol show job {} | grep Dependency".format(self.job_id))
        start_index = output.find("Dependency=")
        end_index = output.find("\n", start_index)
        dependency_value = output[start_index + len("Dependency="):end_index]
        dependency_value = dependency_value.split('(')
        if len(dependency_value) == 1:
            self.dependency = False
            return
        else:
            self.dependency = True
            return
        
    def get_heighest_job_time(self):
        job_id = subprocess.getoutput('sprio --sort=-y -o "%i" | sed -n "2p"')
        #user regular expression to get the job id
        if job_id is None:
            print("Without Job waiting in squeue")
            job_time = ""
            self.reason = "Without Job waiting in squeue"
            self.job_id = ""
            self.job_submit_user = ""
            return job_time
        self.job_id = job_id
        self.dependency_check()
        if self.dependency is True:
            # find the dependency job start time
            parent_start_time = subprocess.getoutput('squeue -j {} --format \"%S\" | sed -n "2p"'.format(job_id))
            self.reason = subprocess.getoutput('squeue -j {} --format \"%r\" | sed -n "2p"'.format(job_id))
            self.job_submit_user = subprocess.getoutput('squeue -j {} --format \"%u\" | sed -n "2p"'.format(job_id))
            job_time = datetime.strptime(parent_start_time, "%Y-%m-%dT%H:%M:%S")
            return job_time
        else:
            excepted_end_time = subprocess.getoutput('squeue -j {} --format \"%V\" | sed -n "2p"'.format(job_id))
            self.reason = subprocess.getoutput('squeue -j {} --format \"%r\" | sed -n "2p"'.format(job_id))
            self.job_submit_user = subprocess.getoutput('squeue -j {} --format \"%u\" | sed -n "2p"'.format(job_id))
            job_time = datetime.strptime(excepted_end_time, "%Y-%m-%dT%H:%M:%S")
            return job_time
        # print all self variables
        
    def compare_time(self, job_time):
        now = datetime.now()
        if (now-job_time).total_seconds() >= 259200:
            return True
        return False

                
    def chage_queue_algorithm_fifo(self):
        # slurm chage queue algorithm from backfill to fifo
        with open('/etc/slurm-llnl/slurm.conf', 'r') as f:
            lines = f.readlines()
        
        for i, line in enumerate(lines):
            if line.startswith('SchedulerType'):
                if lines[i] == 'SchedulerType=sched/builtin\n':
                    print('already fifo')
                    return
                lines[i] = 'SchedulerType=sched/builtin\n'
                break
        with open('/etc/slurm-llnl/slurm.conf', 'w') as f:
            f.writelines(lines)
        print('change queue algorithm to fifo')
        # change queue algorithm to fifo in controller and compute node
        for ip in compute_node:
            subprocess.getoutput('srun --pty --partition={} python3 /etc/slurm-llnl/chage_fifo.py'.format(ip))
        subprocess.getoutput('systemctl restart slurmctld')
        
    def chage_queue_algorithm_backfill(self):
        with open('/etc/slurm-llnl/slurm.conf', 'r') as f:
            lines = f.readlines()
        
        for i, line in enumerate(lines):
            if line.startswith('SchedulerType'):
                if lines[i] == 'SchedulerType=sched/backfill\n':
                    print('already backfill')
                    return
                lines[i] = 'SchedulerType=sched/backfill\n'
                break
        with open('/etc/slurm-llnl/slurm.conf', 'w') as f:
            f.writelines(lines)
        print('change queue algorithm to backfill')
        # change queue algorithm to fifo in controller and compute node
        for ip in compute_node:
            subprocess.getoutput('srun --pty --partition={} python3 /etc/slurm-llnl/chage_backfill.py'.format(ip))
        subprocess.getoutput('systemctl restart slurmctld')
        
print(datetime.now())       
testInstance = strace()
job_time = testInstance.get_heighest_job_time()

longer = False
if job_time is not None:
    longer = testInstance.compare_time(job_time)
print("Three days longer:",longer ,"\tQueue reson:",testInstance.reason,"\tSubmit time:", job_time, "\tJob ID:", testInstance.job_id, "\tUser:", testInstance.job_submit_user)
if longer and (testInstance.reason == "Resources") is True:
    testInstance.chage_queue_algorithm_fifo()
else:
    testInstance.chage_queue_algorithm_backfill()
