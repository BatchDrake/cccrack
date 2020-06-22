# cccrack

_A practical implementation of the [Marazin-Gautier-Burel](https://www.researchgate.net/publication/224093438_Dual_Code_Method_for_Blind_Identification_of_Convolutional_Encoder_for_Cognitive_Radio_Receiver_Design) method
to reverse-engineer non-punctured convolutional codes (i.e. rates of the form 1/n)_

---
`cccrack` accepts a plain text file containing the symbol stream represented as ASCII characters starting by `'0'`, eg: `00111010110001101`. 
The ASCII code for the i-th symbol can be calculated by adding `48 + i`, therefore a stream containing 4 bits per symbol could look like `32;>4:2==>4916:6=0321;54633><15<`

`cccrack` guesses the number of bits per symbol according to the range of symbols found in the input file, and applies the algorithm to all the <img src="https://render.githubusercontent.com/render/math?math=2^n!"> symbol-bit permutations to crack the code.
Although up to 6 bits per symbol are supported, nothing above 2 bits per symbol is likely to be feasible (3 bits per symbol yields to 40320 possible taggings).

This is the _hard_ implementation, i.e. the one based on hard decisions. Bit errors are not well tolerated by the algorithm and would yield to invalid results.

This application has been tested in GNU/Linux only, although it is likely to run in most Unix systems.

## Build cccrack
`cccrack` does not require any special library, apart from a working C compiler like gcc and the classical autotools (autoconf + automake). In order to build Ccrack, just run:

```
% autoreconf -i
% ./configure
% make
```

You can optionally run `make install` as root to install cccrack system-wide. Assuming your symbols are stored in a text file named `symbols.log`, just run:

```
% src/cccrack symbols.log
```

And `cccrack` will start testing different permutations, applying the Marazin-Gautier-Burel algorithm against each. Candidate rates and polynomials are printed to stdout, omitting non-Gray-coded taggings (this can be prevented by passing -n to `cccrack`).

Information on additional options can be obtained by running `cccrack --help`.
