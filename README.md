# TeraQC
This is a Vaa3D plugin project for fMOST brain image quality check in Braintell single neuron morphology annotation generation pipeline, meant to be blended into MorphoHub.

## Background Explanation
The fMost imaging data of mouse brain is of highest quality and suitable for generating whole-brain manual and automatic reconstructions of single neurons. However, its varying quality can still hinders the process. Plus, to reduce the effort to locate probable sites for reconstruction(since we use sparsely labeled brains), we may also need know which part of the brain is of interest.

Thus we come up with an idea that uses intensity distribution randomly collected from parts of the brain with most signal to judge whether it is of high quality or not. To balance the speed and effectiveness, some information is gathered from downsampling images used by TeraVR, while some other is gathered from the raw resolution.

## Algorithm

1. Strong signal intensity sites filtering  
    Extract regions with strong signals, which can have lots of somas or fiber tracts, elligible for judging the image quality.
2. Random sampling of sub-blocks  
    Randomly choose n(=10) blocks in a single region.
3. Plotting grayscale histogram
    Plotting grayscale histogram for each block, thus we have the randomly generated histograms for each region, and able to compare the regions and assaemble these traits to make a final evaluation of the image quality.

## Usage
TODO



## Folder Structure



```
├── data                    # Images for test and validation (not shown)
├── build                   # Vaa3D plugin build output (not shown)
├── src                     # Source files of the Vaa3D plugin
├── validation              # Python scripts to validate the algorithm and results
├── LICENSE
└── README.md
```



Zuohan Zhao  
2020/1/9

