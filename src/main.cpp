#include "thread_safe_queue.h"
#include "pipeline/pipeline_element.h"
#include "pipeline/pipeline.h"
#include "pipeline/udp_listener.h"
#include "pipeline/spx_processor_element.h"

using namespace std;

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
    cerr << "Pipelline Chain Error: " << e.what() << endl;
  }
}

int main()
{
  Test3();

  return 0;
}