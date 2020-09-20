# Performance Lab
_by Kai Schuyler Gonzalez_

### Original Median CPE: ~1600, Final Median CPE: ~80

## Total Number of Calls/Pixel In Original
* `filter->getsize()` is being called in each iteration of 
```
for (int j = 0; j < filter -> getSize(); j++) {
  for (int i = 0; i < filter -> getSize(); i++) {
      output -> color[plane][row][col] = output -> color[plane][row][col]
      +  input -> color[plane][row + i - 1][col + j - 1] * filter -> get(i, j);
  }
}
```
* This is nested in the planes for loop, which is nested in the cols for loop, which is nested in the rows for loop. This chunk of code executes `rows * cols * 3` times, and since the filter size will always be 3, `filter->getSize()` and `filter->get(i,j)` executes `rows * cols * 3 * 9` times. 
* `filter->getDivisor()` executes `rows * cols` times. because it is inside the `rows` for-loop.
* NOTE: rows and cols both equal `1024` in the `blocks-small.bmp` file.
* Decreasing the number of function calls/pixel increases efficency because it makes it so the program does not have to access the same memory at each iteration of the loop.

## Loop Inefficiencies
* move the calculations of `input->width` and `input->height` outside of the for loops because these values will not change, they should only be calculated once.

## Procedure Calls
* move `filter->getsize()` and `filter->getDivisor()` function calls outside the loops because they only have to be called once and are causing overhead and blocking program optimization.

## Memory References
* make temporary accumulator variable to prevent excessive reading and writing memory multiple times per iteration. Set output to accumulator variable at the end of the loop.

## Change Max Dimensions in `cs1300bmp.h`
* from: `#define MAX_DIM 8192`
* to: `#define MAX_DIM 1025`
* This is an estimate of the max size of rows/colums for file `blocks-small.bmp`
* The actual resolution of `blocks-small.bmp` is 1024 x 1024

## Compile with `-g++` and `-02` flag
`CXXFLAGS= -g -O2 -fno-omit-frame-pointer -Wall`
* `O2` is the optimization level g++ will compile the program with

## Inline get\set functions from `Filter.cpp` at the top of `FilterMain.cpp`
* If a function is inline, the compiler places a copy of the code of that function at each point where the function is called at compile time. This saves time because it doesnt have to reference the code far away every time a `get` or `set` function is called.
```
inline int Filter::get(int r, int c){return data[ r * dim + c ];}
inline int Filter::getDivisor(){return divisor;}
inline int Filter::getSize() {return dim;}
inline void Filter::set(int r, int c, int value) { data[ r * dim + c ] = value; }
inline void Filter::setDivisor(int value) { divisor = value; }
```

## Taking variable calculation outside of for loop (Eliminating Loop Inefficiencies / Reducting Procedure Calls)
* In lines 113 and 114 of the FilterMain.cpp (the two inner for loops that loop through the filter) `filter->getSize()` and `filter->getDivisor()` get executed every single iteration of the loop. Instead of having these functions called every iteration, I moved its call outside of the for loop since they remain constant throughout the iteration. This was also done with `input->width` and `input->height` variables to reduce Loop Inefficiencies.

## Loop Unrolling Plane for-loop (Accumulating)
* Perform loop unrolling on plane loop. Instead of calculating the filter for red, green and then blue, calculate the output values for all 3 planes at the same time.

## Loop Unroll Cols for-loop (Multiple Accumulators)
* Perform one loop unroll on the cols for-loop. Calculate the current and next row output calculations at the same time to reduce the number of iterations of this loop. Chose cols instead of Rows because it is the innermost loop.

## Swap col and row loops
* In C, matrices are stored in Row-Major order, meaning that all elements across a row are accessed before moving to the next row. Originally, `applyFilter` was arranged such that the outer loop was going through the columns and the inner loop was going through the rows. This means that in memory the elements are stored the opposite of how they are accessed by the loop. Reversing this significantly improved performance.

### Data Structure Size and Cache
* Programs tend to use data and instructions with addresses near or equal to those they have used recently.
    * temporal and spatial locality
* Cache-Friendly Code:
    * Make common case go fast
        * focus on inner loops of the core functions
    * Minimize misses in the inner loops
        * repeated references to variables are good (temporal locality)
        * stride-1 reference patters are good (spatial locality)
    * good spacial locality equates to less cache misses because there is less of a chance of jumping memory hierarchies.