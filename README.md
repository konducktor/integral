**Final project for C/ASM course** – a CLI app that calculates the area of ​​a plane figure bounded by three curves with a given accuracy ε.

### Default functions and custom build
By default, these three functions will be used for calculating area:
   1. $f_1 = 1 + \frac{4}{x^2+1}$
   2. $f_2 = x^3$
   3. $f_3 = 2^{-x}$
   
You make use a `custom` build, to which you pass the name of the file with functions via `SPEC_FILE`:
```
make custom SPEC_FILE=in.txt
```

### Approximate equation solutions
The tangent (Newton) method is used by default for approximate equation solutions.
A bisection method is also implemented. You can select one at the program build stage:
```
make BISECTION=1
```
You can use `--method` to check:
```
./integral -m
```

4. **Quadrature formulas:** rectangle formula.
