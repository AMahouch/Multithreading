# Multithreading

Calculating the minimum, maximum, and mean angular distance between 30,000 stars in the [Tycho Star Catalogue](https://www.cosmos.esa.int/web/hipparcos/tycho-2). Can be run either serially or multithreaded by specifying a ```-t``` parameter.

This program refactors the code to support multithreading, ultimately allowing the program to execute in parallel and optimizing time efficiency.

Utilizes C's ```pthread.h``` library. Detailed analysis available in the [report PDF](https://github.com/AMahouch/Multithreading/blob/main/Multithreading%20Report.pdf).


### Building and Running Program
```
> make
> time ./findAngular -t n
```
where ```n``` is the number of threads.
