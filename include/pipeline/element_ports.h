#ifndef ELEMENTS_PORTS_H
#define ELEMENTS_PORTS_H

#include <string>
#include <functional>
#include "RadarTypes.h"

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
class ItPort
{
public:
  // Listener callback
  using PushListener = std::function<void(const T &)>;

protected:
  ItPortDirection direction;
  ItPortMode mode;
  std::string name;

  ItPort *downstreamPort = NULL;
  ItPort *upstreamPort = NULL;

  std::vector<PushListener> pushListeners;

  virtual bool pushFunc(T data) = 0;
  virtual bool getDataRangeFunc(ItBuffer<T> *buffer, int length) = 0;
  virtual bool getDataRangeFunc(T *data) = 0;

  // Notify all registered push listeners
  void notifyPushListeners(const T &data)
  {
    for (const auto &listener : pushListeners)
    {
      if (listener)
        listener(data);
    }
  }

public:
  ItPort(std::string portName, ItPortDirection portDirection, ItPortMode portMode)
      : name(portName), direction(portDirection), mode(portMode) {}
  ItPort(std::string portName, ItPortDirection portDirection)
      : name(portName), direction(portDirection), mode(IT_MODE_PUSH) {}
  virtual ~ItPort() {}

  std::string getName() const { return name; }
  ItPortDirection getDirection() const { return direction; }
  ItPortMode getMode() const { return mode; }

  // Register push listener
  void addListener(PushListener callback)
  {
    pushListeners.push_back(callback);
  }

  bool push(T data)
  {
    if (mode != IT_MODE_PUSH)
      return false;
    bool pushSuccess = pushFunc(data);
    if (pushSuccess)
      notifyPushListeners(data);

    return pushSuccess;
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
class ItInputPort : ItPort<T>
{
protected:
  ItPort<T> upstreamPort = NULL;
  virtual bool pushFunc(T data) = 0;

public:
  ItInputPort(std::string name, ItPortMode portMode) : ItPort<T>(name, IT_DIR_INPUT, portMode) {}
  ItInputPort(std::string name) : ItPort<T>(name, IT_DIR_INPUT) {}

  void setUpstreamPort(ItPort<T> *upstream_port) { upstreamPort = upstream_port; }
  ItPort<T> *getUpstreamPort() { return upstreamPort; }
};

template <typename T>
class ItOutputPort : ItPort<T>
{
protected:
  ItPort<T> *downstreamPort = NULL;
  virtual bool getDataRangeFunc(ItBuffer<T> *buffer, int length) = 0;
  virtual bool getDataRangeFunc(T *data) = 0;

public:
  ItOutputPort(std::string name, ItPortMode portMode) : ItPort<T>(name, IT_DIR_OUTPUT, portMode) {}
  ItOutputPort(std::string name) : ItPort<T>(name, IT_DIR_OUTPUT) {}

  void setDownstreamPort(ItPort<T> *downstream_port) { downstreamPort = downstream_port; }
  ItPort<T> *getDownstreamPort(ItPort<T> *downstream_port) { return downstreamPort; }
};

template <typename TI, typename TO>
class ItElement
{
protected:
  std::string name;
  // std::vector<ItPort<void*>> ports;
  ItInputPort<TI> *inputPort;
  ItOutputPort<TO> *outputPort;

  virtual TO* processFunc(const TI data);
  virtual void initialize();
  
  void handleDataReceived(const TI data)
  {
    auto result = processFunc(data);
    if (outputPort != NULL)
    {
      outputPort->push(*result.get());
    }
  }

  void setInputPort(ItInputPort<TI> *port)
  {
    this->inputPort = port;
    if (inputPort != NULL)
    {
      inputPort->addListener(handleDataReceived);
    }
  }

  void setOutputPort(ItOutputPort<TO> *port)
  {
    this->outputPort = port;
  }

public:
  ItElement(std::string elName) : name(elName)
  {
    initialize();
  }

  ~ItElement() {}
};





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

#endif // ELEMENTS_PORTS_H