#ifndef ELEMENTS_PORTS_H
#define ELEMENTS_PORTS_H

#include <string>
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
 *        Output port (IT_DIR_OUTPUT) produce data, and input port (IT_PORT_INPUT) receive/consume data.
 */
template <typename T>
class ItPort
{
protected:
  ItPortDirection direction;
  ItPortMode mode;
  std::string name;

  ItPort* downstreamPort = NULL;
  ItPort* upstreamPort = NULL;

  virtual bool receiveFunc(T data) = 0;
  virtual bool getDataRangeFunc(ItBuffer<T>* buffer, int length) = 0;
  virtual bool getDataRangeFunc(T* data) = 0;

public:
  ItPort(std::string portName, ItPortDirection portDirection, ItPortMode portMode)
    : name(portName), direction(portDirection), mode(portMode) {}
  ItPort(std::string portName, ItPortDirection portDirection)
    : name(portName), direction(portDirection), mode(IT_MODE_PUSH) {}
  virtual ~ItPort() {}

  std::string getName() const { return name; }
  ItPortDirection getDirection() const { return direction; }
  ItPortMode getMode() const { return mode; }

  bool receive(T data)
  {
    if (mode != IT_MODE_PUSH) return false;
    return receiveFunc(data);
  }

  bool getDataRange(ItBuffer<T>* buffer, int length)
  {
    if (mode != IT_MODE_PULL) return false;
    return getDataRangeFunc(buffer, length);
  }

  bool getDataRange(T* data)
  {
    if (mode != IT_MODE_PULL) return false;
    return getDataRangeFunc(data);
  }
};

template <typename T>
class ItInputPort : ItPort<T>
{
protected:
  virtual bool receiveFunc(T data) = 0;
public:
  ItInputPort(std::string name, ItPortMode portMode) : ItPort(name, IT_DIR_INPUT, portMode) {}
  ItInputPort(std::string name) : ItPort(name, IT_DIR_INPUT) {}
};

template <typename T>
class ItOutputPort : ItPort<T>
{
protected:
  virtual bool getDataRangeFunc(ItBuffer<T>* buffer, int length) = 0;
  virtual bool getDataRangeFunc(T* data) = 0;
public:
  ItInputPort(std::string name, ItPortMode portMode) : ItPort(name, IT_DIR_OUTPUT, portMode) {}
  ItInputPort(std::string name) : ItPort(name, IT_DIR_OUTPUT) {}
};

/**
 * @brief Interface for elements that can receive data (all consumers)
 */
template <typename T>
class IInputPort
{
public:
  virtual ~IInputPort() = default;
  // Called by the upstream element's thread.
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
  // Connects the provider's output to the downstream input port (RENAMED)
  virtual void connect_to_output(IInputPort<T> *port) = 0;
};

#endif // ELEMENTS_PORTS_H