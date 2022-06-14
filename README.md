# Roll
A simple cli app for rolling dice.

## Building
A meson.build, CMakeLists.txt and a Makefile are provided. Makefile works with clang or gcc.

## Usage
```
roll: A program for rolling dice.

usage: roll dice ... [-v | --verbose]

Early Out Arguments:
--------------------
-h, --help:
    Print this help and exit. 

Positional Arguments:
---------------------
dice: string
    The dice expression to roll. 

Keyword Arguments:
------------------
-v, --verbose: flag
    Display the individual dice rolls instead of just the total. 
```

```
$ roll d20 + 4
[13] + 4 -> 17
```

```
$ roll
ctrl-d or "q" to exit
"v" toggles verbose output
Enter repeats last die roll
>> d20
[10] -> 10
>> d8
[8] -> 8
>> d4
[2] -> 2
>> q
```
