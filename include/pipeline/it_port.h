#ifndef IT_PORT_H
#define IT_PORT_H

#include <string>
#include <functional>
#include "it_obj.h"
#include "it_util.h"
#include "RadarTypes.h"
#include "thread_safe_queue.h"

enum ItPortDirection
{
  IT_DIR_NONE,
  IT_DIR_INPUT,
  IT_DIR_OUTPUT
};

enum ItPortMode
{
  IT_MODE_NONE,
  IT_MODE_PUSH,
  IT_MODE_PULL
};

/**
 * @brief Interface for elements that output from source element to destination element.
 *        ItPort have direction that determine its behaviour.
 *        Output port (IT_DIR_OUTPUT) produce data, and input port (IT_PORT_INPUT) push/consume data.
 */
template <typename T>
class ItPort : public ItObject
{
public:
  using PushFunc = std::function<void(std::unique_ptr<T>)>;

protected:
  ItPortDirection direction;

  PushFunc pushFunc;
  
  virtual void handleStateChanged() override {}

public:
  ItPort(std::string portName, ItPortDirection portDirection)
      : ItObject(portName), direction(portDirection) {}
  ~ItPort() = default;;

  ItPortDirection getDirection() const { return direction; }

  void push(std::unique_ptr<T> data)
  {
    if (getState() == IT_STATE_STARTED)
    {
      pushFunc(move(data));
    }
  }

};

template <typename T>
class ItInputPort : public ItPort<T>
{
public:
  using DataReceiveHandler = std::function<void(std::unique_ptr<T>)>;

protected:
  ItPort<T> *upstreamPort = nullptr;
  DataReceiveHandler dataReceiveHandler;

public:
  ItInputPort(std::string name) : ItPort<T>(name, IT_DIR_INPUT) { }

  ItPort<T> *getUpstreamPort() { return upstreamPort; }
  void setUpstreamPort(ItPort<T> *upstream_port) { upstreamPort = upstream_port; }

  void setDataReceiveHandler(DataReceiveHandler handler)
  {
    dataReceiveHandler = handler;
    this->pushFunc = [this](std::unique_ptr<T> d) { dataReceiveHandler(move(d));};
  }
};


template <typename T>
class ItOutputPort : public ItPort<T>
{
private:
  void dropPushHandler(std::unique_ptr<T> data) {}

  void chainDownstreamPushHandler(std::unique_ptr<T> data)
  {
    downstreamPort->push(std::move(data));
  }

  void initializePushFunc()
  {
    if (downstreamPort == nullptr)
    {
      this->pushFunc = [this](std::unique_ptr<T> d){ dropPushHandler(move(d)); };
      return;
    }

    this->pushFunc = [this](std::unique_ptr<T> d){ chainDownstreamPushHandler(move(d)); };
  }

protected:
  ItPort<T> *downstreamPort = nullptr;

public:
  ItOutputPort(std::string name) : ItPort<T>(name, IT_DIR_OUTPUT)
  {
    initializePushFunc();
  }

  void setDownstreamPort(ItPort<T> *downstream_port)
  {
    downstreamPort = downstream_port;
    initializePushFunc();
  }

  ItPort<T> *getDownstreamPort() { return downstreamPort; }
};


//======================================================================================



/**
 * @brief Interface for elements that can push data (all consumers)
 */
template <typename T>
class IInputPort
{
public:
  virtual ~IInputPort() = default;
  // Push data to port.
  virtual void receive(T value) = 0;
  // Called by the upstream element's thread to signal EOF.
  virtual void signal_stop() = 0;
};

/**
 * @brief Interface for elements that provide output (all producers)
 */
template <typename T>
class IOutputPort
{
public:
  virtual ~IOutputPort() = default;
  // Connects the provider's output to the downstream input port
  virtual void connect_to_output(IInputPort<T> *port) = 0;
};

#endif // IT_PORT_H