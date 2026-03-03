#ifndef IT_OBJ_H
#define IT_OBJ_H

#include <atomic>

enum ItObjState
{
  IT_STATE_STOPING,
  IT_STATE_STOPED,
  IT_STATE_PAUSING,
  IT_STATE_PAUSED,
  IT_STATE_STARTING,
  IT_STATE_STARTED,
};

class ItObject
{
private:
  std::string name;
  std::atomic<ItObjState> state;

protected:
  virtual void handleStateChanged() = 0;

public:
  ItObject(std::string objName) : name(objName)
  {
    state = IT_STATE_STOPED;
  }

  virtual ~ItObject() {}

  std::string getName() const { return name; }
  
  ItObjState getState() { return state; }

  void setState(ItObjState newState) {
    state = newState;
    handleStateChanged();
  }
};

#endif // IT_OBJ_H