import pandas as pd
import numpy as np
import glob


def parse_log_files(log_files):
    aggregated_data = pd.DataFrame()

    for file in log_files:
        df = pd.read_csv(file)

        df['entries'] = df['entries'].astype(float)
        df['send_time'] = pd.to_datetime(df['send_time'], unit='ns')
        df['return_time'] = pd.to_datetime(df['return_time'], unit='ns')

        df['time_difference'] = (df['return_time'] - df['send_time']).dt.total_seconds()


        # sum_time = 0 
        # df_list = []
        # old_index = 0
        # for index, i in enumerate(df['time_difference']):
        #     sum_time += df['time_difference']
        #     if sum_time > buffer_span:
        #         df_list.append(df[old_index:index+1])
        #         old_index = index + 1
        #         sum_time = 0
        # df_list.append(df[old_index:])

        ##  queue	num_clients	throughput(ops/ms)	latency(ns/op)	latency_deviation	

        avg_time_diff = df['time_difference'].mean()
        total_items = df['entries'].iloc[-1]
        # percentile_90 = np.percentile(df['time_difference'], 90)
        # percentile_95 = np.percentile(df['time_difference'], 95)
        # percentile_99 = np.percentile(df['time_difference'], 99)

        # Append aggregated data to the DataFrame
        row = pd.DataFrame({
            'log_file': [file],
            'num_clients': [total_items],
            'latency(sec)': [avg_time_diff],
            'throughput(sec/op)': [total_items/avg_time_diff]
            # '90th_percentile': [percentile_90],
            # '95th_percentile': [percentile_95],
            # '99th_percentile': [percentile_99]
        })
        
        aggregated_data = pd.concat([aggregated_data,row])
        

    return aggregated_data


# log_files_path = '/home/estebanramos/projects/notnets_shm/experiments/bin/expData'

# Path to log files
log_files_path = '../bin/expData/**.dat'

# Get a list of all log files
log_files = glob.glob(log_files_path)

# Parse log files and aggregate data
aggregated_data = parse_log_files(log_files)

# Print aggregated data
print(aggregated_data)
