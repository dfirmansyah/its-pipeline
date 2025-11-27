#ifndef UDP_LISTENER_H
#define UDP_LISTENER_H

#include <vector>
#include "pipeline_element.h"

using namespace std;

class UdpListener : public PipelineElement, public IOutputPort<vector<uint8_t>>
{
private:
  IInputPort<vector<uint8_t>> *downstream_port = nullptr;
  int port;

protected:
  virtual const string get_name() const override { return "UDP Listener"; }
  virtual void process() override;

public:
  UdpListener(int port);
  ~UdpListener();

  void connect_to_output(IInputPort<vector<uint8_t>>* port) override {
      downstream_port = port;
  }
};

#endif // UDP_LISTENER_H