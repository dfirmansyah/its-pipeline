#ifndef IT_ELEMENT_H
#define IT_ELEMENT_H

#include <atomic>
#include "string_util.h"
#include "it_obj.h"
#include "it_port.h"


class ItElement : public ItObject
{
protected:
  virtual void handleStateChanged() override {}

public:
  ItElement(std::string elName) : ItObject(elName) {}

  ~ItElement() = default;

  virtual bool start() { setState(IT_STATE_STARTED); return true; }

  virtual bool pause() { setState(IT_STATE_PAUSED); return true; }

  virtual bool stop() { setState(IT_STATE_STOPED); return true; }
};


template <typename TI, typename TO>
class StandardIOElement : public ItElement
{
protected:
  std::unique_ptr<ItInputPort<TI>> inputPort = nullptr;
  std::unique_ptr<ItOutputPort<TO>> outputPort = nullptr;

  virtual std::unique_ptr<TO> processFunc(const std::unique_ptr<TI> &data) = 0;

  virtual void handleStateChanged() override
  {
    ItObjState _portNewState = getState() == IT_STATE_STARTED ? IT_STATE_STARTED : IT_STATE_STOPED;

    if (inputPort != nullptr)
    {
      inputPort->setState(_portNewState);
    }
    if (outputPort != nullptr)
    {
      outputPort->setState(_portNewState);
    }
  }
  
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


template <typename T>
class StandardInputElement : public ItElement
{
protected:
  std::unique_ptr<ItInputPort<T>> inputPort = nullptr;

  virtual void processFunc(const std::unique_ptr<T> &data) = 0;

  virtual void handleStateChanged() override
  {
    ItObjState _portNewState = getState() == IT_STATE_STARTED ? IT_STATE_STARTED : IT_STATE_STOPED;

    if (inputPort != nullptr)
    {
      inputPort->setState(_portNewState);
    }
  }
  
  void handleDataReceived(std::unique_ptr<T> data)
  {
    processFunc(data);
  }

  void setInputPort(std::unique_ptr<ItInputPort<T>> port)
  {
    this->inputPort = move(port);
    if (inputPort != nullptr)
    {
      // inputPort->addListener([this](std::shared_ptr<TI> d){ handleDataReceived(d); });
      inputPort->setDataReceiveHandler([this](std::unique_ptr<T> d){ handleDataReceived(move(d)); });
    }
  }

public:
  StandardInputElement(std::string elName) : ItElement(elName)
  {
    inputPort = nullptr;
  }

  ~StandardInputElement() {}

  ItInputPort<T>* getInputPort() const {
    return inputPort ? inputPort.get() : nullptr;
  }
};


template <typename T>
class ForkElement : public StandardInputElement<T>
{
protected:
  int outPortNum;
  std::vector<std::unique_ptr<ItOutputPort<T>>> outputPorts;

  void initializeOutputPorts()
  {
    if (!outputPorts.empty()) { outputPorts.clear(); }
    
    std::string outPortPrefix = create_slug(ItElement::getName()).append("_op_");
    for (int i=0; i < outPortNum; i++)
    {
      std::unique_ptr<ItOutputPort<T>> _outPort(new ItOutputPort<T>(outPortPrefix + std::to_string(i)));
      outputPorts.push_back(move(_outPort));
    }
  }

  void handleStateChanged() override
  {
    ItObjState _portNewState = ItElement::getState() == IT_STATE_STARTED ? IT_STATE_STARTED : IT_STATE_STOPED;
    StandardInputElement<T>::handleStateChanged();

    if (!outputPorts.empty())
    {
      for (const auto& oPort : outputPorts)
      {
        oPort->setState(_portNewState);
      }
    }
  }

  void processFunc(const unique_ptr<T> &data) override
  {
    for (const auto& outPort : outputPorts)
    {
      unique_ptr<T> cloned_data = clone(data);
      outPort->push(move(cloned_data));

    }
  }

public:
  ForkElement(std::string elName, int pOutportNum) : StandardInputElement<T>(elName), outPortNum(pOutportNum)
  {
    std::string inPortPrefix = create_slug(elName).append("_ip");
    unique_ptr<ItInputPort<T>> _inPort(new ItInputPort<T>(inPortPrefix));
    this->setInputPort(move(_inPort));

    initializeOutputPorts();
  }

  ItOutputPort<T>* getOutputPort(int port_index)
  {
    if (port_index >= outputPorts.size()) return nullptr;

    return outputPorts.at(port_index).get();
  }

  ~ForkElement() {}

};

#endif // IT_ELEMENT_H