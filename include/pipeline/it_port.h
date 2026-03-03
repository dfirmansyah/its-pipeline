#ifndef IT_PORT_H
#define IT_PORT_H

#include <string>
#include <functional>
#include "it_obj.h"
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
  // Listener callback
  // using PushListener = std::function<void(std::shared_ptr<T>)>;
  using PushFunc = std::function<void(std::unique_ptr<T>)>;

protected:
  ItPortDirection direction;
  ItPortMode mode;
  std::string name;

  // std::vector<PushListener> pushListeners;
  PushFunc pushFunc;
  
  virtual void handleStateChanged() override {}

  // virtual bool pushFunc(T data) = 0;
  // virtual bool getDataRangeFunc(ItBuffer<T> *buffer, int length) = 0;
  // virtual bool getDataRangeFunc(T *data) = 0;
  virtual bool getDataRangeFunc(ItBuffer<T> *buffer, int length) { return false; };
  virtual bool getDataRangeFunc(T *data) { return false; };

  // Notify all registered push listeners
  // void notifyPushListeners(std::unique_ptr<T> data)
  // {
  //   for (const auto &listener : pushListeners)
  //   {
  //     if (listener) {
  //       std::shared_ptr<T> data_ptr(move(data));
  //       listener(data_ptr);
  //     }
  //   }
  // }

public:
  ItPort(std::string portName, ItPortDirection portDirection, ItPortMode portMode)
      : ItObject(portName), direction(portDirection), mode(portMode) {}
  ItPort(std::string portName, ItPortDirection portDirection)
      : ItObject(portName), direction(portDirection), mode(IT_MODE_PUSH) {}
  ~ItPort() = default;;

  std::string getName() const { return name; }
  ItPortDirection getDirection() const { return direction; }
  ItPortMode getMode() const { return mode; }

  // Register push listener
  // void addListener(PushListener callback)
  // {
  //   pushListeners.push_back(callback);
  // }

  void push(std::unique_ptr<T> data)
  {
  //   if (mode != IT_MODE_PUSH)
  //     return;
    // bool pushSuccess = pushFunc(data);
    // if (pushSuccess)
      // notifyPushListeners(move(data));
      
      // return pushSuccess;
    
    if (getState() == IT_STATE_STARTED)
    {
      pushFunc(move(data));
    }
  }

  bool getDataRange(ItBuffer<T> *buffer, int length)
  {
    if (mode != IT_MODE_PULL)
      return false;
    return getDataRangeFunc(buffer, length);
  }

  bool getDataRange(T *data)
  {
    if (mode != IT_MODE_PULL)
      return false;
    return getDataRangeFunc(data);
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

  void initializePort()
  {
    // if (this->mode == IT_MODE_PUSH) this->pushFunc = [this](std::unique_ptr<T> d) { dataReceiveHandler(move(d)); };
    if (this->mode == IT_MODE_PUSH) this->pushFunc = [this](std::unique_ptr<T> d) { dataReceiveHandler(move(d)); };
    else this->pushFunc = [this](std::unique_ptr<T> d) { };
  }

public:
  ItInputPort(std::string name, ItPortMode portMode) : ItPort<T>(name, IT_DIR_INPUT, portMode) { }
  ItInputPort(std::string name) : ItPort<T>(name, IT_DIR_INPUT) { }

  void setUpstreamPort(ItPort<T> *upstream_port) { upstreamPort = upstream_port; }
  ItPort<T> *getUpstreamPort() { return upstreamPort; }

  void setDataReceiveHandler(DataReceiveHandler handler)
  {
    dataReceiveHandler = handler;
    initializePort();
  }
};


template <typename T>
class ItOutputPort : public ItPort<T>
{
private:
  ThreadSafeQueue<T> internal_queue;

  void dropPushHandler(std::unique_ptr<T> data) {}

  void chainDownstreamPushHandler(std::unique_ptr<T> data)
  {
    downstreamPort->push(std::move(data));
  }

  void queuePushHandler(std::unique_ptr<T> data)
  {
    internal_queue.push(std::move(*data));
  }

  void initializePushFunc()
  {
    if (downstreamPort == nullptr)
    {
      this->pushFunc = [this](std::unique_ptr<T> d){ dropPushHandler(move(d)); };
      return;
    }

    switch(downstreamPort->getMode())
    {
      case IT_MODE_PUSH: this->pushFunc = [this](std::unique_ptr<T> d){ chainDownstreamPushHandler(move(d)); };  break;
      case IT_MODE_PULL: this->pushFunc = [this](std::unique_ptr<T> d){ queuePushHandler(move(d)); };  break;
      default: this->pushFunc = [this](std::unique_ptr<T> d){ dropPushHandler(move(d)); };  break;
    }
  }

protected:
  ItPort<T> *downstreamPort = nullptr;
  // virtual bool getDataRangeFunc(ItBuffer<T> *buffer, int length) = 0;
  // virtual bool getDataRangeFunc(T *data) = 0;
  virtual bool getDataRangeFunc(ItBuffer<T> *buffer, int length) override { return false; };
  virtual bool getDataRangeFunc(T *data) override {
    if (downstreamPort == nullptr || downstreamPort->getMode() != IT_MODE_PULL) return false;

    if (data != nullptr)
    {
      internal_queue.wait_and_pop(*data);
      return true;
    }
    return false;
  };

public:
  ItOutputPort(std::string name, ItPortMode portMode) : ItPort<T>(name, IT_DIR_OUTPUT, portMode)
  {
    initializePushFunc();
  }

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