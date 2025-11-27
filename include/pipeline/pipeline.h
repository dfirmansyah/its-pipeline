#ifndef PIPELINE_H
#define PIPELINE_H

#include "pipeline_element.h"

// --- Core Pipeline Elements (Direct Call) ---

// Source: Generates data and sends it directly to a downstream element's input port.
class Source : public PipelineElement, public IOutputPort<int> {
private:
    IInputPort<int>* downstream_port = nullptr; 
    int max_items;

protected:
    virtual const string get_name() const override { return "Source"; }

    void process() override {
        if (!downstream_port) {
            cerr << "[" << get_name() << "] Error: Not connected to an input port." << endl;
            return;
        }
        
        for (int i = 1; i <= max_items && running; ++i) {
            this_thread::sleep_for(chrono::milliseconds(200)); 
            cout << "[" << get_name() << "] Generating item: " << i << endl;
            // Direct call to the downstream element's input interface
            downstream_port->receive(i);
        }
        downstream_port->signal_stop(); 
        running = false; 
    }

public:
    Source(int count) : max_items(count) {}
    
    void connect_to_output(IInputPort<int>* port) override {
        downstream_port = port;
    }
};

// Processor: Receives data, processes it, and sends it directly to a downstream element.
class Processor : public PipelineElement, public IInputPort<int>, public IOutputPort<int> {
private:
    IInputPort<int>* downstream_port = nullptr;
    ThreadSafeQueue<int> internal_queue; // Used to decouple receiving thread from processing thread

protected:
    virtual const string get_name() const override { return "Processor"; }

    void process() override {
        if (!downstream_port) {
            cerr << "[" << get_name() << "] Warning: No downstream port connected." << endl;
        }

        int input_data;
        while (running) {
            // Pull from internal queue (data was pushed here by the upstream element's thread)
            if (internal_queue.wait_and_pop(input_data)) {
                
                this_thread::sleep_for(chrono::milliseconds(100)); 
                int processed_data = input_data * 2; 

                cout << "[" << get_name() << "] Transforming " << input_data 
                     << " -> " << processed_data << endl;

                if (downstream_port) {
                    downstream_port->receive(processed_data);
                }

            } else {
                // Internal queue was stopped. Signal downstream and exit.
                if (downstream_port) {
                    downstream_port->signal_stop();
                }
                break;
            }
        }
        running = false;
    }

public:
    // --- IInputPort Implementation (Input) ---
    void receive(int value) override {
        internal_queue.push(std::move(value));
    }
    
    void signal_stop() override {
        internal_queue.stop();
    }

    // --- IOutputPort Implementation (Output) ---
    void connect_to_output(IInputPort<int>* port) override {
        downstream_port = port;
    }
    
    void stop() override {
        internal_queue.stop(); 
        PipelineElement::stop(); 
    }
};

// Sink: Receives data and consumes it.
class Sink : public PipelineElement, public IInputPort<int> {
private:
    ThreadSafeQueue<int> internal_queue; // Used to decouple receiving thread from consuming thread

protected:
    virtual const string get_name() const override { return "Sink"; }

    void process() override {
        int input_data;
        while (running) {
            // Pull from internal queue
            if (internal_queue.wait_and_pop(input_data)) {
                
                this_thread::sleep_for(chrono::milliseconds(50)); 
                cout << "[" << get_name() << "] Final Result: " << input_data << " (DONE)" << endl;
                
            } else {
                break;
            }
        }
        running = false;
        cout << "[" << get_name() << "] Finished consuming all items." << endl;
    }

public:
    // --- IInputPort Implementation (Input) ---
    void receive(int value) override {
        internal_queue.push(std::move(value));
    }
    
    void signal_stop() override {
        internal_queue.stop();
    }
    
    void stop() override {
        internal_queue.stop(); 
        PipelineElement::stop(); 
    }
};

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