C++ Version of [CERN's BLonD code][1]

## STATUS

Currently under development

## INSTALATION

1. clone the code into a directory (lets call it home)
2. cd to `home\build`
3. run `cmake ..`
4. run `make`
5. The executables should be ready!

## Usage

The following environmental variables can be specified in order to dynamically alter the program's behaviour:

* N_THREADS : number of openmp threads that will be used for the simulation (default: 1)
* N_PARTICLES : number of macroparticles that will be simulated (default: 10k)
* N_TURNS : number of simulation turns (default: 10k)
* N_SLICES : number of slices that will be used to generate the beam's histogram (default: 100)

Example: `N_THREADS=4 N_TURNS=10000 ./testcase`

## LINKS

* Repository: https://gitlab.cern.ch/dquartul/BLonD
* Documentation: http://blond-documentation.web.cern.ch/
* Project website: http://blond.web.cern.ch

## DEVELOPERS

- Alexandre Lasheen (alexandre.lasheen (at) cern.ch)
- Juan Esteban Muller (juan.fem (at) cern.ch)
- Danilo Quartullo (danilo.quartullo (at) cern.ch)
- Helga Timko (Helga.Timko (at) cern.ch)
- Konstantinos Iliakis (konstantinos.iliakis (at) cern.ch)


[1]: http://blond.web.cern.ch