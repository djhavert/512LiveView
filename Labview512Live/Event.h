#pragma once
#include <string>

enum class EventType
{
  StartupRun,
  ConnectComplete,
  ClientDisconnect
};

class Event
{
public:
  Event(EventType type)
    :event_type(type)
  {
  }

  virtual EventType GetEventType() { return event_type; };
  virtual const std::string GetName() const = 0;
protected:
  EventType event_type;
};


class StartupRunEvent : public Event
{
public:
  StartupRunEvent(bool stim_button, std::string filename)
    :Event(EventType::StartupRun), stim_enabled(stim_button), stim_filename(filename)
  {
  }
  inline const std::string GetName() const override { return "StartupRunEvent"; }
  inline const bool StimEnabled() const { return stim_enabled; }
  inline const std::string GetStimFilename() const { return stim_filename; }
protected:
  bool stim_enabled;
  std::string stim_filename;
};
