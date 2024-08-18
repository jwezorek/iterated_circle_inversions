# iterated_circle_inversions

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
 
