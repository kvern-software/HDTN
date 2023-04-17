import matplotlib.pyplot as plt
import numpy as np
import sys, getopt

logfile_prefix="/home/kyle/nasa/dev/test_outputs"

#argv
"""
0 = name of program
1 = test_name
"""
def main(argv):
    file_name="ssim_logfile"

    test_folder_name=argv[1]

    full_filepath = logfile_prefix + "/" +test_folder_name + "/" + file_name + ".txt"
    output_file_name=logfile_prefix + "/" + test_folder_name + "/" + file_name + ".png"


    #load
    matrix = np.loadtxt(full_filepath, dtype=np.string_, delimiter=" ")

    ssim = matrix[:, 4]

    for i in range(len(ssim)):
       ssim[i] = ssim[i][4:]

    ssim = ssim.astype(np.float)
    
    #plot
    plt.plot(ssim, "b", linewidth=0.7)
    plt.title("Structural Similarity Index")
    plt.ylabel("SSIM")
    plt.xlabel("Frame Number")
    plt.savefig(output_file_name, dpi=300)

    # plt.show()



if __name__ == "__main__":
    main(sys.argv)
    