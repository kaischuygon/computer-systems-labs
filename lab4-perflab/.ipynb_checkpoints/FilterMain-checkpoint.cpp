#include <stdio.h>
#include "cs1300bmp.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "Filter.h"

using namespace std;

#include "rdtsc.h"

// INLINE 
inline int Filter::get(int r, int c){return data[ r * dim + c ];}
inline int Filter::getDivisor(){return divisor;}
inline int Filter::getSize() {return dim;}
// END

//
// Forward declare the functions
//
Filter * readFilter(string filename);
double applyFilter(Filter *filter, cs1300bmp *input, cs1300bmp *output);

int main(int argc, char **argv) {

  if ( argc < 2) {
    fprintf(stderr,"Usage: %s filter inputfile1 inputfile2 .... \n", argv[0]);
  }

  //
  // Convert to C++ strings to simplify manipulation
  //
  string filtername = argv[1];

  //
  // remove any ".filter" in the filtername
  //
  string filterOutputName = filtername;
  string::size_type loc = filterOutputName.find(".filter");
  if (loc != string::npos) {
    //
    // Remove the ".filter" name, which should occur on all the provided filters
    //
    filterOutputName = filtername.substr(0, loc);
  }

  Filter *filter = readFilter(filtername);

  double sum = 0.0;
  int samples = 0;

  for (int inNum = 2; inNum < argc; inNum++) {
    string inputFilename = argv[inNum];
    string outputFilename = "filtered-" + filterOutputName + "-" + inputFilename;
    struct cs1300bmp *input = new struct cs1300bmp;
    struct cs1300bmp *output = new struct cs1300bmp;
    int ok = cs1300bmp_readfile( (char *) inputFilename.c_str(), input);

    if ( ok ) {
      double sample = applyFilter(filter, input, output);
      sum += sample;
      samples++;
      cs1300bmp_writefile((char *) outputFilename.c_str(), output);
    }
    delete input;
    delete output;
  }
  fprintf(stdout, "Average cycles per sample is %f\n", sum / samples);

}

class Filter *readFilter(string filename) {
  ifstream input(filename.c_str());

  if ( ! input.bad() ) {
    int size = 0;
    input >> size;
    Filter *filter = new Filter(size);
    int div;
    input >> div;
    filter -> setDivisor(div);
    for (int i=0; i < size; i++) {
      for (int j=0; j < size; j++) {
	int value;
	input >> value;
	filter -> set(i,j,value);
      }
    }
    return filter;
  } else {
    cerr << "Bad input in readFilter:" << filename << endl;
    exit(-1);
  }
}


double applyFilter(class Filter *filter, cs1300bmp *input, cs1300bmp *output) {

  long long cycStart, cycStop;

  cycStart = rdtscll();

  output->width = input->width;
  output->height = input->height;

  // TAKING VARIABLE CALC OUT OF FOR LOOPS
  int width = input->width;
  int height = input->height;
  int size = filter->getSize();
  int divisor = filter->getDivisor();
  // END
  for(int row = 1; row < height - 1; row += 2) {
    for(int col = 1; col < width - 1; col += 2) {
      int tempRed1,tempRed2 = 0;
      int tempGreen1,tempGreen2 = 0;
      int tempBlue1 = 0;

      for (int i = 0; i < size; i++) { 
        for (int j = 0; j < size; j++) {
          tempRed1 += input->color[COLOR_RED][row + i - 1][col + j - 1] * filter->get(i, j);
          tempGreen1 += input->color[COLOR_GREEN][row + i - 1][col + j - 1] * filter->get(i, j);
          tempBlue1 += input->color[COLOR_BLUE][row + i - 1][col + j - 1] * filter->get(i, j);
        }
      }

      tempRed1 /= divisor;
        if (tempRed1 < 0){tempRed1 = 0;}
        if (tempRed1 > 255){tempRed1 = 255;}
        output->color[COLOR_RED][row][col] = tempRed1;
      tempGreen1 /= divisor;
        if (tempGreen1 < 0){tempGreen1 = 0;}
        if (tempGreen1 > 255){tempGreen1 = 255;}
        output->color[COLOR_GREEN][row][col] = tempGreen1;
      tempBlue1 /= divisor;
        if (tempBlue1 < 0){tempBlue1 = 0;}
        if (tempBlue1 > 255){tempBlue1 = 255;}
        output->color[COLOR_BLUE][row][col] = tempBlue1;
    }
  }
  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}

for(int row = 1; row < height - 1; row += 1) {
  for(int col = 1; col < width - 1; col += 1) {
    int tempRed1 = 0;
    int tempGreen1 = 0;
    int tempBlue1 = 0;

    for (int i = 0; i < size; i++) { 
      for (int j = 0; j < size; j++) {
        tempRed1 += input->color[COLOR_RED][row + i - 1][col + j - 1] * filter->get(i, j);
        tempGreen1 += input->color[COLOR_GREEN][row + i - 1][col + j - 1] * filter->get(i, j);
        tempBlue1 += input->color[COLOR_BLUE][row + i - 1][col + j - 1] * filter->get(i, j);
      }
    }

    tempRed1 /= divisor;
      if (tempRed1 < 0){tempRed1 = 0;}
      if (tempRed1 > 255){tempRed1 = 255;}
      output->color[COLOR_RED][row][col] = tempRed1;
    tempGreen1 /= divisor;
      if (tempGreen1 < 0){tempGreen1 = 0;}
      if (tempGreen1 > 255){tempGreen1 = 255;}
      output->color[COLOR_GREEN][row][col] = tempGreen1;
    tempBlue1 /= divisor;
      if (tempBlue1 < 0){tempBlue1 = 0;}
      if (tempBlue1 > 255){tempBlue1 = 255;}
      output->color[COLOR_BLUE][row][col] = tempBlue1;
  }
}