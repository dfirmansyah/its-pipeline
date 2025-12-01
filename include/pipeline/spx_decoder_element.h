#ifndef SPX_DECODER_ELEMENT_H
#define SPX_DECODER_ELEMENT_H

#include <iostream>
#include <thread>
#include "element_ports.h"
#include "RadarTypes.h"
#include "packet_processor/spx_packet_processor.h"


using namespace std;

class SpxDecoderElement : StandardIOElement<RawVideoData, RadarVideoSweep>
{
private:
  SpxProcessor spxProcessor;
protected:
  RadarVideoSweep* processFunc(RawVideoData data)
  {
    return NULL;
  }

public:
  SpxDecoderElement(string name) : StandardIOElement(name)
  {
    unique_ptr<ItInputPort<RawVideoData>> _inPort(new ItInputPort<RawVideoData>("SpxDecoder_in"));
    setInputPort(_inPort.get());

    unique_ptr<ItOutputPort<RadarVideoSweep>> _outPort(new ItOutputPort<RadarVideoSweep>("SpxDecoder_out"));
    setOutputPort(_outPort.get());
  }

  ~SpxDecoderElement() {}

  
};

#endif // SPX_DECODER_ELEMENT_H