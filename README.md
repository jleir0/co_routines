# Coroutines
## What is?
A coroutine is a function that can suspend execution to be resumed later. 
Coroutines are stackless: they suspend execution by returning to the caller and the data that is required to resume execution is stored separately from the stack. This allows for sequential code that executes asynchronously, and also supports algorithms on lazy-computed infinite sequences and other uses.

A function is a coroutine if its definition does any of the following operators: co_return, co_yield amd co_await.

This is not a coroutine, is a normal function which prints "Hello world":

    void foo(){
        std::cout << "Hello world";
    }

    int main() {
        foo();    
    }

But this is not a coroutine either, it will cause a compile error.

      void foo(){
          co_return "Hello world"; //Neither with co_wait or co_yield
      }

      int main() {
          foo();    
      }
      
Every coroutine must have a return type that satisfies a number of requirements

## Requirements
Each coroutine is associated with the promise object, the coroutine handle and the coroutine state.
### Promise Object
This object is manipulated from inside the coroutine. The coroutine submits its result or exception through this object.
An example:

    struct coroutine {
      struct promise_object {
        coroutine get_return_object() { return {}; }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() {}
      };
    };

All the functions inside the promise object are mandatory, let´s explain each one.

- get_return_object() to obtain the object that is passed back to the caller.
- std::suspend_never initial_suspend() the coroutine keeps running until the first suspending co_await. This is the model for “hot-start” coroutines which execute synchronously during their construction and don’t return an object until the first suspension inside the function body.
- final_suspend after the coroutine function body has finished
When a coroutine reaches a suspension point the return object obtained earlier is returned to the caller/resumer, after implicit conversion to the return type of the coroutine, if necessary.
- return_value or return_void to define what the coroutine returns.
- if the coroutine ends with an uncaught exception, catches the exception and calls unhandled_exception() from within the catch-block
### Coroutine Handle
The coroutine handle, manipulated from outside the coroutine. This is used to resume execution of the coroutine or to destroy the coroutine frame.
An example:

