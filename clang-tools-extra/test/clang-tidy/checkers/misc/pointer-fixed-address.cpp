// RUN: %check_clang_tidy %s misc-pointer-fixed-address %t
namespace {
int *getPtrFunc() { return (int *)0x1234ABCD; } // CHECK-MESSAGES: :[[@LINE]]:21: warning: The return value of a pointer is a fixed address [misc-pointer-fixed-address]

void setPtrFunc(int *Ptr) { Ptr = (int *)0xABCD1234; } // CHECK-MESSAGES: :[[@LINE]]:35: warning: Operation with pointer with fixed address [misc-pointer-fixed-address]

class MyClass {
public:
  int *Ptr1;
  int *Ptr2{(int *)0x1234ABCD}; // CHECK-MESSAGES: :[[@LINE]]:8: warning: Field in class has initialization with fixed address [misc-pointer-fixed-address]
  int *Ptr3 = (int *)0x1234ABCD; // CHECK-MESSAGES: :[[@LINE]]:8: warning: Field in class has initialization with fixed address [misc-pointer-fixed-address]
  MyClass(int *Ptr) : Ptr1((int *)0x1234ABCD) { // CHECK-MESSAGES: :[[@LINE]]:3: warning: The initialization list contains a fixed pointer address [misc-pointer-fixed-address]
    Ptr2 = (int *)0x1234ABCD; // CHECK-MESSAGES: :[[@LINE]]:12: warning: Operation with pointer with fixed address [misc-pointer-fixed-address]
    Ptr3 = getPtrFunc();
    setPtrFunc((int *)0x1234ABCD); // CHECK-MESSAGES: :[[@LINE]]:16: warning: The pointer in the argument has a fixed address [misc-pointer-fixed-address]
  }
  int *getPtrMeth() { return (int *)0x1234ABCD; } // CHECK-MESSAGES: :[[@LINE]]:23: warning: The return value of a pointer is a fixed address [misc-pointer-fixed-address]
  void setPtrMeth(int *Ptr = (int *)0xABCD1234) {} // CHECK-MESSAGES: :[[@LINE]]:24: warning: Initializing the pointer with the fixed address [misc-pointer-fixed-address]
};

void Function() {
  MyClass Object1 = MyClass((int *)0xABCD4321); // CHECK-MESSAGES: :[[@LINE]]:21: warning: Constructor for class contains a fixed pointer address [misc-pointer-fixed-address]
  MyClass *Object2 = new MyClass((int *)0xABCD4321); // CHECK-MESSAGES: :[[@LINE]]:26: warning: Constructor for class contains a fixed pointer address [misc-pointer-fixed-address]
  Object1.getPtrMeth();
  Object2->setPtrMeth((int *)0xABCD4321); // CHECK-MESSAGES: :[[@LINE]]:23: warning: The pointer in the argument has a fixed address [misc-pointer-fixed-address]

  int *Ptr1 = (int *)0xABCD4321; // CHECK-MESSAGES: :[[@LINE]]:8: warning: Initializing the pointer with the fixed address [misc-pointer-fixed-address]
  int Num1 = 4;
  Ptr1 = &Num1;
  Ptr1 = (int *)(int *)&Num1;
  Ptr1 = (int *)0x1234ABCD; // CHECK-MESSAGES: :[[@LINE]]:10: warning: Operation with pointer with fixed address [misc-pointer-fixed-address]

  int *Ptr2{(int *)0x0623dcab}; // CHECK-MESSAGES: :[[@LINE]]:8: warning: Initializing the pointer with the fixed address [misc-pointer-fixed-address]
  int *Ptr3((int *)0x0623dcab); // CHECK-MESSAGES: :[[@LINE]]:8: warning: Initializing the pointer with the fixed address [misc-pointer-fixed-address]
}

} // namespace

