import datetime
import os
import pandas as pd
import numpy as np
import glob
import matplotlib.pyplot as pp
import matplotlib

import re


import CommonConf
import CommonViz

import Rtt_Experiment2Xs_Timestamp

from enum import Enum

class Queue_Type(Enum):
    POLL = '0'
    BOOST = '1'
    SEM = '2'
    TCP = 'tcp'
    
class Labeled_Queue_Type(Enum):
    POLL = 'POLL'
    BOOST = 'BOOST'
    SEM = 'SEM'
    TCP = 'TCP'

def parse_log_files(log_file_path):
    aggregated_data = pd.DataFrame() #Final dataframe
    log_files = glob.glob(log_files_path)
    # Get Queue Type
    logs = pd.DataFrame({"name": log_files})
    
    for i,name in enumerate(logs['name']):
        split_name = os.path.basename(name).split("-")
        logs.loc[i,'run'] = Queue_Type(split_name[0]).name + split_name[1]
        logs.loc[i,'queue_type'] = split_name[0]
        logs.loc[i,'num_clients'] = split_name[1]
        logs.loc[i,'client_id'] = split_name[2]

    data_files = {}
    for run in  logs["run"].unique():
        temp = logs[logs['run'].str.fullmatch(run)]
        data_files[run] = temp['num_clients'].unique()
    
    for run in logs["run"].unique():
        for num_clients in data_files[run]:
            runs = logs[ (logs['run'] ==  run )& (logs['num_clients'] == num_clients  )]
            # Aggregate into same data frame 
            run_list = []           
            for file_name in runs['name']:
                try:
                    print(f"Parsing:{file_name}")
                    df = pd.read_csv(file_name)
                    
                    df['entries'] = df['entries'].astype(int)
                    df['send_time'] = pd.to_datetime(df['send_time'], unit='ns')
                    df['return_time'] = pd.to_datetime(df['return_time'], unit='ns')

                    df['time_difference'] = (df['return_time'] - df['send_time']).dt.total_seconds()
                    run_list.append(df)
                except KeyError:
                    print("Parsing issues, next file")
                    continue
            
            print(f"Aggregating Entries for {run}")
            run_frame = pd.concat(run_list, axis=0)
            # We now have full run set for this number of clients and queue type
            # calculate latency, throughput
            start_send_time = run_frame['send_time'].min()
            start_return_time = run_frame['return_time'].min()

            end_send_time = run_frame['send_time'].max()
            end_return_time =  run_frame['return_time'].max()
        
            exclude_sample_range_sec = 60
            excluded_start_window = start_send_time + datetime.timedelta(0, exclude_sample_range_sec)
            excluded_end_window = end_send_time - datetime.timedelta(0, exclude_sample_range_sec)

            subset_samples = run_frame.loc[(run_frame['send_time'] > excluded_start_window) & (run_frame['send_time'] < excluded_end_window)]

            # Calculate Average Latency Per Request/Response for all clients in run
            avg_time_diff = subset_samples['time_difference'].mean()
            total_time = (excluded_end_window - excluded_start_window).total_seconds()
            total_items = len(subset_samples.index)
            percentile_90 = np.percentile(subset_samples['time_difference'], 90)
            percentile_95 = np.percentile(subset_samples['time_difference'], 95)
            # percentile_99 = np.percentile(subset_samples['time_difference'], 99)
            
              # Append aggregated data to the DataFrame
            row = pd.DataFrame({
                'queue_type' : re.sub(r'\d+', '', run),
                'num_clients': num_clients,
                'total_items': [total_items],
                'latency(us)': [avg_time_diff * 1000000] ,
                'throughput(op/ms)': [(total_items/(total_time * 1000))] ,
                '90th_percentile': [percentile_90],
                '95th_percentile': [percentile_95]
                # '99th_percentile': [percentile_99]
            })
            print(f"Run: {run}")
            print(row)
            aggregated_data = pd.concat([aggregated_data,row])
            grouped_ad = aggregated_data.sort_values(by=['num_clients'],key=lambda x: x.astype(int))
            # grouped_ad = aggregated_data.groupby(['queue_type', 'num_clients']).sort()
            grouped_ad.to_csv("expData.csv", sep='\t', index=False, header=False)
            
            ind = grouped_ad['queue_type'].str.contains("TCP|SEM")
            tcp_sem = grouped_ad[ind].sort_values(by=['num_clients'],key=lambda x: x.astype(int)) 
            tcp_sem.to_csv("tcp_sem.csv", sep='\t', index=False, header=False)
            ind = grouped_ad['queue_type'].str.contains("POLL|BOOST")
            poll_boost = grouped_ad[ind].sort_values(by=['num_clients'], key=lambda x: x.astype(int))
            poll_boost.to_csv("poll_boost.csv", sep='\t', index=False, header=False)

            # # Save every few files. 
            # if idx % 50 == 0:
            #     grouped_ad = aggregated_data.groupby(['queue_type', 'num_clients'])
            #     agg_ad = grouped_ad.agg(latency=('latency(us)', 'mean'), throughput=('throughput(op/ms)', 'sum')).reset_index().sort_values(by=['num_clients'],key=lambda x: x.astype(int))
            #     agg_ad.to_csv("expData.csv", sep='\t', index=False, header=False)
                
            #     ind = agg_ad['queue_type'].str.contains("TCP|SEM")
            #     tcp_sem = agg_ad[ind].sort_values(by=['num_clients'],key=lambda x: x.astype(int)) 
            #     tcp_sem.to_csv("tcp_sem.csv", sep='\t', index=False, header=False)

            #     ind = agg_ad['queue_type'].str.contains("POLL|BOOST")
            #     poll_boost = agg_ad[ind].sort_values(by=['num_clients'], key=lambda x: x.astype(int))
            #     poll_boost.to_csv("poll_boost.csv", sep='\t', index=False, header=False)
            #     # break;
                
    return aggregated_data


# Path to log files
log_files_path = '../bin/**.txt'

# Parse log files and aggregate data
aggregated_data = parse_log_files(log_files_path)

grouped_ad = aggregated_data.sort_values(by=['num_clients'],key=lambda x: x.astype(int))
grouped_ad.to_csv("expData.csv", sep='\t', index=False, header=False)

ind = grouped_ad['queue_type'].str.contains("TCP|SEM")
tcp_sem = grouped_ad[ind].sort_values(by=['num_clients'],key=lambda x: x.astype(int)) 
tcp_sem.to_csv("tcp_sem.csv", sep='\t', index=False, header=False)
ind = grouped_ad['queue_type'].str.contains("POLL|BOOST")
poll_boost = grouped_ad[ind].sort_values(by=['num_clients'], key=lambda x: x.astype(int))
poll_boost.to_csv("poll_boost.csv", sep='\t', index=False, header=False)
# Save to file.
# grouped_ad = aggregated_data.groupby(['queue_type', 'num_clients'])
# final_ad = grouped_ad.agg(latency=('latency(us)', np.mean), throughput=('throughput(op/ms)', np.sum)).reset_index().sort_values(by=['num_clients'],key=lambda x: x.astype(int))
# final_ad.to_csv("expData.csv", sep='\t', index=False, header=False)

# ind = final_ad['queue_type'].str.contains("TCP|SEM")
# tcp_sem = final_ad[ind].sort_values(by=['num_clients'],key=lambda x: x.astype(int)) 
# tcp_sem.to_csv("tcp_sem.csv", sep='\t', index=False, header=False)

# ind = final_ad['queue_type'].str.contains("POLL|BOOST")
# poll_boost = final_ad[ind].sort_values(by=['num_clients'], key=lambda x: x.astype(int))
# poll_boost.to_csv("poll_boost.csv", sep='\t', index=False, header=False)


Rtt_Experiment2Xs_Timestamp.main("./","expData")
Rtt_Experiment2Xs_Timestamp.main("./","tcp_sem")
Rtt_Experiment2Xs_Timestamp.main("./","poll_boost")
