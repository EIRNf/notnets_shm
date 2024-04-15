import datetime
import os
import pandas as pd
import numpy as np
import glob
import matplotlib.pyplot as pp
import matplotlib

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

    # Get a list of all log files
    log_files = sorted(glob.glob(log_files_path), key=os.path.getmtime)

    aggregated_data = pd.DataFrame()

    for idx, file in enumerate(log_files):
        try:
            df = pd.read_csv(file)
            df['entries'] = df['entries'].astype(float)
            df['send_time'] = pd.to_datetime(df['send_time'], unit='ns')
            df['return_time'] = pd.to_datetime(df['return_time'], unit='ns')

            df['time_difference'] = (df['return_time'] - df['send_time']).dt.total_seconds()
            
            # Timestamps
            start_time = df['send_time'].iloc[1]
            start_return_time = df['return_time'].iloc[1]

            end_time = df['send_time'].iloc[-1]
        
            exclude_sample_range_sec = 60
            excluded_start_window = start_time + datetime.timedelta(0, exclude_sample_range_sec)
            excluded_end_window = end_time - datetime.timedelta(0, exclude_sample_range_sec)

            subset_samples = df.loc[(df['send_time'] > excluded_start_window) & (df['send_time'] < excluded_end_window)]

            # Calculate Average Latency Per Request/Response
            avg_time_diff = subset_samples['time_difference'].mean()
            ## TODO: COUNT NUMBER OF ITEMS PER CLIENT, JUST TAKE OVER TIME WINDOW.
            total_time = (excluded_end_window - excluded_start_window).total_seconds()
            total_items = len(subset_samples.index)
            # percentile_90 = np.percentile(subset_samples['time_difference'], 90)
            # percentile_95 = np.percentile(subset_samples['time_difference'], 95)
            # percentile_99 = np.percentile(subset_samples['time_difference'], 99)

            file_name = os.path.basename(file)
            meta_data = file_name.split('-')
            queue_type = meta_data[0]
            num_clients = meta_data[1]

            # Append aggregated data to the DataFrame
            row = pd.DataFrame({
                'queue_type' : Queue_Type(queue_type).name,
                'num_clients': num_clients,
                'total_items': [total_items],
                'latency(us)': [avg_time_diff * 1000000] ,
                'throughput(op/ms)': [(total_items/(total_time * 1000))] 
                # '90th_percentile': [percentile_90],
                # '95th_percentile': [percentile_95],
                # '99th_percentile': [percentile_99]
            })

            print(f"File#:{idx} out of {len(log_files)}")
            print(row)
            aggregated_data = pd.concat([aggregated_data,row])

            # Save every few files. 
            if idx % 50 == 0:
                grouped_ad = aggregated_data.groupby(['queue_type', 'num_clients'])
                agg_ad = grouped_ad.agg(latency=('latency(us)', 'mean'), throughput=('throughput(op/ms)', 'sum')).reset_index().sort_values(by=['num_clients'],key=lambda x: x.astype(int))
                agg_ad.to_csv("expData.csv", sep='\t', index=False, header=False)
                
                ind = agg_ad['queue_type'].str.contains("TCP|SEM")
                tcp_sem = agg_ad[ind].sort_values(by=['num_clients'],key=lambda x: x.astype(int)) 
                tcp_sem.to_csv("tcp_sem.csv", sep='\t', index=False, header=False)

                ind = agg_ad['queue_type'].str.contains("POLL|BOOST")
                poll_boost = agg_ad[ind].sort_values(by=['num_clients'], key=lambda x: x.astype(int))
                poll_boost.to_csv("poll_boost.csv", sep='\t', index=False, header=False)
                # break;
        except KeyError:
            print("Parsing issues, next file")
            continue;


    return aggregated_data


# Path to log files
log_files_path = '../bin/**.txt'

# Parse log files and aggregate data
aggregated_data = parse_log_files(log_files_path)
# Save to file.
grouped_ad = aggregated_data.groupby(['queue_type', 'num_clients'])
final_ad = grouped_ad.agg(latency=('latency(us)', np.mean), throughput=('throughput(op/ms)', np.sum)).reset_index().sort_values(by=['num_clients'],key=lambda x: x.astype(int))
final_ad.to_csv("expData.csv", sep='\t', index=False, header=False)

ind = final_ad['queue_type'].str.contains("TCP|SEM")
tcp_sem = final_ad[ind].sort_values(by=['num_clients'],key=lambda x: x.astype(int)) 
tcp_sem.to_csv("tcp_sem.csv", sep='\t', index=False, header=False)

ind = final_ad['queue_type'].str.contains("POLL|BOOST")
poll_boost = final_ad[ind].sort_values(by=['num_clients'], key=lambda x: x.astype(int))
poll_boost.to_csv("poll_boost.csv", sep='\t', index=False, header=False)


Rtt_Experiment2Xs_Timestamp.main("./","expData")
Rtt_Experiment2Xs_Timestamp.main("./","tcp_sem")
Rtt_Experiment2Xs_Timestamp.main("./","poll_boost")
