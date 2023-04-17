import matplotlib.pyplot as plt
import numpy as np
import sys, getopt
import json
import csv

logfile_prefix="/home/kyle/nasa/dev/test_outputs"

# function to add value labels
def addlabels(x,y):
    for i in range(len(x)):
        plt.text(i, y[i], y[i], ha = 'center')
#argv
"""
0 = name of program
1 = test_name
"""
def main(argv):
    test_folder_name=argv[1]
    # test_folder_name="lucia_loopback_outputs"

    file_name="vmaf_filename" 

    full_filepath = logfile_prefix + "/" +test_folder_name + "/" + file_name + ".json"
    output_file_name=logfile_prefix + "/" + test_folder_name + "/" + file_name + ".png"
    output_file_name_2=logfile_prefix + "/" + test_folder_name + "/" + file_name + "_stats.png"

    output_csv_file_name=logfile_prefix + "/" + test_folder_name + "/" + file_name + ".csv"



    #load
    f = open(full_filepath)
    data = json.load(f)

    # parse
    pooled_metrics = data["pooled_metrics"]
    avg_vmaf_scores = pooled_metrics["vmaf_4k"]

    # iterating to get vmaf of each frame to plot
    vmaf_scores = []
    for frame in data["frames"]:
        vmaf_scores.append(frame["metrics"]["vmaf_4k"])
    vmaf_averages = [avg_vmaf_scores["min"], avg_vmaf_scores["max"], avg_vmaf_scores["mean"], avg_vmaf_scores["harmonic_mean"]]
    #plot
    plt.plot(vmaf_scores, "b", linewidth=0.7)
    plt.title("VMAF Index")
    plt.ylabel("VMAF")
    plt.xlabel("Frame Number / 10")
    plt.savefig(output_file_name, dpi=300)

    plt.figure()
    plt.bar(["min", "max", "mean", "harmonic mean"], vmaf_averages, align="center",color=['black', 'green', 'red', 'blue'])
    plt.title("VMAF Averages")
    plt.ylabel("VMAF")
    plt.xlabel("Index")
    addlabels(["min", "max", "mean", "harmonic mean"], vmaf_averages)
    plt.savefig(output_file_name_2, dpi=300)


    # save averages to csv file
    data_file = open(output_csv_file_name, 'w')
    csv_writer = csv.writer(data_file)

    header = ["min", "max", "mean", "harmonic_mean"]
    csv_writer.writerow(header)
    csv_writer.writerow(vmaf_averages)
    data_file.close()




if __name__ == "__main__":
    main(sys.argv)
    