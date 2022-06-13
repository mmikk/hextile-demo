# hextile-demo

<img src="https://github.com/mmikk/mmikk.github.io/blob/master/pictures/hex_demo/splash.png" alt="Hex-Tiling Mask" />

This sample is a demonstration of the paper [Practical Real-Time Hex-Tiling](https://github.com/mmikk/mmikk.github.io/blob/master/papers3d/mm_hex_compressed.pdf).

The method is an adaptation of the original work by [Eric Heitz and Fabrice Neyret](https://eheitzresearch.wordpress.com/722-2/)
as well as [Thomas Deliot and Eric Heitz](https://eheitzresearch.wordpress.com/738-2/).

The original method preserves contrast using a histogram–preserving method which requires a precomputation step to convert the source texture into
a transform– and inverse transform texture which must both be sampled in the shader rather than the original source texture.
Thus deep integration into the application is required for this to appear opaque to the author of the shader and material.
In our adaptation we omit histogram–preservation and replace it with a novel blending method which allows us to sample the original source texture.


 
