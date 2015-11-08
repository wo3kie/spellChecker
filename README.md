# spellChecker  
Implementation of spell checker in C++  

## license  
For license please refer to LICENSE file  

## requirements  
C++11  

## how to build it?  
```{r, engine='bash'}
make  
```

## how to run it?  
```{r, engine='bash'}
$ ./sc english
? splel
spell
spool
sole
? hcekcre
checker
```

You can use it together with *rlwrap* to get history and easy editing  
  
```{r, engine='bash'}
$ rlwrap ./sc english
```
  
## performance
spellChecker most of the time can return suggestions to you in less than 1 millisecond  
```{r, engine='bash'}
$ ./sc english
? a
5µs
? by
142µs
? cad
173µs
? boys
248µs
? empty
260µs
? sister
763µs
? England
584µs
? mitigate
311µs
? Alexander
326µs
? zoologists
1239µs
? Bournemouth
382µs
? Indianapolis
706µs
? Liechtenstein
369µs
? Mephistopheles
450µs
```

but sometimes you can find a word when processing takes a few milliseconds  

```{r, engine='bash'}
$ ./sc english
? abracadabra
7968µs
```

## todo  
* Better memory management  
* Support for *unicode* and polish language  
* Reduce memory usage  
* Code refactoring :)  

