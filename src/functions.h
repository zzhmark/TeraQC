#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <v3d_interface.h>

bool reassemble_teraconvert(V3DPluginCallback& callback, const QString& path, uchar*& pBuffer, V3DLONG sz[4]);
/*
 * If the input image is in teraconverted format, it's in multiple blocks.
 * you can refer to terafly's nature methods paper and its supp materials
 * for its details. Here we need to reassemble them together, since it's in
 * the lowest resolution, it can be easily handled in whole.
 */

//bool extract_roi(const cv::Mat& image, double sigma, int radius);
/*
 * extract ROI based on local maxima density
 */

bool prepare_qc_block(const QString& tmp_path);


#endif // FUNCTIONS_H
