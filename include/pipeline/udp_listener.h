#ifndef UDP_LISTENER_H
#define UDP_LISTENER_H

#include "RadarTypes.h"
#include "pipeline_element.h"

using namespace std;

class UdpListener : public PipelineElement, public IOutputPort<RawVideoData>
{
private:
  IInputPort<RawVideoData> *downstream_port = nullptr;
  int port;

protected:
  virtual const string get_name() const override { return "UDP Listener"; }
  virtual void process() override;

public:
  UdpListener(int port);
  ~UdpListener();

  void connect_to_output(IInputPort<RawVideoData>* port) override {
      downstream_port = port;
  }
};

#endif // UDP_LISTENER_H