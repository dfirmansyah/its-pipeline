#ifndef IT_ELEMENT_H
#define IT_ELEMENT_H

#include <atomic>
#include "it_obj.h"
#include "it_port.h"


class ItElement
{
protected:
  std::string name;
  std::atomic<ItObjState> state;
public:
  ItElement(std::string elName) : name(elName)
  {
    state = IT_STATE_STOPED;
  }

  ~ItElement() = default;

  std::string getName() const { return name; }
  
  ItObjState getState() { return state; }

  virtual bool start() { state = IT_STATE_STARTED; return true; }

  virtual bool pause() { state = IT_STATE_PAUSED; return true; }

  virtual bool stop() { state = IT_STATE_STOPED; return true; }
};


template <typename TI, typename TO>
class StandardIOElement : public ItElement
{
protected:
  std::unique_ptr<ItInputPort<TI>> inputPort = nullptr;
  std::unique_ptr<ItOutputPort<TO>> outputPort = nullptr;

  virtual std::unique_ptr<TO> processFunc(const std::unique_ptr<TI> &data) = 0;
  
  void handleDataReceived(std::unique_ptr<TI> data)
  {
    auto result = processFunc(data);
    if (result && outputPort != nullptr)
    {
      outputPort->push(move(result));
    }
  }

  void setInputPort(std::unique_ptr<ItInputPort<TI>> port)
  {
    this->inputPort = move(port);
    if (inputPort != nullptr)
    {
      // inputPort->addListener([this](std::shared_ptr<TI> d){ handleDataReceived(d); });
      inputPort->setDataReceiveHandler([this](std::unique_ptr<TI> d){ handleDataReceived(move(d)); });
    }
  }

  void setOutputPort(std::unique_ptr<ItOutputPort<TO>> port)
  {
    this->outputPort = move(port);
  }

public:
  StandardIOElement(std::string elName) : ItElement(elName)
  {
    inputPort = nullptr;
    outputPort = nullptr;
  }

  ~StandardIOElement() {}

  ItInputPort<TI>* getInputPort() const {
    return inputPort ? inputPort.get() : nullptr;
  }

  ItOutputPort<TO>* getOutputPort() const {
    return outputPort ? outputPort.get() : nullptr;
  }
};

#endif // IT_ELEMENT_H