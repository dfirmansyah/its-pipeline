#include "common.h"
#include "thread_safe_queue.h"
#include "pipeline/pipeline_element.h"
#include "pipeline/pipeline.h"
#include "pipeline/udp_listener.h"
#include "pipeline/spx_processor_element.h"

#include "pipeline/it_port.h"
#include "pipeline/it_element.h"
#include "pipeline/udp_source.h"
#include "pipeline/spx_decoder_element.h"

using namespace std;

class EchoElement : public ItElement
{
protected:
  std::unique_ptr<ItInputPort<RadarVideoSweep>> inputPort = nullptr;

  void handleDataReceived(std::unique_ptr<RadarVideoSweep> data)
  {
    cout << "[" << data->sequence << "] az: " << data->azimuth << endl;
  }

public:
  EchoElement(std::string elName) : ItElement(elName)
  {
    unique_ptr<ItInputPort<RadarVideoSweep>> t_inputPort = make_unique<ItInputPort<RadarVideoSweep>>("Echo_in");
    inputPort = move(t_inputPort);

    inputPort->setDataReceiveHandler([this](std::unique_ptr<RadarVideoSweep> d){ handleDataReceived(move(d)); });
  }
  
  ItInputPort<RadarVideoSweep>* getInputPort() const {
    return inputPort ? inputPort.get() : nullptr;
  }

};

static void Test3()
{
  // const int port = 4378;
  unique_ptr<UdpListener> udpListener(new UdpListener(4378));
  unique_ptr<SpxProcessorElement> spxProcessorElement(new SpxProcessorElement());
  unique_ptr<Logger<RadarVideoSweep>> loggerEl(new Logger<RadarVideoSweep>());


  try
  {
    connect(udpListener.get(), spxProcessorElement.get());
    connect(spxProcessorElement.get(), loggerEl.get());
    udpListener->start();
    spxProcessorElement->start();
    loggerEl->start();

    while(true){}
    
    udpListener->stop();
    spxProcessorElement->stop();
    loggerEl->stop();
    
    cout << "--- Pipelline execution complete. ---" << endl;
  }
  catch (const exception &e)
  {
    cerr << "Pipeline Chain Error: " << e.what() << endl;
  }
}

static void Test4()
{
  try
  {
    UdpSource el_udpSource("UDP Source", 4378, UdpMode::NonBlocking);
    SpxDecoderElement el_spxDecoder("SPX Decoder");
    EchoElement el_echo("Echoing");

    linkPort(el_spxDecoder.getInputPort(), el_udpSource.getOutputPort());
    linkPort(el_echo.getInputPort(), el_spxDecoder.getOutputPort());

    el_echo.start();
    el_spxDecoder.start();
    el_udpSource.start();
    
    while(true){}
    
  }
  catch (const exception &e)
  {
    cerr << "Pipeline Chain Error: " << e.what() << endl;
  }
}

int main()
{
  // Test3();
  Test4();

  return 0;
}