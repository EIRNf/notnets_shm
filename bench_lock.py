import os
import subprocess
import csv

import matplotlib.pyplot as plt
import pandas as pd

lock_types = ['PTHREAD', 'BAKERY', 'BURNS_AND_LYNCH']    


def run_benchmark(lock_type, start_num_clients, end_num_clients, num_runs):
    results = []
        
    cmd = ['./bench_mutex/bin/bench_mutex', lock_type, str(start_num_clients), str(end_num_clients), str(num_runs)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    output = result.stdout.strip()
    rows = output.split('\n')
    for row in rows:
        # print(row)
        columns = row.split(',')
        # print(columns)
        results.append({
            'lock_type': columns[0],
            'num_clients': int(columns[1]),
            'total_ms': float(columns[2]),
            'total_ns': float(columns[3]),
            'average_ns': float(columns[4]),
            'p99_ns': float(columns[5]),
            'ops_s': float(columns[6])
            # 'u_time': float(columns[7]),
            # 'k_time': float(columns[8]),
        })

    return results

def save_results_to_csv(results, filename):
    keys = results[0].keys()
    with open(filename, 'w', newline='') as output_file:
        dict_writer = csv.DictWriter(output_file, fieldnames=keys)
        dict_writer.writeheader()
        dict_writer.writerows(results)
        
def append_results_to_csv(results, filename):
    keys = results[0].keys()
    
    if not os.path.exists(filename):
        save_results_to_csv(results, filename)
        return
    
    with open(filename, 'a', newline='') as output_file:
        dict_writer = csv.DictWriter(output_file, fieldnames=keys)
        dict_writer.writerows(results)

def plot_graphs(results):
    
    df = pd.read_csv('benchmark_results.csv')
    # print(df)
    
    
    new_df = df.groupby(['lock_type', 'num_clients']).mean()
    fig, ax = plt.subplots()

    for lock_type in range(0 , len(lock_types)):
        lock_df = new_df.xs(lock_type, level='lock_type')
        ax.plot(lock_df.index, lock_df['average_ns'], label=lock_type)
    # print(new_df)

        
    # for data in new_df:
    #     ax.plot(data['num_clients'], data['average_ns'], label=lock_types[lock_type])
        
            
    ax.set_yscale('log')
    plt.legend([ lock_types[0], lock_types[1], lock_types[2]],fontsize="small")
    
    plt.xlabel('num threads (100k ops per thread)')
    plt.ylabel('avg latency (ns)')
    
    plt.title('lock/unlock latency with increasing contention')
        
    plt.tight_layout()
    plt.savefig("latency_vs_num_clients.png")


if __name__ == "__main__":
    # # Clear previous results
    # if os.path.exists('benchmark_results.csv'):
    #     os.remove('benchmark_results.csv')
    
    # for lock_type in lock_types:
    #     results = run_benchmark(lock_type, 1, 30, 10)
    #     append_results_to_csv(results, 'benchmark_results.csv')
            
            
    results = pd.read_csv('benchmark_results.csv')
    plot_graphs(results)