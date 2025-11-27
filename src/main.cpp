#include "thread_safe_queue.h"
#include "pipeline/pipeline_element.h"
#include "pipeline/pipeline.h"
#include "pipeline/udp_listener.h"
#include "pipeline/spx_processor_element.h"

using namespace std;

static void Test1()
{
  cout << "--- Pipeline Demo with Explicit Buffering and Direct Ports ---" << endl;

  const int NUM_ITEMS = 5;

  // --- DEMO 1: Synchronous/Direct Chain (Source -> Processor -> Sink) ---
  cout << "\n--- Starting Direct Chain (Source thread calls Processor, Processor thread calls Sink) ---" << endl;

  unique_ptr<Source> source_direct(new Source(NUM_ITEMS));
  unique_ptr<Processor> processor_direct(new Processor());
  unique_ptr<Sink> sink_direct(new Sink());

  try
  {
    // Uniform connection syntax
    connect(source_direct.get(), processor_direct.get());
    connect(processor_direct.get(), sink_direct.get());

    // Start all worker threads
    sink_direct->start();
    processor_direct->start();
    source_direct->start(); // The flow begins here

    // Wait for the final sink to finish
    sink_direct->stop();
    processor_direct->stop();
    source_direct->stop();

    cout << "--- Direct Chain execution complete. ---" << endl;
  }
  catch (const exception &e)
  {
    cerr << "Direct Chain Error: " << e.what() << endl;
  }
}

static void Test2()
{
  // --- DEMO 2: Asynchronous/Buffered Chain (Source -> Buffer -> Processor -> Sink) ---
  cout << "\n--- Starting Buffered Chain (Source -> Buffer -> Processor -> Sink) ---" << endl;

  const int NUM_ITEMS = 5;
  unique_ptr<Source> source_buffered(new Source(NUM_ITEMS));
  // Explicit buffer element added here
  unique_ptr<BufferElement<int>> buffer(new BufferElement<int>());
  unique_ptr<Processor> processor_buffered(new Processor());
  unique_ptr<Sink> sink_buffered(new Sink());

  try
  {
    // Source -> Buffer (Direct Call)
    connect(source_buffered.get(), buffer.get());
    // Buffer -> Processor (Direct Call)
    connect(buffer.get(), processor_buffered.get());
    // Processor -> Sink (Direct Call)
    connect(processor_buffered.get(), sink_buffered.get());

    // Start all worker threads
    sink_buffered->start();
    processor_buffered->start();
    buffer->start();
    source_buffered->start(); // The flow begins here

    // Wait for the final sink to finish
    sink_buffered->stop();
    processor_buffered->stop();
    buffer->stop();
    source_buffered->stop();

    cout << "--- Buffered Chain execution complete. ---" << endl;
  }
  catch (const exception &e)
  {
    cerr << "Buffered Chain Error: " << e.what() << endl;
  }
}

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
  // Test1();
  // Test2();
  Test3();

  return 0;
}