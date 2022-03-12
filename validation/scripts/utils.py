import numpy as np
import os
import SimpleITK as sitk
import matplotlib.pyplot as plt

def load_entire_resolution(dir):
    '''
    :param dir: path to a resolution
    :return: assembled image stack (by z, y, x) of that resolution
    '''

    # _folders: all the folder in the corresponding dir of that dimension
    # _start: the starting coord of the current dimension of the iterated block
    # z_files: tiff image blocks

    y_folders = [d for d in os.listdir(dir) if os.path.isdir(os.path.join(dir, d))]
    y_folders.sort()
    ensemble_array = []
    for i, y_folder in enumerate(y_folders):
        y_dir = os.path.join(dir, y_folder)
        x_folders = [d for d in os.listdir(y_dir) if os.path.isdir(os.path.join(y_dir, d))]
        x_folders.sort()
        x_array = []
        for j, x_folder in enumerate(x_folders):
            x_dir = os.path.join(y_dir, x_folder)
            z_files = [d for d in os.listdir(x_dir)]
            z_files.sort()
            z_array = []
            for k, z_file in enumerate(z_files):
                z_path = os.path.join(x_dir, z_file)
                img = sitk.ReadImage(z_path)
                arr = sitk.GetArrayFromImage(img).transpose([1, 2, 0])
                z_array.append(arr)
            x_array.append(z_array)
        ensemble_array.append(x_array)
    return np.block(ensemble_array).transpose([2, 0, 1])


def load_from_teraconvert(path_to_brain_id):
    '''
    :param path_to_brain_id: the path to the teraconvert dir of a brain
    :return: the lowest resolution brain (as numpy array) of that dir, by z, y, x
    '''
    res = os.listdir(path_to_brain_id)
    res_x = [str(name.split('x')[1]) for name in res]
    min_res = res[np.argmin(res_x)]
    min_res_dir = os.path.join(path_to_brain_id, min_res)

    return load_entire_resolution(min_res_dir)
