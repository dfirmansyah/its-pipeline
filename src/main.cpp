#include <chrono>
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



const std::chrono::milliseconds SLOT_DURATION_MS = std::chrono::milliseconds(200);

class EchoElement : public ItElement
{
protected:
  std::unique_ptr<ItInputPort<RadarVideoSweep>> inputPort = nullptr;

  virtual void handleStateChanged() override
  {
    ItObjState _portNewState = getState() == IT_STATE_STARTED ? IT_STATE_STARTED : IT_STATE_STOPED;

    if (inputPort != nullptr)
    {
      inputPort->setState(_portNewState);
    }
  }
  
  void handleDataReceived(std::unique_ptr<RadarVideoSweep> data)
  {
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count();
    cout << "[" << millis << "] " << "[" << data->sequence << "] az: " << data->azimuth << endl;
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
    ipl_connect(udpListener.get(), spxProcessorElement.get());
    ipl_connect(spxProcessorElement.get(), loggerEl.get());
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

    ipl_linkPort(el_udpSource.getOutputPort(), el_spxDecoder.getInputPort());
    ipl_linkPort(el_spxDecoder.getOutputPort(), el_echo.getInputPort());

    el_echo.start();
    el_spxDecoder.start();
    el_udpSource.start();
    
    std::chrono::milliseconds TTP = std::chrono::milliseconds(1000);
    
    auto last_trigr = std::chrono::steady_clock::now();
    auto next_slot_time = std::chrono::steady_clock::now();
    
    bool spdx_state = true;
    while(true){
      next_slot_time += SLOT_DURATION_MS;
      std::this_thread::sleep_until(next_slot_time);
      
      // Test for pipeline element state change effect
      auto _now = std::chrono::steady_clock::now();
      auto _eti_mod = (_now - last_trigr) / TTP;

      if (_eti_mod > 3)
      {
        if (spdx_state) {
          el_spxDecoder.stop();
        }
        else {
          el_spxDecoder.start();
        }
        spdx_state = !spdx_state;
        last_trigr = _now;
      }
      // End test for pipeline element state change effect
    }
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