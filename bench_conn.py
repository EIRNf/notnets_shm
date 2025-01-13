import os
import subprocess
import csv

import matplotlib.pyplot as plt
import pandas as pd

lock_types = ['PTHREAD', 'BAKERY', 'BURNS_AND_LYNCH', 'PTHREAD_SPIN']    


def run_benchmark(lock_type, start_num_clients, end_num_clients):
    results = []
        
    cmd = ['./bench_rpc/bin/bench_rpc', lock_type, str(start_num_clients), str(end_num_clients)]
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
        
    fig, ax = plt.subplots()
    for lock_type, data in df.groupby('lock_type'):
        
        ax.plot(data['num_clients'], data['p99_ns'], label=lock_types[lock_type])
        # ax.plot(data['ops_s'], data['average_ns'], label=lock_types[lock_type])

        
    
    # ax.set_yscale('log')
    plt.legend([ lock_types[0], lock_types[1], lock_types[2], lock_types[3]],fontsize="small")
    
    plt.xlabel('Number of Clients')
    plt.ylabel('Avg Client Latency (ns)')
    

    plt.title('Latency vs Number of Clients')
        
    plt.tight_layout()
    plt.savefig("latency_vs_num_clients.png")


if __name__ == "__main__":
    # Clear previous results
    if os.path.exists('benchmark_results.csv'):
        os.remove('benchmark_results.csv')
    
    for lock_type in lock_types:
        for num_clients in range(1, 65, 10):
            results = run_benchmark(lock_type, num_clients, num_clients + 10)
            append_results_to_csv(results, 'benchmark_results.csv')
    plot_graphs(results)