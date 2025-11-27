#ifndef PIPELINE_H
#define PIPELINE_H

#include "pipeline_element.h"

template <typename T>
class Logger : public PipelineElement, public IInputPort<T> {
private:
    // ThreadSafeQueue<T> internal_queue;

protected:
    virtual const string get_name() const override { return "Logger"; }

    void process() override {
        // T input_data;
        // while (running) {
        //     // Pull from internal queue
        //     if (internal_queue.wait_and_pop(input_data)) {
                
        //         // this_thread::sleep_for(chrono::milliseconds(50)); 
        //         cout << "[" << get_name() << "] Data received " << endl;
                
        //     } else {
        //         break;
        //     }
        // }
        // running = false;
        // cout << "[" << get_name() << "] Finished consuming all items." << endl;

        while(running){}
    }

public:
    void receive(T value) override {
        // internal_queue.push(std::move(value));
        printf("[%s] Data received...\n", get_name());

    }
    
    void signal_stop() override {
        // internal_queue.stop();
    }
    
    void stop() override {
        // internal_queue.stop(); 
        PipelineElement::stop(); 
    }
};

// --- Unified Connection Helper ---

template <typename T>
void connect(IOutputPort<T>* upstream, IInputPort<T>* downstream) {
    if (!upstream || !downstream) {
        throw runtime_error("Cannot connect null elements.");
    }
    // Simple, uniform connection: The upstream output port is told who its downstream input port is.
    upstream->connect_to_output(downstream);
};


#endif // PIPELINE_H