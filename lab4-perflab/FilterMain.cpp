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
inline void Filter::set(int r, int c, int value) { data[ r * dim + c ] = value; }
inline void Filter::setDivisor(int value) { divisor = value; }
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
  int filterSize = filter->getSize();
  int divisor = filter->getDivisor();
  int getFilter;
  int accRed1,accGreen1,accBlue1;
  int accRed2,accGreen2,accBlue2;
  for(int row = 1; row < height - 1; row += 1) {
    for(int col = 1; col < width - 1; col += 2) {
      accRed1 = accGreen1 = accBlue1 = 0;
      accRed2 = accGreen2 = accBlue2 = 0;
      for (int i = 0; i < filterSize; i+=1) { 
        for (int j = 0; j < filterSize; j+=1) {
          getFilter = filter->get(i,j);

          accRed1 += input->color[COLOR_RED][row + i - 1][col + j - 1] * getFilter;
          accGreen1 += input->color[COLOR_GREEN][row + i - 1][col + j - 1] * getFilter;
          accBlue1 += input->color[COLOR_BLUE][row + i - 1][col + j - 1] * getFilter;
          
          accRed2 += input->color[COLOR_RED][row + i - 1][col + j] * getFilter;
          accGreen2 += input->color[COLOR_GREEN][row + i - 1][col + j] * getFilter;
          accBlue2 += input->color[COLOR_BLUE][row + i - 1][col + j] * getFilter;
          }
      }
      accRed1 /= divisor;
        if (accRed1 < 0){accRed1 = 0;}
        if (accRed1 > 255){accRed1 = 255;}
        output->color[COLOR_RED][row][col] = accRed1;
      accGreen1 /= divisor;
        if (accGreen1 < 0){accGreen1 = 0;}
        if (accGreen1 > 255){accGreen1 = 255;}
        output->color[COLOR_GREEN][row][col] = accGreen1;
      accBlue1 /= divisor;
        if (accBlue1 < 0){accBlue1 = 0;}
        if (accBlue1 > 255){accBlue1 = 255;}
        output->color[COLOR_BLUE][row][col] = accBlue1;

      accRed2 /= divisor;
        if (accRed2 < 0){accRed2 = 0;}
        if (accRed2 > 255){accRed2 = 255;}
        output->color[COLOR_RED][row][col+1] = accRed2;
      accGreen2 /= divisor;
        if (accGreen2 < 0){accGreen2 = 0;}
        if (accGreen2 > 255){accGreen2 = 255;}
        output->color[COLOR_GREEN][row][col+1] = accGreen2;
      accBlue2 /= divisor;
        if (accBlue2 < 0){accBlue2 = 0;}
        if (accBlue2 > 255){accBlue2 = 255;}
        output->color[COLOR_BLUE][row][col+1] = accBlue2;
    }
  }
  cycStop = rdtscll();
  double diff = cycStop - cycStart;
  double diffPerPixel = diff / (output -> width * output -> height);
  fprintf(stderr, "Took %f cycles to process, or %f cycles per pixel\n",
	  diff, diff / (output -> width * output -> height));
  return diffPerPixel;
}