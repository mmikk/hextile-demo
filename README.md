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

Most of the hex-tiling specific utility functions are in the file hextiling.h however for those of you looking to use
hex-tiling as planar/triplanar projection on large levels you should look at hextiling_rws.h which
provides a special variant to maintain fractional precision as we move far away from the center of absolute world space.

An important observation when rotating hex tiles for normal maps is the average normal of the normal map
must align "well" with the Z axis (0,0,1) or the tilt will appear very clearly in the lit result.
Good alignment is achieved (roughly) if the normal map corresponds to a tiling height map. Note that a 
tiling normal map does not necessarily correspond to a tiling height map. A basic example is a normal map
where every pixel is the same normal. This represents a tiling normal map but not a tiling height map.

In practice it is easy to end up with a normal map which does not represent a tiling height map since artists may use
a combination of different art tools which offer no such guarantee. It is possible to fix a misaligned normal map
by importing it into [Knald](https://www.knaldtech.com/) at which point the integrator finds a best fit tiling height map. However, be sure to set the Slope Range in the integrator group to 80% or higher to ensure the tool allows slopes steeper than 45 degrees.
