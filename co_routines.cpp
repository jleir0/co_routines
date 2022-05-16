#include <coroutine>
#include <iostream>
#include <stdlib.h>
#include <math.h>

enum State { start, cooling, heating, charging, finish, standby};
static const char *enum_str[] = { "Start", "Cooling", "Heating", "charging", "Finish", "StandBy" };

struct message {
    double temperature;
    double batteryCharge;
    State state;
};

struct return_object {
        struct promise_type {
                message info;
                return_object get_return_object() {
                        return {
                                .handle_ = std::coroutine_handle<promise_type>::from_promise(*this)
                        };
                }
                std::suspend_never initial_suspend() { return {}; }
                std::suspend_always final_suspend() noexcept { return {}; }
                void unhandled_exception() {}
                std::suspend_always yield_value(message value) {
                        info = value;
                        return {};
                }
                std::suspend_always return_value(message value) {
                        info = value;
                        return {};
                }
        };
        std::coroutine_handle<promise_type> handle_;
};

return_object acclimate(message info){

    if(info.batteryCharge>20){
        while(info.temperature > 20 and info.batteryCharge>20){   
            info.temperature = info.temperature - 0.1; 
            info.batteryCharge = info.batteryCharge - 0.8;
            info.state = cooling;
            co_yield info;
        }
        while(info.temperature < 18 and info.batteryCharge>20){
            info.temperature = info.temperature + 0.1; 
            info.batteryCharge = info.batteryCharge - 0.8;
            info.state = heating;
            co_yield info;
        }
        if(info.temperature<=20 or info.temperature>=18){            
            info.state = charging;
            co_yield info;
        }
    }else{        
        info.state = charging;
        co_yield info;
    }
}

return_object charger(message info){    
    while(info.batteryCharge<94.9){
        info.batteryCharge = info.batteryCharge + 0.1;
        info.temperature = info.temperature + 0.01;
    }
    co_return info;
}

void init_values(message *p_info){
    p_info->temperature = rand() % 55;
    p_info->batteryCharge = rand() % 100;
    if(p_info->temperature<18 and p_info->batteryCharge>20){
        p_info->state = heating;
        std::cout << "The actual temperature is " + std::to_string(p_info->temperature) + ". Heating at " + std::to_string(p_info->batteryCharge) + "% of battery." << std::endl;             
    }else if(p_info->temperature>20 and p_info->batteryCharge>20){
        p_info->state = cooling;   
        std::cout << "The actual temperature is " + std::to_string(p_info->temperature) + ". Cooling at " + std::to_string(p_info->batteryCharge) + "% of battery." << std::endl;                 
    }else{
        if(p_info->batteryCharge>20){
            p_info->state = finish;
        }else{
            p_info->state = charging;
        }
    }
}

int main(){   

    message info;
    info.state = start;
    message* p_info = nullptr;
    p_info = &info; 

    std::coroutine_handle<return_object::promise_type> handle;                 
    return_object::promise_type* promise;
    std::coroutine_handle<return_object::promise_type> handle2;                
    return_object::promise_type* promise2; 

    while(info.state != 5){
        switch (info.state) {
        case start:       
            std::cout << "Start a new sequence" << std::endl;
            init_values(p_info);
            handle = acclimate(info).handle_;
            promise = &handle.promise();
            break;
        case cooling:  
            handle();
            info = promise->info;
            break;
        case heating:    
            handle();
            info = promise->info;
            break;
        case finish:         
            std::cout << "The actual temperature is " + std::to_string(info.temperature) + ". " + enum_str[info.state] + " at " + std::to_string(info.batteryCharge) + "% of battery." << std::endl;           
            info.state = start; 
            break;
        case charging:
            std::cout << "The actual temperature is " + std::to_string(info.temperature) + ". Start " + enum_str[info.state] + " at " + std::to_string(info.batteryCharge) + "% of battery." << std::endl; 
            //info = promise->info;
            handle2 = charger(info).handle_;                  
            promise2 = &handle2.promise();            
            info = promise2->info;
            std::cout << "The actual temperature is " + std::to_string(info.temperature) + ". Finish " + enum_str[charging] + " at " + std::to_string(info.batteryCharge) + "% of battery." << std::endl;             
            if(info.temperature<18){ 
                handle = acclimate(info).handle_;  
                promise = &handle.promise();               
                info = promise->info;
                info.state = heating;
                std::cout << "The actual temperature is " + std::to_string(info.temperature) + ". " + enum_str[info.state] + " at " + std::to_string(info.batteryCharge) + "% of battery." << std::endl; 
            }else if(info.temperature>20){  
                handle = acclimate(info).handle_;                
                promise = &handle.promise();              
                info.state = cooling;  
                std::cout << "The actual temperature is " + std::to_string(info.temperature) + ". " + enum_str[info.state] + " at " + std::to_string(info.batteryCharge) + "% of battery." << std::endl; 
            }else{
                info.state = finish;
            }
            break;
        }
    }
}
