import os
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import subprocess
import code
import numpy as np 
import argparse


def load_data(directory, prefixes, type_name):
	"""
	Load data from CSV files in the specified directory with the given prefixes.
	"""

	dirname = os.path.dirname(__file__)
	absolute_data_path = os.path.join(dirname, directory)

	data_frames = []
	for prefix in prefixes:
		for series_directory in os.listdir(absolute_data_path):
			if series_directory.startswith(prefix):
				absolute_series_directory = os.path.join(absolute_data_path, series_directory)
				for run_dir in os.listdir(absolute_series_directory):
					absolute_run_directory = os.path.join(absolute_series_directory, run_dir)
					for filename in os.listdir(absolute_run_directory):
						if filename.endswith('.csv'):
							if type_name in filename:
								series_name = filename.split('_')[0]
								# type_name = filename.split('_')[1].split('.')[0]
								run= run_dir.split('_')[-1]

								file_path = os.path.join(absolute_run_directory, filename)
								# Check
								df = pd.read_csv(file_path)
								# Add a column to tag the dataframe with the run directory name
								df['series_name'] =series_name
								# df['type_name'] = type_name
								# df['run'] = run
								data_frames.append(df)
	return pd.concat(data_frames, ignore_index=True)

def run_experiment(program_path: str,data_directory: str,config: list[str],runs : int):
	"""
	Run an experiment with the given configuration and tag.P
	This is a placeholder function and should be replaced with actual experiment logic.
	""" 

	dirname = os.path.dirname(__file__)
	absolute_program_path = os.path.join(dirname, program_path)

	experiment_dir_name = f"{config['param1']}_{config['param2']}_{config['run']}"

	# Create a directory for the experiment
	configuration_name_dir = os.path.join(data_directory, f"{config['param1']}_{config['param2']}/")
	if not os.path.exists(configuration_name_dir):
		os.makedirs(configuration_name_dir)
	
	final_name = configuration_name_dir + experiment_dir_name
	# Create a directory for the experiment
	experiment_dir = os.path.join(final_name)
	if not os.path.exists(experiment_dir):
		os.makedirs(experiment_dir)
	
	# Change the current working directory to the experiment directory
	os.chdir(experiment_dir)
	
	# Construct the command to run the C program
	command = [absolute_program_path, config['param1'], config['param2']]
	
	# Run the command and capture the output
	result = subprocess.run(command, capture_output=True, text=True)

	# Check if the command was successful
	if result.returncode != 0:
		print(f"Error running experiment: {result.stderr}")
	else:
		print(f"Experiment done: {command}")


	os.chdir(dirname)

	# Add your experiment logic here
	# For example, generate data and save to CSV files with the tag

def interact_console(local_vars):
		"""
		Start an interactive console with the given local variables.
		"""
		console = code.InteractiveConsole(locals=local_vars)
		console.interact("Interactive console started. Type 'exit()' or 'Ctrl-D' to exit.")

def analyze_data(data):
	"""
	Perform analysis on the dataset.
	This is a placeholder function and should be replaced with actual analysis logic.
	"""
	print("Analyzing data...")
	# Add your analysis logic here
	# For example, calculate statistics, generate plots, etc.
	
	# Start interactive console for further analysis
	interact_console(locals())
	
	



def generate_graphs(merged_data):
	"""
	Generate graphs from the dataset with selectable variables.
	"""
	fig = plt.figure() # Create matplotlib figure  
	ax = fig.add_subplot(111) # Create matplotlib axes
	ax2 = ax.twinx()

	CB_color_cycle = ['#377eb8', '#ff7f00', '#4daf4a',
					'#f781bf', '#a65628', '#984ea3',
					'#999999', '#e41a1c', '#dede00']

	merged_data['total_CPU_timesec_mean'] = merged_data['user_CPU_timesec_mean'] + merged_data['system_CPU_timesec_mean']


	width = 0.2
	N = len(merged_data['Name'])
	ind = np.arange(N)

	bars1 = ax.bar(ind, merged_data['nsop_mean'], width,color='#377eb8')
	bars2 = ax2.bar(ind + width, merged_data['user_CPU_timesec_mean'], width, color='#ff7f00')
	bars3 = ax2.bar(ind + width*2, merged_data['system_CPU_timesec_mean'], width, color='#4daf4a')

	# Add value labels on top of each bar
	for bar in bars1:
		height = bar.get_height()
		ax.annotate(f'{height:.2f}', xy=(bar.get_x() + bar.get_width() / 2, height), xytext=(0, 3), textcoords="offset points", ha='center', color='#377eb8')

	for bar in bars2:
		height = bar.get_height()
		ax2.annotate(f'{height:.2f}', xy=(bar.get_x() + bar.get_width() / 2, height), xytext=(0, 3), textcoords="offset points", ha='center', color='#ff7f00')

	for bar in bars3:
		height = bar.get_height()
		ax2.annotate(f'{height:.2f}', xy=(bar.get_x() + bar.get_width() / 2, height), xytext=(0, 3), textcoords="offset points", ha='center', color='#4daf4a')

	# Set the labels and title
	ax.set_xlabel('Queue Type')
	ax.set_ylabel('Latency (ns/op)')
	ax2.set_ylabel('CPU time (sec)')


	# Set the x-ticks to use the values in the dataframe
	ax.set_xticks(range(len(merged_data['Name'])))
	ax.set_xticklabels(merged_data['Name'])

	# Create a legend
	ax.legend(['ns/op'], loc='upper left', bbox_to_anchor=(0.18, 1))
	ax2.legend(['User CPU time (sec)', 'System CPU time (sec)'], loc='upper right')

	plt.savefig("queue_latency.png")


# def main():
# 	program_path = './bin/bench_queue'
# 	data_directory = 'data'
# 	series_prefixes = ['POLL', 'UMWAIT', 'SEM']  # Add more prefixes as needed
	
# 	# Define experiment configurations
# 	# queue_sizes = [160024, 320048, 640096, 1280192]
# 	queue_size = 160024
	
# 	# Define different sets of runs to manage the precision of the experiment
# 	run_precision = [10]

# 	# Choose the desired precision level
# 	experiment_configs = []
# 	for prefix in series_prefixes:
# 		for run_set in run_precision:
# 			for i in range(0, run_set):
# 				experiment_configs.append({'param1': prefix, 'param2': str(queue_size), 'run': f'{i}', 'tag': 'cached'})
			
# 	# Ensure the data directory exists
# 	if not os.path.exists(data_directory):
# 		os.makedirs(data_directory)

# 	# Run experiments
# 	for config in experiment_configs:
# 		print(f"Running experiment with config:{config}")
# 		run_experiment(program_path, data_directory,config, config['run'])

# 	# Load and analyze data
# 	bench_data = load_data(data_directory, series_prefixes, 'bench')
# 	rusage_data = load_data(data_directory, series_prefixes, 'rusage')

# 	mean_latency_with_stdev_per_series = bench_data.groupby(['Name']).agg(nsop_mean=('ns/op', 'mean'), nsop_sem=('ns/op', 'sem')).reset_index()

# 	mean_cpu_time_ctx_switches = rusage_data.groupby(['series_name']).agg(user_CPU_timesec_mean=('user_CPU_time(sec)', 'mean'), system_CPU_timesec_mean=('system_CPU_time(sec)', 'mean'), voluntary_context_switches_mean=('voluntary_context_switches', 'mean'), involuntary_context_switches_mean=('involuntary_context_switches', 'mean')).reset_index()

# 	merged_data = pd.merge(mean_latency_with_stdev_per_series, mean_cpu_time_ctx_switches, left_on='Name', right_on='series_name', how='outer')


# 	# Generate graphs
# 	generate_graphs(merged_data)

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Run experiments and analyze data.')
	parser.add_argument('--run', action='store_true', help='Run the experiment')
	parser.add_argument('--analyze', action='store_true', help='Load and analyze data')
	args = parser.parse_args()

	if args.run:
		program_path = './bin/bench_queue'
		data_directory = 'data'
		series_prefixes = ['POLL', 'UMWAIT', 'SEM']  # Add more prefixes as needed
		
		# Define experiment configurations
		queue_size = 160024
		
		# Define different sets of runs to manage the precision of the experiment
		run_precision = [10]

		# Choose the desired precision level
		experiment_configs = []
		for prefix in series_prefixes:
			for run_set in run_precision:
				for i in range(0, run_set):
					experiment_configs.append({'param1': prefix, 'param2': str(queue_size), 'run': f'{i}', 'tag': 'cached'})
				
		# Ensure the data directory exists
		if not os.path.exists(data_directory):
			os.makedirs(data_directory)

		# Run experiments
		for config in experiment_configs:
			print(f"Running experiment with config:{config}")
			run_experiment(program_path, data_directory, config, config['run'])

	if args.analyze:
		data_directory = 'data'
		series_prefixes = ['POLL', 'UMWAIT', 'SEM']  # Add more prefixes as needed

		# Load and analyze data
		bench_data = load_data(data_directory, series_prefixes, 'bench')
		rusage_data = load_data(data_directory, series_prefixes, 'rusage')

		mean_latency_with_stdev_per_series = bench_data.groupby(['Name']).agg(nsop_mean=('ns/op', 'mean'), nsop_sem=('ns/op', 'sem')).reset_index()

		mean_cpu_time_ctx_switches = rusage_data.groupby(['series_name']).agg(user_CPU_timesec_mean=('user_CPU_time(sec)', 'mean'), system_CPU_timesec_mean=('system_CPU_time(sec)', 'mean'), voluntary_context_switches_mean=('voluntary_context_switches', 'mean'), involuntary_context_switches_mean=('involuntary_context_switches', 'mean')).reset_index()

		merged_data = pd.merge(mean_latency_with_stdev_per_series, mean_cpu_time_ctx_switches, left_on='Name', right_on='series_name', how='outer')

		# Generate graphs
		generate_graphs(merged_data)