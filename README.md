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

## Elements of coroutines
Each coroutine is associated with the promise object, the coroutine handle and the coroutine state.
### Promise Object
This object is manipulated from inside the coroutine. The coroutine submits its result or exception through this object.
An example:

        struct return_object {
            struct promise_type {
                return_object get_return_object() { return {}; }
                std::suspend_never initial_suspend() { return {}; }
                std::suspend_never final_suspend() noexcept { return {}; }
                void unhandled_exception() {}               
                void return_void(){}
            };
        };

All the functions inside promise_type are mandatory, let´s explain each one.

- get_return_object() to obtain the object that is passed back to the caller.
- std::suspend_never initial_suspend() the coroutine keeps running until the first suspending co_await. This is the model for “hot-start” coroutines which execute synchronously during their construction and don’t return an object until the first suspension inside the function body.
- final_suspend after the coroutine function body has finished
When a coroutine reaches a suspension point the return object obtained earlier is returned to the caller/resumer, after implicit conversion to the return type of the coroutine, if necessary.
- return_value or return_void to define what the coroutine returns.
- if the coroutine ends with an uncaught exception, catches the exception and calls unhandled_exception() from within the catch-block
### Coroutine Handle
The coroutine handle, manipulated from outside the coroutine. This is used to resume execution of the coroutine or to destroy the coroutine frame.
An example:

        #include <concepts>
        #include <coroutine>
        #include <exception>
        #include <iostream>

        //definition of the return object and the promise type
        struct return_object {
            struct promise_type {
                return_object get_return_object() { return {}; }
                std::suspend_never initial_suspend() { return {}; }
                std::suspend_never final_suspend() noexcept { return {}; }
                void return_void() {}
                void unhandled_exception() {}
            };
        };

        //this is a coroutine, the return type satisfy the requirements and inside is co_return operator.
        return_object foo()
        {
            co_return; 
            /* destroys all variables with automatic storage duration in reverse order they were created.
            Calls promise_type.final_suspend() and co_awaits the result */           
        }

        int main()
        {
            //we create a handle of type coroutine_handle
            std::coroutine_handle<> handle;            
            //this coroutine do nothing
            foo();
            //we can manipulate the hanlde from outside the coroutine
            handle.resume();
            handle.destroy();
        }

The hanlde is like a pointer to the coroutine state, so we can change the value of any parameter in that state and the handle will remain the same. To do that we will use co_await but first we need to understand the coroutine states.
### Coroutine state
The coroutine state is an internal heap-allocated object that contains:
- the promise object 
- the parameters 
- the current suspension point
- local variables whose lifetime spans the current suspension point
## co_await
The unary operator co_await suspends a coroutine and returns control to the caller.
### Awaitable object
We must use the expression "co_await expr;" where "expr" is the awaitable object or awaiter. The awaiter has three methods:
- await_ready is an optimization, if it returns true, then co_await does not suspend the function. The <coroutine> header provides two pre-defined awaiters, std::suspend_always and std::suspend_never. As their names imply, suspend_always::await_ready always returns false, while suspend_never::await_ready always returns true. 
- await_suspend Store the coroutine handle every time await_suspend is called.
- await_resume returns the value of the co_await expression. 

An example:
    
        #include <concepts>
        #include <coroutine>
        #include <exception>
        #include <iostream>

        //definition of the return object and the promise type
        struct return_object {
            struct promise_type {
                return_object get_return_object() { return {}; }
                std::suspend_never initial_suspend() { return {}; }
                std::suspend_never final_suspend() noexcept { return {}; }
                void return_void() {}
                void unhandled_exception() {}
            };
        };

        struct awaiter {
            std::coroutine_handle<> *handle_;
            constexpr bool await_ready() const noexcept { return false; }
            void await_suspend(std::coroutine_handle<> handle) { *handle_ = handle; }
            constexpr void await_resume() const noexcept {}
        };

        //Coroutine using co_await
        return_object foo(std::coroutine_handle<> *handle)
        {
            //pass the handler to the await_suspend method
            awaiter wait{handle};
            for (int i = 1;; ++i) {
                co_await wait; //suspends the coroutine and returns control to the caller.
                std::cout << "It´s the " << i << "th time in coroutine" << std::endl;                
            }          
        }

        int main()
        {
            //we create a handle of type coroutine_handle
            std::coroutine_handle<> handle;            
            //pass the control of the handler to foo
            foo(&handle);
            for (int i = 1; i < 4; ++i) {
                std::cout << "It´s the " << i << "th time in main function" << std::endl;
                //handle() triggers one more iteration of the loop in counter
                handle();
            }
            //To avoid leaking memory, destroy coroutine state 
            handle.destroy();
        }

This is the output:
                                                                                        
        It´s the 1th time in main function
        It´s the 1th time in coroutine
        It´s the 2th time in main function
        It´s the 2th time in coroutine
        It´s the 3th time in main function
        It´s the 3th time in coroutine                                                                     
                                                                                        
You can avoid to use the awaitable by creating the handle in the return_object and returning it to the caller using the get_return_object method from promise_type.
An example:
                 
        #include <concepts>
        #include <coroutine>
        #include <exception>
        #include <iostream>

        struct return_object {
                struct promise_type {
                        return_object get_return_object() {
                                return {
                                        //return the handle
                                        .handle_ = std::coroutine_handle<promise_type>::from_promise(*this)
                                };
                        }
                        std::suspend_never initial_suspend() { return {}; }
                        std::suspend_never final_suspend() noexcept { return {}; }
                        void unhandled_exception() {}
                };
                //create the handle
                std::coroutine_handle<promise_type> handle_;
                operator std::coroutine_handle<promise_type>() const { return handle_; }
                operator std::coroutine_handle<>() const { return handle_; }
        };

        return_object foo()
        {
                for (int i = 1;; ++i) {
                        co_await std::suspend_always{}; 
                        std::cout << "It´s the " << i << "th time in coroutine" << std::endl;
                }          
        }

        int main()
        {
                //we create a pointer to a handle
                std::coroutine_handle<> handle = foo();   
                for (int i = 1; i < 4; ++i) {
                        std::cout << "It´s the " << i << "th time in main function" << std::endl;
                        handle();
                }
                handle.destroy();
        }       
The output is the same.
### Transmit info                                                                                            
What we did till now is to pass the control from the caller to the coroutine but we can send just the info in the promise object to main by changing the handle.
An example:
           
        #include <concepts>
        #include <coroutine>
        #include <exception>
        #include <iostream>

        template<class promise_object>
        struct awaiter {
                //we will send the promise oject in place of the handle
                promise_object *promise_;
                bool await_ready() { return false; }
                bool await_suspend(std::coroutine_handle<promise_object> handle) {
                        promise_ = &handle.promise();
                        return false; //to don´t suspend the coroutine till the promise is set in the coroutine.   
                }
                promise *await_resume() { return promise_; }
        };

        struct return_object {
                struct promise_type {
                        //add something to send, the value is set in the coroutine
                        std::string message;
                        return_object get_return_object() {
                                return {
                                        .handle_ = std::coroutine_handle<promise_type>::from_promise(*this)
                                };
                        }
                        std::suspend_never initial_suspend() { return {}; }
                        std::suspend_never final_suspend() noexcept { return {}; }
                        void unhandled_exception() {}
                };
                std::coroutine_handle<promise_type> handle_;
                operator std::coroutine_handle<promise_type>() const { return handle_; }
        };

        //Coroutine using co_await
        return_object foo()
        {
                auto pointer_promise = co_await awaiter<return_object::promise_type>{};
                for (int i = 1;; ++i) {
                        //set the value
                        pointer_promise->message = "It´s the " +  std::to_string(i) + "th time in coroutine";
                        //suspend the coroutine now the value is set
                        co_await std::suspend_always{}; 
                }          
        }

        int main()
        {
                //create a pointer to a handle
                std::coroutine_handle<return_object::promise_type> handle = foo();
                //from this pointer handle only need the promise object
                return_object::promise_type &promise = handle.promise();        
                for (int i = 1; i < 4; ++i) {
                        std::cout << "It´s the " << i << "th time in main function" << std::endl;
                        //print the promise param that we set in the coroutine                                                                        
                        std::cout << promise.message << std::endl;
                        handle();
                }
                handle.destroy();
        }
The output is the same.
## co_yield
Suspend execution returning a value, so using co_yeild, we can simplify the previous example by adding a yield_value method to the promise_type inside our return object.
An example:
                                                                 
        #include <concepts>
        #include <coroutine>
        #include <exception>
        #include <iostream>

        struct return_object {
                struct promise_type {
                        std::string message;
                        return_object get_return_object() {
                                return {
                                        .handle_ = std::coroutine_handle<promise_type>::from_promise(*this)
                                };
                        }
                        std::suspend_never initial_suspend() { return {}; }
                        std::suspend_never final_suspend() noexcept { return {}; }
                        void unhandled_exception() {}
                        //by addding this method we can modify the promise_object values to transmit
                        std::suspend_always yield_value(auto value) {
                                //store in message the value passed by co_yield
                                message = value;
                                return {};
                        }
                };
                std::coroutine_handle<promise_type> handle_;
        };

        //Coroutine using co_yield
        return_object foo()
        {
                for (int i = 1;; ++i) {
                        auto value = "It´s the " +  std::to_string(i) + "th time in coroutine";
                        co_yield value;
                }          
        }

        int main()
        {
                auto handle = foo().handle_;        
                auto &promise = handle.promise();       
                for (int i = 1; i < 4; ++i) {
                        std::cout << "It´s the " << i << "th time in main function" << std::endl;
                        std::cout << promise.message << std::endl;
                        handle();
                }
                handle.destroy();
        } 
                                                     
## co_return
co_return signal the end of a coroutine, there are three ways for a coroutine to signal that it is complete:
- “co_return value;” to return a final value.
- “co_return;” to end the coroutine without a final value.
- let execution fall off the end of the function.
                                                                 

An example:
                                                                 
        #include <concepts>
        #include <coroutine>
        #include <exception>
        #include <iostream>

        struct return_object {
                struct promise_type {
                        std::string message;
                        return_object get_return_object() {
                                return {
                                        .handle_ = std::coroutine_handle<promise_type>::from_promise(*this)
                                };
                        }
                        std::suspend_never initial_suspend() { return {}; }
                        std::suspend_always final_suspend() noexcept { return {}; }
                        void unhandled_exception() {}
                        std::suspend_always return_value(auto value) {
                                //the value passed with co_return is stored in message
                                message = value;
                                return {};
                        }
                };
                std::coroutine_handle<promise_type> handle_;
        };

        //Coroutine using co_return
        return_object foo()
        {
                //complete execution returning a value
                co_return "It´s in coroutine";        
        }

        int main()
        {
                auto handle = foo().handle_;     
                //at this point coroutin is suspended at its final suspend point,        
                std::cout << handle.done() << std::endl;    
                auto &promise = handle.promise();       
                std::cout << "It´s in main function" << std::endl;
                std::cout << promise.message << std::endl;    
                handle.destroy();
        }
        
This is the output
        
        1
        It´s in main function
        It´s in coroutine

## co_yield & co_return example
        
        #include <concepts>
        #include <coroutine>
        #include <exception>
        #include <iostream>

        struct return_object {
                struct promise_type {
                        std::string message;
                        return_object get_return_object() {
                                return {
                                        .handle_ = std::coroutine_handle<promise_type>::from_promise(*this)
                                };
                        }
                        std::suspend_never initial_suspend() { return {}; }
                        std::suspend_always final_suspend() noexcept { return {}; }
                        void unhandled_exception() {}
                        std::suspend_always yield_value(auto value) {
                                message = value;
                                return {};
                        }
                        std::suspend_always return_value(auto value) {
                                message = value;
                                return {};
                        }
                };
                std::coroutine_handle<promise_type> handle_;
        };

        return_object foo()
        {
            for (int i = 1; i < 4; ++i) {
                auto value = "It´s the " +  std::to_string(i) + "th time in coroutine";
                co_yield value;
            }   
            co_return "It´s last time in coroutine";         
        }

        int main()
        {
                auto handle = foo().handle_;                  
                auto &promise = handle.promise();                    
                std::cout << "It´s in main function" << std::endl;  
                while(!handle.done()){                    
                    std::cout << promise.message << std::endl; 
                    handle();
                }      
                std::cout << promise.message << std::endl; 
                handle.destroy();
        }
                                                         
The output is
                                                         
        It´s in main function
        It´s the 1th time in coroutine
        It´s the 2th time in coroutine
        It´s the 3th time in coroutine
        It´s last time in coroutine                                                         
## Example
In the next example we will code to produce a state machine with the next state diagram 
![Diagrama en blanco](https://user-images.githubusercontent.com/40487776/168542986-c76fbd0b-2871-4bbb-963c-2c68a4a0d10d.png)
                                              
