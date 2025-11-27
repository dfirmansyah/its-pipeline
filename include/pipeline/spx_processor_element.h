#ifndef SPX_PROCESSOR_ELEMENT_H
#define SPX_PROCESSOR_ELEMENT_H

#include "RadarTypes.h"
#include "thread_safe_queue.h"
#include "pipeline_element.h"
#include "packet_processor/spx_packet_processor.h"

using namespace std;

class SpxProcessorElement : public PipelineElement,
                     public IInputPort<vector<uint8_t>>,
                     public IOutputPort<RadarVideoSweep>
{
private:
  IInputPort<RadarVideoSweep> *downstream_port = nullptr;
  ThreadSafeQueue<vector<uint8_t>> internal_queue;
  SpxProcessor spxProcessor;

protected:
  virtual const string get_name() const override { return "SPX Processor"; }

  void process() override
  {
    if (!downstream_port)
    {
      cerr << "[" << get_name() << "] Warning: No downstream port connected." << endl;
    }

    vector<uint8_t> input_data;
    while (running)
    {
      if (internal_queue.wait_and_pop(input_data))
      {
        
        auto result = spxProcessor.processPacket(input_data);

        if (result != NULL && downstream_port)
        {
          downstream_port->receive(*result.get());
        }
      }
      else
      {
        // Internal queue was stopped. Signal downstream and exit.
        if (downstream_port)
        {
          downstream_port->signal_stop();
        }
        break;
      }
    }
    running = false;
  }

public:
  void receive(vector<uint8_t> value) override
  {
    internal_queue.push(std::move(value));
  }

  void signal_stop() override
  {
    internal_queue.stop();
  }

  void connect_to_output(IInputPort<RadarVideoSweep>* port) override {
        downstream_port = port;
    }

  void stop() override
  {
    internal_queue.stop();
    PipelineElement::stop();
  }
};

#endif // SPX_PROCESSOR_ELEMENT_H