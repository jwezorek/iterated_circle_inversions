# iterated_circle_inversions
![sample output](http://jwezorek.com/wp-content/uploads/2024/08/square_new.png)
Command line tool for generating images of iterated circle inversions.

The basic idea is the following:

* Let S be a set of seed circles
* Repeat *n* times:
  * Let S2 be an empty set of circles
  * For each combination [a,b] of circes in S:
    * Add a and b to S2 if they are not already in S2
    * Let c be a inverted about b, and let d be b inverted about a.
    * Add c and d to S2 if they are not already in S2.
  * Let S := S2

The above was generated via the following JSON input:

    {
        "circles": [
            [ 0.7071067811865476, 0, 1 ],
            [ 0, 0.7071067811865476, 1 ],
            [ -0.7071067811865476, 0, 1 ],
            [ 0, -0.7071067811865476, 1 ],
            [ 0, 0, 1.7071067811865475 ]
        ],
    "iterations": 3,
    "out-file": "square_3iters.png",
    "resolution" : 3000,
    "antialiasing_level": 3,
    "colors": [ "#000000", "#444444", "#AAAAAA", "#FFD700" ],
    "view": [ -10, -10, 10, 10 ]
    }
 
* circles: The circles array is the [center_x, center_y, radius] of the seed circles.
* iterations: Number of passes of performating circle inversion over all pairs of circles.
* out-file: Pathname of the output file. The extension determines whether we are rasterizing or exporting SVG.
* resolution: (raster output only) size in pixels of the longest dimension of the view rectangle when outputting to raster.
* antialiasing_level: (raster output only) must be [0..3]. Zero means don't antialias. Three means AA alot but will cause rasterization to be slower.
* colors: (raster output only) color table. The color of a given segment in the number of circles that contain that segment modulo the number of colors.
* view:  (raster output only) region in unscaled logical units, i.e. in the same units as the seeds, of the region to rasterize.
