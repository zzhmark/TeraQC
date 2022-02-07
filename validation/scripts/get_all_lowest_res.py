import os
import tf_utils
import SimpleITK as sitk

if __name__ == '__main__':
    dir = '../../data'
    out_dir = '../results/lowestRes'
    for level in os.listdir(dir):
        level_dir = os.path.join(dir, level)
        for id in os.listdir(level_dir):
            id_dir = os.path.join(level_dir, id)
            res = os.listdir(id_dir)

            min_res = res[0]
            for i in res:
                if len(i) < len(min_res):
                    min_res = i
            min_res_dir = os.path.join(id_dir, min_res)

            ensemble_arr = tf_utils.load_entire_resolution(min_res_dir)

            sitk_io = sitk.GetImageFromArray(ensemble_arr.transpose([2, 0, 1]))
            sitk_io.SetSpacing([5, 5, 1])
            sitk.WriteImage(sitk_io, os.path.join(out_dir, str(id) + '.tif'))
