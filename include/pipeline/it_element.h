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
class ForkElement : public ItElement
{
private:
  void initializeOutputPorts()
  {
    if (!outputPorts.empty()) { outputPorts.clear(); }
    
    std::string outPortPrefix = create_slug(getName()).append("_op_");
    for (int i=1; i <= outPortNum; i++)
    {
      unique_ptr<ItOutputPort<RadarVideoSweep>> _outPort(
        new ItOutputPort<RadarVideoSweep>(outPortPrefix + std::to_string(i)));
      outputPorts.push_back(_outPort);
    }
  }
protected:
  int outPortNum = 1;
  std::vector<std::unique_ptr<ItOutputPort<T>>> outputPorts;

  std::unique_ptr<ItInputPort<T>> inputPort = nullptr;

  virtual void handleStateChanged() override
  {
    ItObjState _portNewState = getState() == IT_STATE_STARTED ? IT_STATE_STARTED : IT_STATE_STOPED;

    if (inputPort != nullptr)
    {
      inputPort->setState(_portNewState);
    }
    if (outputPorts != nullptr && !outputPorts.empty())
    {
      for (const auto& oPort : outputPorts) {
        oPort->setState(_portNewState);
      }
    }
  }
public:
  ForkElement(std::string elName, int outportNum) : ItElement(elName), outPortNum(outPortNum)
  {
    inputPort = nullptr;
    initializeOutputPorts();
  }

  ~ForkElement() {}

  ItInputPort<T>* getInputPort() const {
    return inputPort ? inputPort.get() : nullptr;
  }


};

#endif // IT_ELEMENT_H