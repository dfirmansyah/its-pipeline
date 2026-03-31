#ifndef SPX_DECODER_ELEMENT_H
#define SPX_DECODER_ELEMENT_H

#include <iostream>
#include <thread>
#include "it_port.h"
#include "it_element.h"
#include "RadarTypes.h"
#include "packet_processor/spx_packet_processor.h"


using namespace std;

class SpxDecoderElement : 
  public StandardIOElement<RawVideoData, RadarVideoSweep>
{
private:
  SpxProcessor spxProcessor;
protected:
  unique_ptr<RadarVideoSweep> transformFunc(const unique_ptr<RawVideoData> &data) override
  {
    RawVideoData *rawData = data.get();
    auto packet = spxProcessor.processPacket(*rawData);
    return move(packet);
  }

public:
  SpxDecoderElement(string name) : StandardIOElement(name)
  {
    unique_ptr<ItInputPort<RawVideoData>> _inPort(new ItInputPort<RawVideoData>("SpxDecoder_in"));
    setInputPort(move(_inPort));

    unique_ptr<ItOutputPort<RadarVideoSweep>> _outPort(new ItOutputPort<RadarVideoSweep>("SpxDecoder_out"));
    setOutputPort(move(_outPort));
  }

  ~SpxDecoderElement() {}

  
};

#endif // SPX_DECODER_ELEMENT_H