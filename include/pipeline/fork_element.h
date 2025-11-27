#ifndef FORK_ELEMENT_H
#define FORK_ELEMENT_H

#include <string>
#include <unordered_map>
#include "pipeline_element.h"

using namespace std;

/**
 * @brief A pipeline element that forks the input data to multiple output ports.
 */
template <typename T>
class ForkElement : public PipelineElement, public IInputPort<T>, public IOutputPort<T>
{
private:
  unordered_map<string, IInputPort<T> *> output_ports;

protected:
  virtual const string get_name() const override { return "Fork"; }

public:
  void receive(T value) override
  {
    for (const auto &[key, port] : output_ports)
    {
      port->receive(value);
    }
  }

  void signal_stop() override
  {
  }

  void connect_to_output(IInputPort<T> *port) override
  {
    // output_ports.[port->getName()] = port;
  }

  void stop() override
  {
    PipelineElement::stop();
  }
}

#endif // FORK_ELEMENT_H