# hextile-demo

<img src="https://github.com/mmikk/mmikk.github.io/blob/master/pictures/hex_demo/colorhextiling.png" alt="Hex-Tiling Example" />
 
It is common in real-time graphics to use tiling textures to represent large surface area however repetition is noticeable as can be seen
here on the left. Using hex-tiling allows us to hide the repetition but requires a solution to hide the seams between adjacent hex tiles.
This sample is a demonstration of the paper [Practical Real-Time Hex-Tiling](https://github.com/mmikk/mmikk.github.io/blob/master/papers3d/mm_hex_compressed.pdf) which has been accepted for publication in [JCGT](https://jcgt.org/) and will appear there in the near future. The method is an adaptation of the original work by [Eric Heitz and Fabrice Neyret](https://eheitzresearch.wordpress.com/722-2/) as well as [Thomas Deliot and Eric Heitz](https://eheitzresearch.wordpress.com/738-2/).

<img src="https://github.com/mmikk/mmikk.github.io/blob/master/pictures/hex_demo/splash.png" alt="Hex-Tiling Mask" />


The original method preserves contrast using a histogram–preserving method which requires a precomputation step to convert the source texture into
a transform– and inverse transform texture which must both be sampled in the shader rather than the original source texture.
Thus deep integration into the application is required for this to appear opaque to the author of the shader and material.
In our adaptation we omit histogram–preservation and replace it with a novel blending method which allows us to sample the original source texture.

When running this demo be sure to try to rotate the hex tiles. You can toggle between 4 different parametters to tweak on the m key:
tile rate, rotation, hex tile contrast for color and hex tile contrast for normal mapping. Press and hold the scroll wheel while moving the mouse
to adjust the active parameter. Press the r key to reset all 4 parameters. 
