import cv2
import numpy as np
import SimpleITK as sitk
import ipywidgets as widgets
import os


def load_entire_resolution(dir):
    '''
    :param dir: path to a resolution
    :return: assembled image stack (by z, y, x) of that resolution
    '''

    # _folders: all the folder in the corresponding dir of that dimension
    # _start: the starting coord of the current dimension of the iterated block
    # z_files: tiff image blocks

    y_folders = [d for d in os.listdir(dir) if os.path.isdir(os.path.join(dir, d))]
    ensemble_array = []
    for i, y_folder in enumerate(y_folders):
        y_dir = os.path.join(dir, y_folder)
        x_folders = [d for d in os.listdir(y_dir) if os.path.isdir(os.path.join(y_dir, d))]
        x_array = []
        for j, x_folder in enumerate(x_folders):
            x_dir = os.path.join(y_dir, x_folder)
            z_files = [d for d in os.listdir(x_dir)]
            z_array = []
            for k, z_file in enumerate(z_files):
                z_path = os.path.join(x_dir, z_file)
                img = sitk.ReadImage(z_path)
                arr = sitk.GetArrayFromImage(img).transpose([1, 2, 0])
                z_array.append(arr)
            x_array.append(z_array)
        ensemble_array.append(x_array)
    return np.block(ensemble_array)


def load_from_teraconvert(path_to_brain_id):
    '''
    :param path_to_brain_id: the path to the teraconvert dir of a brain
    :return: the lowest resolution brain (as numpy array) of that dir, by z, y, x
    '''
    res = os.listdir(path_to_brain_id)
    res_x = [str(name.split('x')[1]) for name in res]
    min_res = res[np.argmin(res_x)]
    min_res_dir = os.path.join(path_to_brain_id, min_res)

    return load_entire_resolution(min_res_dir).transpose([2, 0, 1])

def filter_lines(lines, minDist, angleLim, imgShape, ellipseRatio):
    '''
    Given a set of lines, fitler based on distance to center and orientation.
    The distance of lines to the center of the 3D image should have a lower limit;
    The orientation of the lines(markers) can vary, but based on the data we have,
    they should be aligned to a specific direction, which means some can be discarded provided
    a variance limit.
    coord: x, y, z
    :param lines: all detected marker lines in the image
    :param minDist: the minimal distance to the center
    :param angleLim: the angle limit of the lines to the weighted average direction
    :param imgShape: the shape of the 3d image array, for finding the center
    :param ellipseRatio: fit the markers as an ellipse, and beyond this ratio scope lines are retained
    :return: filtered list of lines
    '''

    # fct = np.diag([1, 1, zThickness])
    # center = np.flip(imgShape).dot(fct) / 2


    # def pass_dist(p1, p2):
    #     _p1 = p1 * fct
    #     _p2 = p2 * fct
    #     return np.linalg.norm(np.cross(_p2 - _p1, _p1 - center)) / np.linalg.norm(_p2 - _p1) >= minDist
    center = np.flip(imgShape) / 2
    def get_dist_xy(p1, p2):
        _p1 = p1[:2]
        _p2 = p2[:2]
        return np.linalg.norm(np.cross(_p2 - _p1, _p1 - center[:2])) / np.linalg.norm(_p2 - _p1)

    coords = [(get_dist_xy(*line), line[0][2]) for line in lines]

    (d0, z0), (a, b), angle = cv2.fitEllipse(coords)
    theta = np.radians(angle)
    c, s = np.cos(theta), np.sin(theta)
    R = np.array(((c, -s), (s, c)))
    ell = R @ np.diag(1/a, 1/b) @ R.transpose()

    def pass_dist(d, z):
        dp = np.array([(d - d0, z - z0)])
        dist = dp @ ell @ dp.transpose()
        return dist >= ellipseRatio and

    # return acute angle of 2 vectors
    def get_angle(v1, v2):
        inner = np.inner(v1, v2)
        norms = np.linalg.norm(v1) * np.linalg.norm(v2)
        cos = abs(inner / norms)
        rad = np.arccos(np.clip(cos, -1.0, 1.0))
        deg = np.rad2deg(rad)
        return abs(deg)

    # averaging orientation
    u, s, v = np.linalg.svd([p1-p2 for p1, p2 in lines], False)
    main_ax = v[np.argmax(s)]
    print(main_ax)

    def pass_angle(p1, p2):
        return get_angle(p1 - p2, main_ax) <= angleLim

    return [line for i, line in enumerate(lines) if pass_angle(*line) and pass_dist(*coords[i])]


def remove_marker(img, **kwargs):
    '''
    :param img: 3d stack numpy array (z,y,x)
    :return: the binary mask of the marker (255 for marker)
    '''
    stack = (img / img.max() * 255).astype('uint8')
    SE1 = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (kwargs['SE1'],)*2)
    SE2 = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (kwargs['SE2'],)*2)
    SE3 = cv2.getStructuringElement(cv2.MORPH_RECT, (kwargs['SE3'], 1))
    for i, sl in enumerate(stack):
        # Closing
        closed = cv2.morphologyEx(sl, cv2.MORPH_CLOSE, SE1)
        # Sobel
        grad_x, grad_y = cv2.spatialGradient(closed, kwargs['sobelKsize'])
        edges = np.hypot(grad_x, grad_y)
        stack[i] = edges

    # OTSU
    stack = np.stack(stack).reshape(-1)
    thr, stack = cv2.threshold(stack, 0, 255, cv2.THRESH_OTSU)
    stack = stack.reshape(img.shape)

    # get all marker lines
    all_lines = []
    for i, sl in enumerate(stack):
        # Closing
        closed = cv2.morphologyEx(sl, cv2.MORPH_CLOSE, SE2)
        # Hough Line Transform P
        lines = cv2.HoughLinesP(closed, kwargs['rho'], kwargs['theta'],
                                kwargs['threshold'], minLineLength=kwargs['minLineLength'], maxLineGap=kwargs['maxLineGap'])
        if lines is not None:
            # reshape lines so that it looks like:
            # [ [[x1, y1], [x2, y2]], .. ]
            # and append the slice info so that it looks like
            # [ [[x1, y1, z1], [x2, y2, z2]], .. ]
            lines = np.append(lines.reshape(-1, 2, 2), np.ones([len(lines), 2, 1], dtype=np.int32) * i, axis=2)
            all_lines.extend(lines)

    # draw the mask
    stack = np.zeros(stack.shape, dtype=np.uint8)
    for p1, p2 in filter_lines(all_lines, kwargs['minDist'], kwargs['angleLim'], img.shape, kwargs['zThickness']):
        cv2.line(stack[p1[2]], p1[:2], p2[:2], 255, kwargs['lineWidth'])

    # dilation
    stack = stack.reshape(stack.shape[0], -1)
    stack = cv2.morphologyEx(stack, cv2.MORPH_CLOSE, SE3)
    stack = stack.reshape(img.shape)

    return stack


def main(
        inPath,
        outPath,
        SE1=11,
        sobelKSize=7,
        SE2=7,
        houghRho=1,
        houghAngleDivision=180,
        houghThresh=100,
        houghMinLineLen=100,
        houghMaxLineGap=1,
        filterAngleLim=5,
        lineWidth=3,
        SE3=11
):
    pass


if __name__ == '__main__':
    fire.Fire(main)