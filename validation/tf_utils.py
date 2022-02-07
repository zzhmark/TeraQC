import cv2
import numpy as np
import os
import SimpleITK as sitk


def load_entire_resolution(dir):
    # the folder name has the dimension info of the 3D image, extract
    folder_name = os.path.split(dir)[-1]
    # dim_y, dim_x, dim_z = folder_name[4:-2].split('x')

    # when only low level resolution is provided, we have to assume that the image is evenly spliced
    # if blended with max resolution, it may be able to work with the unevenly sliced

    # _folders: all the folder in the corresponding dir of that dimension
    # _start: the starting coord of the current dimension of the iterated block
    # z_files: tiff image blocks

    y_folders = [d for d in os.listdir(dir) if os.path.isdir(os.path.join(dir, d))]
    ensemble_array = []
    for i, y_folder in enumerate(y_folders):
        y_dir = os.path.join(dir, y_folder)
        # y_start = y_folder.split('_')[-1]
        x_folders = [d for d in os.listdir(y_dir) if os.path.isdir(os.path.join(y_dir, d))]
        x_array = []
        for j, x_folder in enumerate(x_folders):
            x_dir = os.path.join(y_dir, x_folder)
            # x_start = x_folder.split('_')[-1]
            z_files = [d for d in os.listdir(x_dir)]
            z_array = []
            for k, z_file in enumerate(z_files):
                z_path = os.path.join(x_dir, z_file)
                # z_start = z_file.split('_')[-1]
                img = sitk.ReadImage(z_path)
                arr = sitk.GetArrayFromImage(img).transpose([1, 2, 0])
                z_array.append(arr)
            x_array.append(z_array)
        ensemble_array.append(x_array)
    return np.block(ensemble_array)