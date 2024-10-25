# Clang-tidy checker CWE-587

## About

This branch contains a checker for clang-tidy - `misc-pointer-fixed-address`. The checker detects and warns when a pointer has been assigned a fixed address, which is a vulnerability of [CWE-587](https://cwe.mitre.org/data/definitions/587.html).

Possible cases where a fixed address can be detected:

- Operation with fixed address pointer
- Initializating pointer with the fixed address
- Return fixed address
- Fixed address in argument of function
- Fixed address in class initialization list
- Fixed address in constructor class

## Build

This fork of llvm-project can be build like regular llvm-project with cmake. For example, with this command:

```bash
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../install -DLLVM_TARGETS_TO_BUILD="X86" -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_USE_SPLIT_DWARF=ON -DBUILD_SHARED_LIBS=ON ../llvm

ninja
```

## Usage

Checker can be enabled by this name in `-checks=` flag or in `.clang-tidy` file

```
misc-pointer-fixed-address
```

No other settings is required.

Command to run llvm-lit tests:

```bash
./build/bin/llvm-lit -v ./clang-tools-extra/test/clang-tidy/checkers/misc/pointer-fixed-address.cpp
```

## Possible detections

### Below are examples with reason and lines that caused the warnings.

#### Operation with pointer with fixed address:

```cpp
void example() {
    int *Ptr;
    Ptr = (int *)0xABCD4321; // warn
    Ptr = 5 + (int *)0xDEF54321; // warn
}
```

#### Initializing the pointer with the fixed address:

```cpp
void example(int *ptr = (int *)0x09876554) { // warn
  int *Ptr1 = (int *)0xABCD4321; // warn
  int *Ptr2((int *)0xABCD4321); // warn
  int *Ptr3{(int *)0xABCD4321}; // warn
}
```

#### The return value of a pointer is a fixed address:

```cpp
int *exampleGet() {
    return (int *)0x1234ABCD; // warn
}
```

#### Fixed address in argument of function:

```cpp
void exampleSet(int *ptr) {}

void example() {
    exampleSet((int *)0x1234ABCD); // warn
}
```

#### Fixed address in class field and initialization list:

```cpp
class ExampleClass {
public:
  int *Ptr1;
  int *Ptr2{(int *)0x1234ABCD}; // warn
  int *Ptr3 = (int *)0x1234ABCD; // warn
  ExampleClass(int *Ptr) : Ptr1((int *)0x1234ABCD) {} // warn
};
```

#### Fixed address in constructor class:

```cpp
void example() {
  ExampleClass Object1 = ExampleClass((int *)0xABCD4321); // warn
  ExampleClass *Object2 = new ExampleClass((int *)0xABCD4321); // warn
}
```

---

Macros are also supported correctly

```cpp
#define FIXED_ADDRESS (int *)0xABCD4321
#define NOT_FIXED_ADDRESS 0

void Function() {
    int *Ptr(FIXED_ADDRESS); // warn
    Ptr = NOT_FIXED_ADDRESS;
    Ptr = FIXED_ADDRESS + 5; // warn
}
```

## What has been done

After building llvm-project, the new checker was added with this command:

```bash
./clang-tools-extra/clang-tidy/add_new_check.py misc pointer-fixed-address
```

That created 2 files in `llvm-project/clang-tools-extra/clang-tidy/misc/` directory - `PointerFixedAddressCheck.cpp` and `PointerFixedAddressCheck.h`. In `PointerFixedAddressCheck.cpp` added logic in `void PointerFixedAddressCheck::registerMatchers(MatchFinder *Finder)` method to match some cases with pointers. After, in `PointerFixedAddressCheck.h` defined private function `bool PointerFixedAddressCheck::isPointerAddressFixed(const Expr *RVal);` to check all expressions with logic in `PointerFixedAddressCheck.cpp`. Further was added logic in `void PointerFixedAddressCheck::check(const MatchFinder::MatchResult &Result)` to check that has been matched and warning, if come of elements is fixed pointer. In the end, was added LIT tests in `llvm-project/clang-tools-extra/test/clang-tidy/checkers/misc/pointer-fixed-address.cpp` file.

## Examples of checker usage and work

### Example with functions

Sample content:

```cpp
#define FIXED_ADDRESS (int *)0x1234ABCD

int *getPtrFunc() { return (int *)0x1234ABCD; }

void setPtrFunc(int *Ptr = (int *)0xABCD1234) { Ptr = (int *)0xABCD1234; }

void Function() {
  int *Ptr1 = (int *)0xABCD4321;
  int Num1 = 4;
  Ptr1 = &Num1;
  Ptr1 = *((int *)(int *)&Num1) + (int *)0x1234ABCD;

  setPtrFunc(FIXED_ADDRESS);
  int *Ptr2{(int *)0x0623dcab};
  int *Ptr3(getPtrFunc());
}

```

Start command:

```bash
% ../llvm-project/build/bin/clang-tidy -checks="-*,misc-pointer-fixed-address" ./sample_1.cpp --
```

Output:

```bash
7 warnings generated.
/home/dedligma/Documents/repos/samples/sample_1.cpp:3:21: warning: The return value of a pointer is a fixed address [misc-pointer-fixed-address]
    3 | int *getPtrFunc() { return (int *)0x1234ABCD; }
      |                     ^
/home/dedligma/Documents/repos/samples/sample_1.cpp:5:22: warning: Initializing the pointer with the fixed address [misc-pointer-fixed-address]
    5 | void setPtrFunc(int *Ptr = (int *)0xABCD1234) { Ptr = (int *)0xABCD1234; }
      |                      ^
/home/dedligma/Documents/repos/samples/sample_1.cpp:5:55: warning: Operation with pointer with fixed address [misc-pointer-fixed-address]
    5 | void setPtrFunc(int *Ptr = (int *)0xABCD1234) { Ptr = (int *)0xABCD1234; }
      |                                                       ^
/home/dedligma/Documents/repos/samples/sample_1.cpp:8:8: warning: Initializing the pointer with the fixed address [misc-pointer-fixed-address]
    8 |   int *Ptr1 = (int *)0xABCD4321;
      |        ^
/home/dedligma/Documents/repos/samples/sample_1.cpp:11:35: warning: Operation with pointer with fixed address [misc-pointer-fixed-address]
   11 |   Ptr1 = *((int *)(int *)&Num1) + (int *)0x1234ABCD;
      |                                   ^
/home/dedligma/Documents/repos/samples/sample_1.cpp:13:14: warning: The pointer in the argument has a fixed address [misc-pointer-fixed-address]
   13 |   setPtrFunc(FIXED_ADDRESS);
      |              ^
/home/dedligma/Documents/repos/samples/sample_1.cpp:1:23: note: expanded from macro 'FIXED_ADDRESS'
    1 | #define FIXED_ADDRESS (int *)0x1234ABCD
      |                       ^
/home/dedligma/Documents/repos/samples/sample_1.cpp:14:8: warning: Initializing the pointer with the fixed address [misc-pointer-fixed-address]
   14 |   int *Ptr2{(int *)0x0623dcab};
      |        ^

```

### Example with class

Sample content:

```cpp
class ExampleClass {
public:
  int *Ptr1;
  int *Ptr2{(int *)0x1234ABCD};
  int *Ptr3 = ;
  ExampleClass(int *Ptr) : Ptr1((int *)0x1234ABCD) {}
};

void example() {
  ExampleClass Object1 = ExampleClass((int *)0xABCD4321);
  ExampleClass *Object2 = new ExampleClass((int *)0xABCD4321);
  delete Object2;
}
```

Start command:

```bash
% ../llvm-project/build/bin/clang-tidy -checks="-*,misc-pointer-fixed-address" ./sample_2.cpp --
```

Output:

```bash
5 warnings generated.
/home/dedligma/Documents/repos/samples/sample_2.cpp:6:8: warning: Field in class has initialization with fixed address [misc-pointer-fixed-address]
    6 |   int *Ptr2{FIXED_ADDRESS};
      |        ^
/home/dedligma/Documents/repos/samples/sample_2.cpp:7:8: warning: Field in class has initialization with fixed address [misc-pointer-fixed-address]
    7 |   int *Ptr3 = (int *)0x1234ABCD;
      |        ^
/home/dedligma/Documents/repos/samples/sample_2.cpp:8:3: warning: The initialization list contains a fixed pointer address [misc-pointer-fixed-address]
    8 |   ExampleClass(int *Ptr) : Ptr1((int *)0x1234ABCD) {}
      |   ^
/home/dedligma/Documents/repos/samples/sample_2.cpp:12:26: warning: Constructor for class contains a fixed pointer address [misc-pointer-fixed-address]
   12 |   ExampleClass Object1 = ExampleClass((int *)0xABCD4321);
      |                          ^
/home/dedligma/Documents/repos/samples/sample_2.cpp:13:31: warning: Constructor for class contains a fixed pointer address [misc-pointer-fixed-address]
   13 |   ExampleClass *Object2 = new ExampleClass((int *)0xABCD4321);
      |                               ^
```
