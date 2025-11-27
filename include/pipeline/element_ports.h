#ifndef ELEMENTS_PORTS_H
#define ELEMENTS_PORTS_H


/**
 * @brief Interface for elements that can receive data (all consumers)
 */
template <typename T>
class IInputPort {
public:
    virtual ~IInputPort() = default;
    // Called by the upstream element's thread.
    virtual void receive(T value) = 0; 
    // Called by the upstream element's thread to signal EOF.
    virtual void signal_stop() = 0; 
};

/**
 * @brief Interface for elements that provide output (all producers)
 */
template <typename T>
class IOutputPort {
public:
    virtual ~IOutputPort() = default;
    // Connects the provider's output to the downstream input port (RENAMED)
    virtual void connect_to_output(IInputPort<T>* port) = 0;
};


#endif // ELEMENTS_PORTS_H