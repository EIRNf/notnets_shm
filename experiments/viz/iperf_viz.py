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

MESSAGE_SIZE = 128
EXECUTION_LENGTH = 30


# def wavg(group):
#     d = group['avg_lat']
#     w = group['count']
#     return (d * w).sum() / w.sum()


def wavg(group, avg_name, weight_name):
    """ http://stackoverflow.com/questions/10951341/pandas-dataframe-aggregate-function-using-multiple-columns
    In rare instance, we may not have weights, so just return the mean. Customize this if your business case
    should return otherwise.
    """
    d = group[avg_name]
    w = group[weight_name]
    try:
        return (d * w).sum() / w.sum()
    except ZeroDivisionError:
        return d.mean()
    
def wsum(group, avg_name, weight_name):
    """ Get weighted average and sum it's total
    """
    d = group[avg_name]
    w = group[weight_name]
    try:
        return (d * w).sum()
    except ZeroDivisionError:
        return d.mean()

def parse_iperf_log_files(log_file_path):
    aggregated_data = pd.DataFrame() #Final dataframe
    log_files = glob.glob(log_files_path)
    # Get Queue Type
    logs = pd.DataFrame({"name": log_files})
    
    for i,name in enumerate(logs['name']):
        split_root_name = os.path.basename(name).split("/")
        split_name_suffix = os.path.basename(split_root_name[0]).split("_")
        split_name = os.path.basename(split_name_suffix[1]).split(".")
        logs.loc[i,'num_clients'] = split_name[0]

    for num_clients in logs['num_clients']:
        run = logs[ (logs['num_clients'] == num_clients)]
        # Data already aggregated into a single file
        # ,client,rps, avg_lat, min_lat, max_lat, stddev_lat
        # colnames = ["num_clients", "client", "count", "rps", "avg_lat", "min_lat", "max_lat", "stddev_lat"]
        df = pd.DataFrame()
        try:
            with open(run['name'].iloc[0]) as file:
                # lines to parse
                summary_lines = 3 + (int(num_clients) * 2) #Aligns with iperf bounceback output
                line_array =  file.readlines() [-summary_lines:] # Capture aggregated data
                del line_array[-3:] # Get rid of last two lines

                i=0
                for line in line_array:
                    if 'BB8' in line:
                        continue                        
                    
                    count_string = re.findall(r'\d*=',line)
                    rps_string = re.findall(r'\d*\srps',line)
                    lat_string = re.findall(r'=[\d\s\.\/]*',line)
                    
                    count = count_string[0].replace('=','')
                    rps = rps_string[0].replace('rps','')
                    split_lat = lat_string[0].replace('=','').split("/")
                    avg_lat = split_lat[0]
                    min_lat = split_lat[1]
                    max_lat = split_lat[2]
                    std_dev_lat = split_lat[3]
                    client_entry = pd.DataFrame({
                        'num_clients' : int(num_clients),
                        'client' : i,
                        'count' : float(count),
                        'rps' : float(rps),
                        'avg_lat': float(avg_lat),
                        'min_lat': float(min_lat),
                        'max_lat': float(max_lat),
                        'stddev_lat': float(std_dev_lat),
                    },index=[i])
                    i+=1
                    df = pd.concat([df, client_entry])
            
            # colnames = ["timestamp", "source_address","source_port","destination_address","destination_port","interval","transferred_bytes","bits_per_second"]
            print(f"Parsing:{run['name']}")
            # df = pd.read_csv(run['name'].iloc[0], names=colnames, header=None)
                        
                        
            
            # Average throughput
            # sum_average_throughput = wsum(df , 'rps', 'count')
            total_throughput =  df["rps"].sum()
            total_average_bytes= total_throughput * 8 
            throughput_b_s = total_average_bytes * MESSAGE_SIZE # bits per sec to megabytes per second
            throughput_mb_s = throughput_b_s /  1e+6

            #  weighted average latency?
            avg_lat_agg =   wavg(df , 'avg_lat', 'count')
            average_latency_us = avg_lat_agg * 1000 
                        
            # Append aggregated data to the DataFrame
            row = pd.DataFrame({
                'queue_type' : "iperf",
                'num_clients': num_clients,
                'total_items': [0], 
                'latency(us)': [average_latency_us] ,
                'throughput(rps)': [total_throughput] ,
                '90th_percentile(us)': [0],
                '95th_percentile(us)': [0],
                '99th_percentile(us)': [0]
            })
            
            print(f"Run: {run}")
            print(row)
            aggregated_data = pd.concat([aggregated_data,row])
            
            grouped_ad = aggregated_data.sort_values(by=['num_clients'],key=lambda x: x.astype(int))
            # grouped_ad = aggregated_data.groupby(['queue_type', 'num_clients']).sort()
            grouped_ad.to_csv("iperf_data.csv", sep='\t', index=False, header=False)
            
            # ind = grouped_ad['queue_type'].str.contains("TCP|SEM")
            # tcp_sem = grouped_ad[ind].sort_values(by=['num_clients'],key=lambda x: x.astype(int)) 
            # tcp_sem.to_csv("tcp_sem.csv", sep='\t', index=False, header=False)
            # ind = grouped_ad['queue_type'].str.contains("POLL|BOOST")
            # poll_boost = grouped_ad[ind].sort_values(by=['num_clients'], key=lambda x: x.astype(int))
            # poll_boost.to_csv("poll_boost.csv", sep='\t', index=False, header=False)
            
        except KeyError:
            print("Parsing issues, next file")
            continue
            
    return aggregated_data


# Path to log files
# log_files_path = '../bin/**.txt'
log_files_path = '../viz/iperf_exps/**.csv'

# Parse log files and aggregate data
aggregated_data = parse_iperf_log_files(log_files_path)

grouped_ad = aggregated_data.sort_values(by=['num_clients'],key=lambda x: x.astype(int))
grouped_ad.to_csv("iperf_data.csv", sep='\t', index=False, header=False)

# ind = grouped_ad['queue_type'].str.contains("TCP|SEM")
# tcp_sem = grouped_ad[ind].sort_values(by=['num_clients'],key=lambda x: x.astype(int)) 
# tcp_sem.to_csv("tcp_sem.csv", sep='\t', index=False, header=False)
# ind = grouped_ad['queue_type'].str.contains("POLL|BOOST")
# poll_boost = grouped_ad[ind].sort_values(by=['num_clients'], key=lambda x: x.astype(int))
# poll_boost.to_csv("poll_boost.csv", sep='\t', index=False, header=False)
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


Rtt_Experiment2Xs_Timestamp.main("./","iperf_data")
# Rtt_Experiment2Xs_Timestamp.main("./","tcp_sem")
# Rtt_Experiment2Xs_Timestamp.main("./","poll_boost")










