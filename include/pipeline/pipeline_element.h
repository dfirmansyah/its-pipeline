#ifndef PIPELINE_ELEMENT_H
#define PIPELINE_ELEMENT_H

#include <iostream>
#include <thread>

#include "it_port.h"
#include "thread_safe_queue.h"

using namespace std;

/**
 * @class PipelineElement
 * @brief Base class for pipeline element processing node
 */
class PipelineElement
{
protected:
  unique_ptr<thread> worker_thread;
  bool running = false;

  // Function run by the worker thread
  virtual void process() = 0;

public:
  PipelineElement() = default;
  virtual ~PipelineElement() = default;

  void start()
  {
    if (!running)
    {
      running = true;
      worker_thread.reset(new thread(&PipelineElement::process, this));
      cout << "[" << get_name() << "] Starting thread ID: " << worker_thread->get_id() << endl;
    }
  }

  virtual void stop()
  {
    if (running)
    {
      running = false;
    }
    if (worker_thread && worker_thread->joinable())
    {
      worker_thread->join();
      cout << "[" << get_name() << "] Thread stopped." << endl;
    }
  }

  virtual const string get_name() const = 0;
};

/**
 * @class BufferElement
 * @brief Bridges two direct-call elements with an asynchronous queue.
 */
template <typename T>
class BufferElement : public PipelineElement, public IInputPort<T>, public IOutputPort<T>
{
private:
  IInputPort<T> *downstream_port = nullptr;
  ThreadSafeQueue<T> internal_queue; // Internal queue buffer

protected:
  virtual const string get_name() const override { return "Buffer"; }

  void process() override
  {
    if (!downstream_port)
    {
      cerr << "[" << get_name() << "] Warning: No downstream port connected." << endl;
    }

    T data;
    while (running)
    {
      // Pull from internal queue (input)
      if (internal_queue.wait_and_pop(data))
      {

        // this_thread::sleep_for(chrono::milliseconds(10));

        // Push directly to downstream (output)
        if (downstream_port)
        {
          downstream_port->receive(data);
        }
      }
      else
      {
        // Input channel stopped. Signal downstream and exit.
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
  void receive(T value) override
  {
    internal_queue.push(std::move(value));
  }

  void signal_stop() override
  {
    internal_queue.stop();
  }

  void connect_to_output(IInputPort<T> *port) override
  {
    downstream_port = port;
  }

  void stop() override
  {
    internal_queue.stop();
    PipelineElement::stop();
  }
};

#endif // PIPELINE_ELEMENT_H